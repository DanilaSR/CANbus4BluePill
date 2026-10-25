/* Central processor analog (spacecraft OBC) -- node-ID 50.
 *
 * Listens to every data subject published by the six spacecraft units and to
 * their heartbeats, decodes the readings and streams a human-readable report
 * to USART1 (PA9 TX, 115200 8N1):
 *
 *   - an EVENT line the moment something changes: a unit comes online or goes
 *     offline, a health transition, a subject going live/stale, the flywheel
 *     changing state / finishing its self-test / tripping over-temperature;
 *   - a 1 Hz status block: fleet header + one line per unit with the age of
 *     its heartbeat and the latest value of its subject(s).
 *
 * Subscribed subjects:
 *   7509 uavcan.node.Heartbeat.1.0                 (all units)
 *   7510/7515 uavcan.si.sample.angle.Scalar.1.0    (solar alpha/beta, node 51)
 *   7511/7512 si.sample.angular_velocity/acceleration.Vector3.1.0 (IMU, 52)
 *   7513 reg.udral.physics.kinematics.geodetic.Point.0.1 (GNSS, 55)
 *   7514 uavcan.si.sample.temperature.Scalar.1.0   (temperature, 56)
 *   7516 spacecraft.flywheel.Status.1.0            (flywheel, 53)
 *
 * This node publishes its own heartbeat with a fleet-liveness health ladder:
 * NOMINAL = all six units heard within 5 s, ADVISORY = some missing,
 * CAUTION = at most two online, WARNING = none; the vendor-specific status
 * code carries the number of offline units.
 *
 * Concurrency model (same as the other units): libcanard delivers complete
 * transfers inside the CAN RX0 IRQ, where the library must not be re-entered
 * -- the callbacks here only deserialize into seqlocked slots. libcanard is
 * explicitly not thread-safe (canard.h), so the main loop masks the RX0 IRQ
 * around its own canard calls; frames arriving meanwhile wait in the RX FIFO
 * (the masked section is only a few microseconds long). Printing is blocking
 * UART TX; RX keeps running in the IRQ while the loop waits.
 */

#include "main.h"

#include <canard.h>
#include <node_getinfo.h>
#include <reg/udral/physics/kinematics/geodetic/Point_0_1.h>
#include <spacecraft/flywheel/Status_1_0.h>
#include <uavcan/node/Heartbeat_1_0.h>
#include <uavcan/si/sample/acceleration/Vector3_1_0.h>
#include <uavcan/si/sample/angle/Scalar_1_0.h>
#include <uavcan/si/sample/angular_velocity/Vector3_1_0.h>
#include <uavcan/si/sample/temperature/Scalar_1_0.h>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

CAN_HandleTypeDef hcan1;
UART_HandleTypeDef huart1;

static canard_t             canard_inst;
#define CANARD_NODE_ID 50U /* fixed local node ID (Cyphal 7-bit, 1..127) */

static const uint16_t       SUBJECT_HEARTBEAT   = 7509U;
static uint8_t                    hb_transfer_id;
static uint32_t                   hb_uptime_sec;
static canard_us_t                last_hb_us;
static canard_us_t                last_summary_us;

static uint32_t                   can_rx_count;
static uint32_t                   can_tx_count;

/* Defined with the CANard vtable below; used by stamp() earlier in the file. */
static canard_us_t canard_now_cb(const canard_t* self);

/* Liveness thresholds: the units publish at 1 Hz, five missed beats = offline. */
#define HB_TIMEOUT_US       5000000ULL
#define DATA_TIMEOUT_US     5000000ULL

/* ---------------------------------------------------------
 * Expected fleet and per-unit naming (summary/event lines).
 * --------------------------------------------------------- */

static const uint8_t EXPECTED_UNITS[] = {51U, 52U, 53U, 54U, 55U, 56U};
#define EXPECTED_UNIT_COUNT (sizeof(EXPECTED_UNITS) / sizeof(EXPECTED_UNITS[0]))

static bool is_expected(uint_least8_t node) {
  for (size_t i = 0; i < EXPECTED_UNIT_COUNT; i++) {
    if (EXPECTED_UNITS[i] == node) {
      return true;
    }
  }
  return false;
}

static const char* unit_name(uint_least8_t node) {
  switch (node) {
    case 51U: return "solar";
    case 52U: return "imu";
    case 53U: return "flywheel";
    case 54U: return "coil";
    case 55U: return "gnss";
    case 56U: return "temp";
    default:  return "node";
  }
}

static const char* health_name(uint_least8_t h) {
  switch (h) {
    case uavcan_node_Health_1_0_NOMINAL:   return "NOMINAL";
    case uavcan_node_Health_1_0_ADVISORY:  return "ADVISORY";
    case uavcan_node_Health_1_0_CAUTION:   return "CAUTION";
    case uavcan_node_Health_1_0_WARNING:   return "WARNING";
    default:                               return "?";
  }
}

static const char* fw_state_name(uint_least8_t s) {
  switch (s) {
    case spacecraft_flywheel_Status_1_0_STATE_IDLE:           return "IDLE";
    case spacecraft_flywheel_Status_1_0_STATE_SPINNING:       return "SPINNING";
    case spacecraft_flywheel_Status_1_0_STATE_FAULT_OVERTEMP: return "FAULT_OVERTEMP";
    case spacecraft_flywheel_Status_1_0_STATE_SELFTEST:       return "SELFTEST";
    default:                                                  return "?";
  }
}

static const char* fw_selftest_name(uint_least8_t s) {
  switch (s) {
    case spacecraft_flywheel_Status_1_0_SELFTEST_IDLE:    return "idle";
    case spacecraft_flywheel_Status_1_0_SELFTEST_RUNNING: return "running";
    case spacecraft_flywheel_Status_1_0_SELFTEST_PASSED:  return "passed";
    case spacecraft_flywheel_Status_1_0_SELFTEST_FAILED:  return "failed";
    default:                                              return "?";
  }
}

/* ---------------------------------------------------------
 * Bare-metal heap for newlib malloc/free (CANard memory).
 * --------------------------------------------------------- */

void* _sbrk(ptrdiff_t incr) {
  extern char _end;
  static char* heap_end;
  if (heap_end == 0) {
    heap_end = &_end;
  }
  char* prev_heap_end = heap_end;
  heap_end += incr;
  return (void*)prev_heap_end;
}

/* ---------------------------------------------------------
 * Float formatting without newlib %f (broken in this toolchain:
 * even a literal 12.34f prints "-0.00"). Scale by 10^dec with
 * rounding and print as integers; the sign is handled
 * explicitly so values in (-1, 0) keep it ("-0.500").
 * fmt_deg does the same for the double-precision GNSS fields.
 * --------------------------------------------------------- */

static void fmt_f(char* out, size_t n, float v, int dec) {
  static const float pw[4] = {1e0f, 1e1f, 1e2f, 1e3f};
  const float  p   = (dec >= 0 && dec <= 3) ? pw[dec] : 100.0f;
  const bool   neg = (v < 0.0f);
  const unsigned long div = (unsigned long)p;
  const unsigned long sc  = (unsigned long)((neg ? -v : v) * p + 0.5f);
  const unsigned long ip  = (dec > 0) ? sc / div : sc;
  const unsigned long fr  = (dec > 0) ? sc % div : 0UL;
  if (dec > 0) {
    snprintf(out, n, "%s%lu.%0*lu", neg ? "-" : "", ip, dec, fr);
  } else {
    snprintf(out, n, "%s%lu", neg ? "-" : "", ip);
  }
}

static void fmt_deg(char* out, size_t n, double v, int dec) {
  static const double pw[7] = {1.0, 10.0, 100.0, 1000.0, 1e4, 1e5, 1e6};
  const double p   = (dec >= 0 && dec <= 6) ? pw[dec] : 1e6;
  const bool   neg = (v < 0.0);
  const unsigned long div = (unsigned long)p;
  const unsigned long sc  = (unsigned long)((neg ? -v : v) * p + 0.5);
  const unsigned long ip  = (dec > 0) ? sc / div : sc;
  const unsigned long fr  = (dec > 0) ? sc % div : 0UL;
  if (dec > 0) {
    snprintf(out, n, "%s%lu.%0*lu", neg ? "-" : "", ip, dec, fr);
  } else {
    snprintf(out, n, "%s%lu", neg ? "-" : "", ip);
  }
}

/* "[   12.345] " boot-relative timestamp prefix. */
static void stamp(char* out, size_t n) {
  const canard_us_t t = canard_now_cb(NULL);
  snprintf(out, n, "[%5lu.%03lu] ",
           (unsigned long)(t / 1000000ULL), (unsigned long)((t % 1000000ULL) / 1000ULL));
}

/* printf-style line writer into the UART (blocking TX, one call per line). */
static void uout(const char* fmt, ...) {
  char    buf[256];
  va_list ap;
  va_start(ap, fmt);
  const int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  if (n > 0) {
    const size_t len = ((size_t)n < sizeof(buf)) ? (size_t)n : sizeof(buf) - 1U;
    (void)HAL_UART_Transmit(&huart1, (uint8_t*)buf, (uint16_t)len, 500U);
  }
}

/* printf-style append into a line buffer; returns the new length. */
static size_t lcat(char* line, size_t cap, size_t off, const char* fmt, ...) {
  if (off >= cap) {
    return off;
  }
  va_list ap;
  va_start(ap, fmt);
  const int w = vsnprintf(line + off, cap - off, fmt, ap);
  va_end(ap);
  if (w < 0) {
    return off;
  }
  return off + (((size_t)w < cap - off) ? (size_t)w : cap - off - 1U);
}

/* ---------------------------------------------------------
 * CANard vtable: time, TX (order-preserving), no HW filtering.
 * --------------------------------------------------------- */

static canard_us_t canard_now_cb(const canard_t* self) {
  (void)self;
  return (canard_us_t)HAL_GetTick() * 1000;
}

static bool canard_tx_cb(canard_t* self,
                         void*     user_context,
                         canard_us_t    deadline,
                         uint_least8_t  iface_index,
                         bool           fd,
                         uint32_t       extended_can_id,
                         canard_bytes_t data) {
  (void)self;
  (void)user_context;
  (void)deadline;
  (void)iface_index;
  (void)fd; /* Blue Pill bxCAN: Classic CAN only */

  /* Order preservation -- see STM32F103_Flywheel for the full story: bxCAN
   * arbitrates its three mailboxes by CAN ID and, for equal IDs, by mailbox
   * number, while HAL_CAN_AddTxMessage() picks the *first free* mailbox --
   * so a later frame of one transfer can overtake an earlier one and the
   * receiver drops the whole transfer on the CRC check. Never queue a frame
   * while another is still on the wire; NART makes every mailbox free itself
   * after a single attempt, so this cannot stall, and libcanard retries the
   * same frame on the next canard_poll(). */
  if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) < 3U) {
    return false; /* frame still on the wire -> keep our place in line */
  }

  CAN_TxHeaderTypeDef hdr = {0};
  hdr.ExtId            = extended_can_id;
  hdr.IDE              = CAN_ID_EXT;
  hdr.RTR              = CAN_RTR_DATA;
  hdr.DLC              = (uint32_t)data.size;
  hdr.TransmitGlobalTime = DISABLE;

  uint32_t mailbox = 0;
  if (HAL_CAN_AddTxMessage(&hcan1, &hdr, (uint8_t*)data.data, &mailbox) != HAL_OK) {
    return false; /* no free mailbox -> retry on next poll() */
  }
  can_tx_count++;
  return true;
}

static const canard_vtable_t canard_vt = {
    .now = &canard_now_cb,
    .tx  = &canard_tx_cb,
    .filter = NULL, /* accept-all; software filtering inside libcanard */
};

/* ---------------------------------------------------------
 * CANard memory allocator (newlib malloc over _sbrk heap).
 * --------------------------------------------------------- */

static void* mem_alloc_cb(canard_mem_t memory, size_t size) {
  (void)memory;
  return malloc(size);
}

static void mem_free_cb(canard_mem_t memory, size_t size, void* ptr) {
  (void)memory;
  (void)size;
  free(ptr);
}

static const canard_mem_vtable_t mem_vt = {.free = &mem_free_cb, .alloc = &mem_alloc_cb};

/* ---------------------------------------------------------
 * RX0 IRQ mask around main-loop canard calls: the library is
 * not thread-safe and the RX0 IRQ reenters it via
 * canard_ingest_frame(). Frames arriving while masked wait in
 * the RX FIFO; the masked section is only a few microseconds.
 * --------------------------------------------------------- */

static void canard_lock(void)   { HAL_NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn); }
static void canard_unlock(void) { HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn); }

/* ---------------------------------------------------------
 * USART1 (PA9 TX / PA10 RX) -- text console, 115200 8N1.
 * --------------------------------------------------------- */

static void MX_USART1_UART_Init(void) {
  huart1.Instance          = USART1;
  huart1.Init.BaudRate     = 115200U;
  huart1.Init.WordLength   = UART_WORDLENGTH_8B;
  huart1.Init.StopBits     = UART_STOPBITS_1;
  huart1.Init.Parity       = UART_PARITY_NONE;
  huart1.Init.Mode         = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK) {
    Error_Handler();
  }
}

/* ---------------------------------------------------------
 * CAN1 init (PA11 RX / PA12 TX -- default pins, no remap).
 * --------------------------------------------------------- */

static void MX_CAN1_Init(void) {
  __HAL_RCC_CAN1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* PA12 = CAN1_TX (AF push-pull). PA11 = CAN1_RX must be an INPUT on F1:
     AF_PP ignores the pull config, so a pull-up here is what lets the bxCAN
     leave Initialization mode (it needs 11 consecutive recessive bits =
     logic-high on RX). With a transceiver wired, the bus idle level drives
     this pin anyway, so the pull-up is harmless. */
  GPIO_InitTypeDef gpio = {0};
  gpio.Pin   = GPIO_PIN_12;
  gpio.Mode  = GPIO_MODE_AF_PP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &gpio);

  gpio.Pin   = GPIO_PIN_11;
  gpio.Mode  = GPIO_MODE_INPUT;
  gpio.Pull  = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &gpio);

  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler   = 2;               /* APB1=8 MHz / 8 tq / 2 = 500 kbit/s */
  hcan1.Init.Mode        = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1    = CAN_BS1_4TQ;
  hcan1.Init.TimeSeg2    = CAN_BS2_3TQ;
  hcan1.Init.TimeTriggeredMode   = DISABLE;
  hcan1.Init.AutoBusOff          = DISABLE;
  hcan1.Init.AutoWakeUp          = DISABLE;
  hcan1.Init.AutoRetransmission  = DISABLE; /* one-shot: no bus flooding while
                                             no ACKing node is awake yet */
  hcan1.Init.ReceiveFifoLocked   = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK) {
    Error_Handler();
  }

  /* Pass-all hardware filter: CANard does software filtering (filter_count=0),
     so every frame must reach the RX FIFO. */
  CAN_FilterTypeDef f = {0};
  f.FilterActivation   = ENABLE;
  f.FilterMode         = CAN_FILTERMODE_IDMASK;
  f.FilterScale         = CAN_FILTERSCALE_32BIT;
  f.FilterIdHigh       = 0;
  f.FilterIdLow        = 0;
  f.FilterMaskIdHigh   = 0;
  f.FilterMaskIdLow    = 0;
  f.FilterFIFOAssignment = CAN_RX_FIFO0;
  if (HAL_CAN_ConfigFilter(&hcan1, &f) != HAL_OK) {
    Error_Handler();
  }
}

void HAL_CAN_MspInit(CAN_HandleTypeDef* hcan) {
  if (hcan->Instance == CAN1) {
    /* RX FIFO0 IRQ drives CanardRxCallback(). No TX IRQ is needed: with
       NART one-shot the mailbox frees itself (ACKed or not), and TX is
       polled from the main loop via canard_poll(). */
    HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
  }
}

static void MX_GPIO_Init(void) {
  __HAL_RCC_GPIOC_CLK_ENABLE();
  GPIO_InitTypeDef gpio = {0};
  gpio.Pin   = GPIO_PIN_13;
  gpio.Mode  = GPIO_MODE_OUTPUT_PP;
  gpio.Pull  = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &gpio);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); /* LED off */
}

void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /* Straight from HSI @ 8 MHz (no PLL) -- same clocking as the other units. */
  RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType =
      RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI; /* 8 MHz direct */
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1; /* 8 MHz (<=36MHz ok) */
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1; /* 8 MHz */
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
    Error_Handler();
  }
}

/* ---------------------------------------------------------
 * CANard RX -- called from the CAN RX0 IRQ (see it.c).
 * --------------------------------------------------------- */

void CanardRxCallback(void) {
  CAN_RxHeaderTypeDef rx_hdr;
  uint8_t             rx_data[8];
  if (HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &rx_hdr, rx_data) != HAL_OK) {
    return;
  }
  if (rx_hdr.IDE == CAN_ID_EXT) {
    can_rx_count++;
    const canard_bytes_t frame = {.size = rx_hdr.DLC, .data = rx_data};
    (void)canard_ingest_frame(&canard_inst, canard_now_cb(NULL), 0, rx_hdr.ExtId, frame);
  }
}

/* ---------------------------------------------------------
 * Received data: one seqlocked slot per subject, written from
 * the RX0 IRQ (on_sample/on_heartbeat) and read by the main
 * loop (events + summary). seq odd = writer inside.
 * --------------------------------------------------------- */

typedef enum {
  SLOT_SOLAR_ALPHA = 0,
  SLOT_SOLAR_BETA,
  SLOT_GYRO,
  SLOT_ACCEL,
  SLOT_GNSS,
  SLOT_TEMP,
  SLOT_FLYWHEEL,
  SLOT_COUNT
} slot_id_t;

static const uint16_t slot_subject[SLOT_COUNT] = {
    7510U, /* SLOT_SOLAR_ALPHA */
    7515U, /* SLOT_SOLAR_BETA  */
    7511U, /* SLOT_GYRO        */
    7512U, /* SLOT_ACCEL       */
    7513U, /* SLOT_GNSS        */
    7514U, /* SLOT_TEMP        */
    7516U, /* SLOT_FLYWHEEL    */
};

typedef union {
  uavcan_si_sample_angle_Scalar_1_0                 angle;
  uavcan_si_sample_angular_velocity_Vector3_1_0     gyro;
  uavcan_si_sample_acceleration_Vector3_1_0         accel;
  uavcan_si_sample_temperature_Scalar_1_0           temp;
  reg_udral_physics_kinematics_geodetic_Point_0_1   point;
  spacecraft_flywheel_Status_1_0                    flywheel;
} sample_val_u;

typedef struct {
  volatile uint32_t seq;     /* seqlock: odd while the IRQ writer is inside */
  uint_least8_t     node;    /* publisher of the stored sample */
  uint32_t          rx_count;
  canard_us_t       last_us; /* arrival of the first frame of the transfer */
  sample_val_u      val;
} sample_slot_t;

typedef struct {
  uint_least8_t     node;
  uint32_t          rx_count;
  canard_us_t       last_us;
  sample_val_u      val;
} sample_snap_t;

static sample_slot_t samples[SLOT_COUNT];

static bool sample_read(slot_id_t id, sample_snap_t* out) {
  const sample_slot_t* s = &samples[id];
  uint32_t a;
  uint32_t b;
  do {
    a = s->seq;
    if (a & 1U) {
      return false; /* writer inside -> skip this pass */
    }
    out->node     = s->node;
    out->rx_count = s->rx_count;
    out->last_us  = s->last_us;
    out->val      = s->val;
    b = s->seq;
  } while (a != b);
  return out->rx_count > 0U; /* false until the first sample arrived */
}

static bool data_fresh(const sample_snap_t* s, canard_us_t now) {
  return (now - s->last_us) < DATA_TIMEOUT_US;
}

/* Heartbeat table: written from the IRQ, indexed by source node-ID; unknown
 * nodes (e.g. a yakut instance) fill spare slots, the oldest is recycled. */
#define HB_SLOT_COUNT 8U

typedef struct {
  volatile uint32_t seq;
  bool              used;
  uint_least8_t     node;
  uint32_t          uptime;
  uint_least8_t     health;
  uint_least8_t     mode;
  uint_least8_t     vss;
  uint32_t          rx_count;
  canard_us_t       last_us;
} hb_slot_t;

typedef struct {
  bool              used;
  uint_least8_t     node;
  uint32_t          uptime;
  uint_least8_t     health;
  uint_least8_t     mode;
  uint_least8_t     vss;
  uint32_t          rx_count;
  canard_us_t       last_us;
} hb_snap_t;

static hb_slot_t hb_slots[HB_SLOT_COUNT];

static hb_slot_t* hb_slot_for(uint_least8_t node) {
  hb_slot_t* free_slot = NULL;
  hb_slot_t* oldest    = NULL;
  for (size_t i = 0; i < HB_SLOT_COUNT; i++) {
    hb_slot_t* s = &hb_slots[i];
    if (s->used && s->node == node) {
      return s;
    }
    if (!s->used && free_slot == NULL) {
      free_slot = s;
    }
    if (s->used && (oldest == NULL || s->last_us < oldest->last_us)) {
      oldest = s;
    }
  }
  if (free_slot != NULL) {
    return free_slot;
  }
  return (oldest != NULL) ? oldest : &hb_slots[0];
}

static bool hb_read_idx(size_t i, hb_snap_t* out) {
  const hb_slot_t* s = &hb_slots[i];
  uint32_t a;
  uint32_t b;
  do {
    a = s->seq;
    if (a & 1U) {
      return false;
    }
    out->used     = s->used;
    out->node     = s->node;
    out->uptime   = s->uptime;
    out->health   = s->health;
    out->mode     = s->mode;
    out->vss      = s->vss;
    out->rx_count = s->rx_count;
    out->last_us  = s->last_us;
    b = s->seq;
  } while (a != b);
  return true;
}

static bool hb_read_node(uint_least8_t node, hb_snap_t* out) {
  for (size_t i = 0; i < HB_SLOT_COUNT; i++) {
    if (hb_read_idx(i, out) && out->used && out->node == node) {
      return true;
    }
  }
  return false;
}

static bool hb_fresh(const hb_snap_t* hb, canard_us_t now) {
  return (now - hb->last_us) < HB_TIMEOUT_US;
}

/* ---------------------------------------------------------
 * Subscription callbacks -- run in the RX0 IRQ: deserialize
 * and publish into the slot, nothing else (no libcanard calls,
 * no printf, no UART).
 * --------------------------------------------------------- */

/* canard.h contract: on multi-frame transfers payload.origin owns the
 * rx_payload block and the application must release it; single-frame
 * transfers arrive with origin.data == NULL, which this handles as a no-op.
 * Failing to release leaked one slot per multi-frame transfer (~1.3 KiB/s
 * under fleet traffic): the heap grew into the ISR stack, the collision
 * corrupted live rx sessions and CANARD_ASSERT fired inside the CAN RX ISR,
 * where newlib's abort never returns -- the node hung with VECTACTIVE=36
 * and uwTick frozen. */
static void release_payload(canard_payload_t* const payload)
{
  if (payload->origin.data != NULL) {
    free(payload->origin.data);
    payload->origin.data = NULL;
    payload->origin.size = 0U;
  }
}

static void on_heartbeat(canard_subscription_t* self,
                         canard_us_t            timestamp,
                         canard_prio_t          priority,
                         uint_least8_t          source_node_id,
                         uint_least8_t          transfer_id,
                         canard_payload_t       payload) {
  (void)self;
  (void)priority;
  (void)transfer_id;
  if (source_node_id > CANARD_NODE_ID_MAX) {
    release_payload(&payload);
    return; /* anonymous source: nothing to index by */
  }
  uavcan_node_Heartbeat_1_0 v;
  size_t sz = payload.view.size;
  if (uavcan_node_Heartbeat_1_0_deserialize_(&v, payload.view.data, &sz) < 0) {
    release_payload(&payload);
    return;
  }
  hb_slot_t* const s = hb_slot_for((uint_least8_t)source_node_id);
  s->seq++;
  __DMB(); /* payload before seq publication */
  s->used     = true;
  s->node     = (uint_least8_t)source_node_id;
  s->uptime   = v.uptime;
  s->health   = v.health.value;
  s->mode     = v.mode.value;
  s->vss      = v.vendor_specific_status_code;
  s->last_us  = timestamp;
  s->rx_count++;
  __DMB();
  s->seq++;
  release_payload(&payload);
}

static void on_sample(canard_subscription_t* self,
                      canard_us_t            timestamp,
                      canard_prio_t          priority,
                      uint_least8_t          source_node_id,
                      uint_least8_t          transfer_id,
                      canard_payload_t       payload) {
  (void)priority;
  (void)transfer_id;
  if (source_node_id > CANARD_NODE_ID_MAX) {
    release_payload(&payload);
    return;
  }
  slot_id_t id = SLOT_COUNT;
  for (size_t i = 0; i < SLOT_COUNT; i++) {
    if (slot_subject[i] == self->port_id) {
      id = (slot_id_t)i;
      break;
    }
  }
  if (id == SLOT_COUNT) {
    release_payload(&payload);
    return;
  }

  sample_val_u v;
  bool         ok  = false;
  size_t       sz  = payload.view.size;
  switch (id) {
    case SLOT_SOLAR_ALPHA:
    case SLOT_SOLAR_BETA: {
      uavcan_si_sample_angle_Scalar_1_0 t;
      if (uavcan_si_sample_angle_Scalar_1_0_deserialize_(&t, payload.view.data, &sz) >= 0) {
        v.angle = t;
        ok      = true;
      }
      break;
    }
    case SLOT_GYRO: {
      uavcan_si_sample_angular_velocity_Vector3_1_0 t;
      if (uavcan_si_sample_angular_velocity_Vector3_1_0_deserialize_(&t, payload.view.data, &sz) >= 0) {
        v.gyro = t;
        ok     = true;
      }
      break;
    }
    case SLOT_ACCEL: {
      uavcan_si_sample_acceleration_Vector3_1_0 t;
      if (uavcan_si_sample_acceleration_Vector3_1_0_deserialize_(&t, payload.view.data, &sz) >= 0) {
        v.accel = t;
        ok      = true;
      }
      break;
    }
    case SLOT_GNSS: {
      reg_udral_physics_kinematics_geodetic_Point_0_1 t;
      if (reg_udral_physics_kinematics_geodetic_Point_0_1_deserialize_(&t, payload.view.data, &sz) >= 0) {
        v.point = t;
        ok      = true;
      }
      break;
    }
    case SLOT_TEMP: {
      uavcan_si_sample_temperature_Scalar_1_0 t;
      if (uavcan_si_sample_temperature_Scalar_1_0_deserialize_(&t, payload.view.data, &sz) >= 0) {
        v.temp = t;
        ok     = true;
      }
      break;
    }
    case SLOT_FLYWHEEL: {
      spacecraft_flywheel_Status_1_0 t;
      if (spacecraft_flywheel_Status_1_0_deserialize_(&t, payload.view.data, &sz) >= 0) {
        v.flywheel = t;
        ok         = true;
      }
      break;
    }
    default:
      break;
  }
  if (!ok) {
    release_payload(&payload);
    return;
  }

  sample_slot_t* const s = &samples[id];
  s->seq++;
  __DMB(); /* payload before seq publication */
  s->val      = v;
  s->node     = (uint_least8_t)source_node_id;
  s->last_us  = timestamp;
  s->rx_count++;
  __DMB();
  s->seq++;
  release_payload(&payload);
}

static const canard_subscription_vtable_t hb_vt     = {.on_message = &on_heartbeat};
static const canard_subscription_vtable_t sample_vt = {.on_message = &on_sample};

/* ---------------------------------------------------------
 * Fleet health ladder: all six units heard -> NOMINAL, some
 * missing -> ADVISORY, at most two online -> CAUTION, none ->
 * WARNING. vss = number of offline units.
 * --------------------------------------------------------- */

static uint8_t fleet_online(canard_us_t now) {
  uint8_t online = 0;
  for (size_t i = 0; i < EXPECTED_UNIT_COUNT; i++) {
    hb_snap_t hb;
    if (hb_read_node(EXPECTED_UNITS[i], &hb) && hb_fresh(&hb, now)) {
      online++;
    }
  }
  return online;
}

static uint_least8_t health_for(uint8_t online) {
  if (online >= EXPECTED_UNIT_COUNT) {
    return uavcan_node_Health_1_0_NOMINAL;
  }
  if (online >= 3U) {
    return uavcan_node_Health_1_0_ADVISORY;
  }
  if (online >= 1U) {
    return uavcan_node_Health_1_0_CAUTION;
  }
  return uavcan_node_Health_1_0_WARNING;
}

/* ---------------------------------------------------------
 * Event detection (main loop): changes since the last pass are
 * printed immediately as EVENT lines. Shadows below are owned
 * by the main loop only.
 * --------------------------------------------------------- */

typedef struct {
  bool          valid;
  uint_least8_t node;
  bool          online;
  uint_least8_t health;
} hb_shadow_t;

static hb_shadow_t hb_shadow[HB_SLOT_COUNT];

typedef struct {
  bool ever;   /* at least one sample received */
  bool stale;  /* was live and timed out (while the unit's hb was fresh) */
} data_shadow_t;

static data_shadow_t data_shadow[SLOT_COUNT];

typedef struct {
  bool          valid;
  uint_least8_t state;
  uint_least8_t selftest;
  uint_least8_t overtemp;
  bool          at_max;
} fw_shadow_t;

static fw_shadow_t fw_shadow;

static void detect_events(void) {
  const canard_us_t now = canard_now_cb(NULL);
  char ts[24];
  char f1[16];
  stamp(ts, sizeof(ts));

  /* Heartbeat liveness and health transitions (expected units only). */
  for (size_t i = 0; i < HB_SLOT_COUNT; i++) {
    hb_snap_t hb;
    if (!hb_read_idx(i, &hb)) {
      continue; /* writer inside this slot this instant */
    }
    if (!hb.used || !is_expected(hb.node)) {
      hb_shadow[i].valid = false;
      continue;
    }
    const bool fresh    = hb_fresh(&hb, now);
    hb_shadow_t* const sh = &hb_shadow[i];
    if (!sh->valid || sh->node != hb.node) {
      *sh = (hb_shadow_t){.valid = true, .node = hb.node, .online = fresh, .health = hb.health};
      if (fresh) {
        uout("%sEVENT %u %s online (health %s, uptime %lu s)\r\n", ts, (unsigned)hb.node,
             unit_name(hb.node), health_name(hb.health), (unsigned long)hb.uptime);
      }
      continue;
    }
    if (sh->online != fresh) {
      sh->online = fresh;
      if (fresh) {
        uout("%sEVENT %u %s online (health %s, uptime %lu s)\r\n", ts, (unsigned)hb.node,
             unit_name(hb.node), health_name(hb.health), (unsigned long)hb.uptime);
      } else {
        fmt_f(f1, sizeof(f1), (float)((now - hb.last_us) / 1000000.0), 1);
        uout("%sEVENT %u %s offline (last heartbeat %ss ago)\r\n", ts, (unsigned)hb.node,
             unit_name(hb.node), f1);
      }
    }
    if (fresh && sh->health != hb.health) {
      uout("%sEVENT %u %s health %s -> %s (vss %u)\r\n", ts, (unsigned)hb.node,
           unit_name(hb.node), health_name(sh->health), health_name(hb.health),
           (unsigned)hb.vss);
      sh->health = hb.health;
    } else if (!fresh) {
      sh->health = hb.health; /* do not announce transitions while offline */
    }
  }

  /* Data-subject liveness + flywheel transitions. */
  for (size_t i = 0; i < SLOT_COUNT; i++) {
    sample_snap_t s;
    if (!sample_read((slot_id_t)i, &s)) {
      continue;
    }
    const bool   fresh = data_fresh(&s, now);
    data_shadow_t* const sh = &data_shadow[i];

    if (!sh->ever) {
      sh->ever  = true;
      sh->stale = false;
      uout("%sEVENT %u %s: subject %u live\r\n", ts, (unsigned)s.node, unit_name(s.node),
           (unsigned)slot_subject[i]);
    } else if (sh->stale && fresh) {
      sh->stale = false;
      uout("%sEVENT %u %s: subject %u data live again\r\n", ts, (unsigned)s.node,
           unit_name(s.node), (unsigned)slot_subject[i]);
    } else if (!sh->stale && !fresh) {
      /* Announce a data dropout only while the publisher's heartbeat is
         still fresh: otherwise the unit-offline event already says it. */
      hb_snap_t hb;
      if (hb_read_node(s.node, &hb) && hb_fresh(&hb, now)) {
        fmt_f(f1, sizeof(f1), (float)((now - s.last_us) / 1000000.0), 1);
        uout("%sEVENT %u %s: subject %u data stale (last sample %ss ago)\r\n", ts,
             (unsigned)s.node, unit_name(s.node), (unsigned)slot_subject[i], f1);
        sh->stale = true;
      }
    }

    if ((slot_id_t)i == SLOT_FLYWHEEL) {
      const spacecraft_flywheel_Status_1_0* const fw = &s.val.flywheel;
      if (!fw_shadow.valid) {
        fw_shadow = (fw_shadow_t){.valid   = true,
                                  .state   = fw->state,
                                  .selftest = fw->selftest_state,
                                  .overtemp = fw->overtemp_count,
                                  .at_max  = fw->at_max_speed};
      } else {
        const bool state_ch   = fw_shadow.state != fw->state;
        const bool ot_ch      = fw_shadow.overtemp != fw->overtemp_count;
        const bool sts_ch     = fw_shadow.selftest != fw->selftest_state;
        const bool max_ch     = fw_shadow.at_max != fw->at_max_speed;
        if (ot_ch) {
          uout("%sEVENT 53 flywheel over-temperature trips %u -> %u\r\n", ts,
               (unsigned)fw_shadow.overtemp, (unsigned)fw->overtemp_count);
        }
        if (state_ch) {
          uout("%sEVENT 53 flywheel state %s -> %s (overtemp trips %u)\r\n", ts,
               fw_state_name(fw_shadow.state), fw_state_name(fw->state),
               (unsigned)fw->overtemp_count);
        }
        if (sts_ch) {
          fmt_f(f1, sizeof(f1), fw->selftest_error_rpm, 2);
          uout("%sEVENT 53 flywheel selftest %s -> %s (error %srpm)\r\n", ts,
               fw_selftest_name(fw_shadow.selftest), fw_selftest_name(fw->selftest_state), f1);
        }
        if (max_ch) {
          uout("%sEVENT 53 flywheel speed limit %s\r\n", ts,
               fw->at_max_speed ? "reached (torque cut)" : "released");
        }
        fw_shadow = (fw_shadow_t){.valid   = true,
                                  .state   = fw->state,
                                  .selftest = fw->selftest_state,
                                  .overtemp = fw->overtemp_count,
                                  .at_max  = fw->at_max_speed};
      }
    }
  }
}

/* ---------------------------------------------------------
 * 1 Hz status block: fleet header + one line per unit.
 * --------------------------------------------------------- */

static void print_unit_line(uint_least8_t unit, canard_us_t now) {
  char line[224];
  char ts[24];
  char f[4][16];
  bool any_fresh = false;
  bool any_ever  = false;

  stamp(ts, sizeof(ts));
  size_t m = lcat(line, sizeof(line), 0U, "%s%-3u %-9s", ts, (unsigned)unit, unit_name(unit));

  hb_snap_t hb;
  if (hb_read_node(unit, &hb)) {
    fmt_f(f[0], sizeof(f[0]), (float)((now - hb.last_us) / 1000000.0), 1);
    m = lcat(line, sizeof(line), m, " hb %5s%s", f[0],
             hb_fresh(&hb, now) ? "s" : "s STALE");
  } else {
    m = lcat(line, sizeof(line), m, " hb     --");
  }
  m = lcat(line, sizeof(line), m, " |");

  switch (unit) {
    case 51: { /* solar: alpha/beta angles (wire is radians -> print degrees) */
      sample_snap_t sa;
      sample_snap_t sb;
      const bool ea = sample_read(SLOT_SOLAR_ALPHA, &sa);
      const bool eb = sample_read(SLOT_SOLAR_BETA, &sb);
      const bool va = ea && data_fresh(&sa, now);
      const bool vb = eb && data_fresh(&sb, now);
      any_ever  = ea || eb;
      any_fresh = va || vb;
      if (va) {
        fmt_f(f[0], sizeof(f[0]), sa.val.angle.radian * 57.2957795f, 3);
      } else {
        snprintf(f[0], sizeof(f[0]), "--");
      }
      if (vb) {
        fmt_f(f[1], sizeof(f[1]), sb.val.angle.radian * 57.2957795f, 3);
      } else {
        snprintf(f[1], sizeof(f[1]), "--");
      }
      m = lcat(line, sizeof(line), m, " alpha %sdeg beta %sdeg", f[0], f[1]);
      break;
    }
    case 52: { /* IMU: gyro (rad/s) + accel (m/s2) */
      sample_snap_t sg;
      sample_snap_t sa;
      const bool eg = sample_read(SLOT_GYRO, &sg);
      const bool ea = sample_read(SLOT_ACCEL, &sa);
      const bool vg = eg && data_fresh(&sg, now);
      const bool va = ea && data_fresh(&sa, now);
      any_ever  = eg || ea;
      any_fresh = vg || va;
      const float* wg = vg ? sg.val.gyro.radian_per_second : NULL;
      const float* ac = va ? sa.val.accel.meter_per_second_per_second : NULL;
      for (int k = 0; k < 3; k++) {
        if (wg != NULL) {
          fmt_f(f[k], sizeof(f[k]), wg[k], 3);
        } else {
          snprintf(f[k], sizeof(f[k]), "--");
        }
      }
      m = lcat(line, sizeof(line), m, " w %s %s %s rad/s", f[0], f[1], f[2]);
      for (int k = 0; k < 3; k++) {
        if (ac != NULL) {
          fmt_f(f[k], sizeof(f[k]), ac[k], 3);
        } else {
          snprintf(f[k], sizeof(f[k]), "--");
        }
      }
      m = lcat(line, sizeof(line), m, " a %s %s %s m/s2", f[0], f[1], f[2]);
      break;
    }
    case 53: { /* flywheel status */
      sample_snap_t s;
      const bool e = sample_read(SLOT_FLYWHEEL, &s);
      const bool v = e && data_fresh(&s, now);
      any_ever  = e;
      any_fresh = v;
      if (v) {
        const spacecraft_flywheel_Status_1_0* const fw = &s.val.flywheel;
        fmt_f(f[0], sizeof(f[0]), fw->speed_rpm, 1);
        fmt_f(f[1], sizeof(f[1]), fw->target_rpm, 1);
        fmt_f(f[2], sizeof(f[2]), fw->temperature, 1);
        fmt_f(f[3], sizeof(f[3]), fw->momentum, 3);
        m = lcat(line, sizeof(line), m, " %s %s/%srpm T %sC mom %s", fw_state_name(fw->state),
                 f[0], f[1], f[2], f[3]);
        if (fw->selftest_state != spacecraft_flywheel_Status_1_0_SELFTEST_IDLE) {
          fmt_f(f[0], sizeof(f[0]), fw->selftest_error_rpm, 2);
          m = lcat(line, sizeof(line), m, " sts %s err %srpm",
                   fw_selftest_name(fw->selftest_state), f[0]);
        } else {
          m = lcat(line, sizeof(line), m, " sts idle");
        }
        if (fw->overtemp_count > 0U) {
          m = lcat(line, sizeof(line), m, " ot:%u", (unsigned)fw->overtemp_count);
        }
        if (fw->at_max_speed) {
          m = lcat(line, sizeof(line), m, " @max");
        }
      } else {
        m = lcat(line, sizeof(line), m, " --");
      }
      break;
    }
    case 54: { /* coil: heartbeat only */
      m = lcat(line, sizeof(line), m, " (heartbeat only)");
      break;
    }
    case 55: { /* GNSS fix: degrees + meters */
      sample_snap_t s;
      const bool e = sample_read(SLOT_GNSS, &s);
      const bool v = e && data_fresh(&s, now);
      any_ever  = e;
      any_fresh = v;
      if (v) {
        fmt_deg(f[0], sizeof(f[0]), s.val.point.latitude, 6);
        fmt_deg(f[1], sizeof(f[1]), s.val.point.longitude, 6);
        fmt_deg(f[2], sizeof(f[2]), s.val.point.altitude.meter, 1);
        m = lcat(line, sizeof(line), m, " lat %s lon %s alt %sm", f[0], f[1], f[2]);
      } else {
        m = lcat(line, sizeof(line), m, " --");
      }
      break;
    }
    case 56: { /* board temperature: kelvin on the wire, C in parentheses */
      sample_snap_t s;
      const bool e = sample_read(SLOT_TEMP, &s);
      const bool v = e && data_fresh(&s, now);
      any_ever  = e;
      any_fresh = v;
      if (v) {
        fmt_f(f[0], sizeof(f[0]), s.val.temp.kelvin, 2);
        fmt_f(f[1], sizeof(f[1]), s.val.temp.kelvin - 273.15f, 1);
        m = lcat(line, sizeof(line), m, " %sK (%sC)", f[0], f[1]);
      } else {
        m = lcat(line, sizeof(line), m, " --");
      }
      break;
    }
    default:
      break;
  }

  if (unit != 54 && !any_fresh) {
    m = lcat(line, sizeof(line), m, any_ever ? " [data stale]" : " [no data]");
  }
  (void)m;
  uout("%s\r\n", line);
}

static void print_summary(void) {
  const canard_us_t now    = canard_now_cb(NULL);
  const uint8_t     online = fleet_online(now);
  char ts[24];
  stamp(ts, sizeof(ts));
  uout("%s=== units online %u/%u, health %s, uptime %lu s, rx %lu frames ===\r\n", ts,
       (unsigned)online, (unsigned)EXPECTED_UNIT_COUNT,
       health_name(health_for(online)), (unsigned long)hb_uptime_sec,
       (unsigned long)can_rx_count);
  for (size_t i = 0; i < EXPECTED_UNIT_COUNT; i++) {
    print_unit_line(EXPECTED_UNITS[i], now);
  }
}

/* ---------------------------------------------------------
 * Own heartbeat, 1 Hz, fleet-liveness health.
 * --------------------------------------------------------- */

static void publish_heartbeat(canard_us_t now) {
  const uint8_t     online = fleet_online(now);
  const uint8_t     offline = (uint8_t)(EXPECTED_UNIT_COUNT - online);

  uavcan_node_Heartbeat_1_0 hb = {0};
  hb.uptime                   = hb_uptime_sec++;
  hb.health.value             = health_for(online);
  hb.mode.value               = uavcan_node_Mode_1_0_OPERATIONAL;
  hb.vendor_specific_status_code = offline;

  uint8_t   buf[7];
  size_t    sz = sizeof(buf);
  const int8_t err = uavcan_node_Heartbeat_1_0_serialize_(&hb, buf, &sz);
  if (err >= 0) {
    const canard_bytes_chain_t chain = {
        .bytes = {.size = sz, .data = buf},
        .next  = NULL,
    };
    /* Cyphal v1.0 CAN ID layout via canard_publish_13b(): subject-ID in
       bits [20:8], source node-ID in [6:0]. Deliberately not
       canard_publish_16b() (v1.1): pycyphal/yakut implement the v1.0
       layout only. */
    canard_lock();
    (void)canard_publish_13b(&canard_inst, now + 5000000, 1U, canard_prio_fast,
                             SUBJECT_HEARTBEAT, hb_transfer_id++, chain, NULL);
    canard_unlock();
  }
}

/* ---------------------------------------------------------
 * main
 * --------------------------------------------------------- */

int main(void) {
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_CAN1_Init();

  const canard_mem_t mem = {.vtable = &mem_vt, .context = NULL};
  const canard_mem_set_t mems = {
      .tx_transfer = mem,
      .tx_frame    = mem,
      .rx_session  = mem,
      .rx_payload  = mem,
      .rx_filters  = mem, /* unused: filter_count == 0 */
  };

  if (!canard_new(&canard_inst, &canard_vt, mems, 1U /* iface 0 */,
                  32U /* queue capacity in frames */, 0x12345678ULL /* PRNG seed */,
                  0U /* no HW filters */)) {
    Error_Handler();
  }
  canard_inst.tx.fd = false; /* STM32F103 bxCAN: Classic CAN only */

  /* Fix the node ID: by default canard_new() draws it randomly from the PRNG
     seed each boot, which would make the source node-ID change between boots. */
  (void)canard_set_node_id(&canard_inst, CANARD_NODE_ID);

  static const node_getinfo_config_t getinfo_cfg = {
      .hardware_version_major = 1U,
      .hardware_version_minor = 0U,
      .software_version_major = 1U,
      .software_version_minor = 0U,
      .software_vcs_revision_id = 0U,
      .name = "canbus4bluepill.central",
      .unique_id = NULL,
  };
  if (!node_getinfo_init(&canard_inst, &getinfo_cfg)) {
    Error_Handler();
  }

  /* Subscriptions: extent per the type's declared extent, 5 s transfer-ID
     timeout for deduplication (same value the flywheel uses). All are set up
     before the RX IRQ is armed, so no locking is needed here. */
  static canard_subscription_t sub_heartbeat;
  static canard_subscription_t sub_samples[SLOT_COUNT];
  if (canard_subscribe_13b(&canard_inst, &sub_heartbeat, SUBJECT_HEARTBEAT,
                           uavcan_node_Heartbeat_1_0_EXTENT_BYTES_, 5000000U,
                           &hb_vt) == NULL) {
    Error_Handler();
  }
  static const size_t slot_extent[SLOT_COUNT] = {
      uavcan_si_sample_angle_Scalar_1_0_EXTENT_BYTES_,
      uavcan_si_sample_angle_Scalar_1_0_EXTENT_BYTES_,
      uavcan_si_sample_angular_velocity_Vector3_1_0_EXTENT_BYTES_,
      uavcan_si_sample_acceleration_Vector3_1_0_EXTENT_BYTES_,
      reg_udral_physics_kinematics_geodetic_Point_0_1_EXTENT_BYTES_,
      uavcan_si_sample_temperature_Scalar_1_0_EXTENT_BYTES_,
      spacecraft_flywheel_Status_1_0_EXTENT_BYTES_,
  };
  for (size_t i = 0; i < SLOT_COUNT; i++) {
    if (canard_subscribe_13b(&canard_inst, &sub_samples[i], slot_subject[i], slot_extent[i],
                             5000000U, &sample_vt) == NULL) {
      Error_Handler();
    }
  }

  if (HAL_CAN_Start(&hcan1) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
    Error_Handler();
  }

  char ts[24];
  stamp(ts, sizeof(ts));
  uout("\r\n%scentral processor (node %u) -- USART1 115200 8N1 (PA9 TX)\r\n", ts,
       (unsigned)CANARD_NODE_ID);
  uout("%ssubscribed: 7509 hb | 7510 7515 solar | 7511 7512 imu | 7513 gnss | 7514 temp | 7516 flywheel\r\n",
       ts);
  uout("%sexpected units: 51 52 53 54 55 56 -- waiting for traffic ...\r\n", ts);

  const canard_us_t t0 = canard_now_cb(NULL);
  last_hb_us      = t0;
  last_summary_us = t0;

  while (1) {
    const canard_us_t now = canard_now_cb(NULL);

    /* Publish uavcan.node.Heartbeat.1.0 every second. */
    if ((now - last_hb_us) >= 1000000ULL) {
      publish_heartbeat(now);
      last_hb_us = now;
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }

    /* Drain the TX pipeline (RX0 IRQ masked: libcanard is not thread-safe).
       iface 0 is always "ready": with NART=1 every one-shot frame frees its
       mailbox on its own, and a rejected frame is retried on the next poll(). */
    canard_lock();
    node_getinfo_poll(&canard_inst);
    canard_poll(&canard_inst, 1U);
    canard_unlock();

    detect_events();

    if ((now - last_summary_us) >= 1000000ULL) {
      print_summary();
      last_summary_us = now;
    }
  }
}

void SysTick_Handler(void) { HAL_IncTick(); }

void Error_Handler(void) {
  __disable_irq();
  while (1) {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line) {
  (void)file;
  (void)line;
}
#endif
