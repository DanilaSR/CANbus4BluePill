/* Solar sensor unit (spacecraft unit 1/6) -- node-ID 51, Heartbeat.1.0 (7509) +
 * si.sample.angle.Scalar.1.0 (7510 = alpha, 7515 = beta: the two sun aspect
 * angles, published back-to-back from the same sample).
 * Derived from the STM32F103_CAN template.
 */

#include "main.h"

#include <canard.h>
#include <node_getinfo.h>
#include <uavcan/node/Heartbeat_1_0.h>
#include <uavcan/si/sample/angle/Scalar_1_0.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

CAN_HandleTypeDef hcan1;

static canard_t             canard_inst;
#define CANARD_NODE_ID 51U /* fixed local node ID (Cyphal 7-bit, 1..127) */
static const uint16_t       SUBJECT_HEARTBEAT = 7509U;
static uint8_t                    hb_transfer_id;
static uint32_t                   hb_uptime_sec;
static canard_us_t                last_hb_us;
static const uint16_t       SUBJECT_SOLAR_ALPHA = 7510U;
static const uint16_t       SUBJECT_SOLAR_BETA  = 7515U;
static uint8_t                    data_alpha_tid;
static uint8_t                    data_beta_tid;
static uint32_t                   data_seq;
static canard_us_t                last_data_us;

static uint32_t                   can_rx_count;
static uint32_t                   can_tx_count;

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
 * CANard vtable: time, TX, (no HW filtering).
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

  /* Order preservation -- see STM32F103_Flywheel for the full story. With
   * ARTE a frame that lost arbitration holds its mailbox until it finally
   * wins, and bxCAN chooses between mailboxes by CAN ID, so a later frame of
   * one transfer can overtake an earlier one -- the receiver then fails the
   * CRC and drops the whole transfer. Keep at most one frame in flight;
   * libcanard retries the same frame on the next canard_poll(). */
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
 * CAN1 init (PA11 RX / PA12 TX — default pins, no remap).
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
  /* ABOM: a node that ends up bus-off (unACKed or shorted bus) must recover
   * on its own -- otherwise it stays mute until someone resets it. */
  hcan1.Init.AutoBusOff          = ENABLE;
  hcan1.Init.AutoWakeUp          = DISABLE;
  /* ARTE: MANDATORY on a shared bus. Losing arbitration is NORMAL operation
   * (any two nodes whose 1 Hz timers line up collide every single time), and
   * only a retry-after-loss keeps the frame. With NART the loser ABORTS the
   * frame silently -- no error frame, no error counter, it simply never
   * reaches the wire -- so a node booted in phase with the others is
   * invisible forever. The "bus flooding" NART used to avoid is bounded by
   * ARTE itself: an unACKed frame drives the TX error counter up until the
   * node goes bus-off, and ABOM above then recovers it automatically. */
  hcan1.Init.AutoRetransmission  = ENABLE;
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
  f.FilterScale        = CAN_FILTERSCALE_32BIT;
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
    /* RX FIFO0 IRQ drives CanardRxCallback(). No TX IRQ is needed: a mailbox
       frees itself once the frame is really on the wire, and TX progress is
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

  /* Straight from HSI @ 8 MHz (no PLL) — same clocking as the IMU board. */
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
 * CANard RX — called from the CAN RX0 IRQ (see it.c).
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
 * main
 * --------------------------------------------------------- */

int main(void) {
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
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
     seed each boot (seed is XORed with the instance address), which makes the
     source node-ID change between boots. A heartbeat publisher needs a stable
     ID so other nodes can find it. */
  (void)canard_set_node_id(&canard_inst, CANARD_NODE_ID);

  static const node_getinfo_config_t getinfo_cfg = {
      .hardware_version_major = 1U,
      .hardware_version_minor = 0U,
      .software_version_major = 1U,
      .software_version_minor = 0U,
      .software_vcs_revision_id = 0U,
      .name = "canbus4bluepill.solar",
      .unique_id = NULL,
  };
  if (!node_getinfo_init(&canard_inst, &getinfo_cfg)) {
    Error_Handler();
  }

  if (HAL_CAN_Start(&hcan1) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
    Error_Handler();
  }

  last_hb_us = canard_now_cb(NULL);

  while (1) {
    /* Publish uavcan.node.Heartbeat.1.0 every second. */
    const canard_us_t now = canard_now_cb(NULL);
    if ((now - last_hb_us) >= 1000000) {
      uavcan_node_Heartbeat_1_0 hb = {0};
      hb.uptime   = hb_uptime_sec++;
      hb.health.value = uavcan_node_Health_1_0_NOMINAL;
      hb.mode.value   = uavcan_node_Mode_1_0_OPERATIONAL;

      uint8_t   buf[7];
      size_t    sz = sizeof(buf);
      const int8_t err = uavcan_node_Heartbeat_1_0_serialize_(&hb, buf, &sz);
      if (err >= 0) {
        const canard_bytes_chain_t chain = {
            .bytes = {.size = sz, .data = buf},
            .next  = NULL,
        };
        /*
         * Cyphal v1.0 CAN ID layout via canard_publish_13b():
         *   bits [28:26] priority
         *   bit    [25]  service_not_message = 0
         *   bit    [24]  anonymous = 0
         *   bit    [23]  0
         *   bits  [22:21] 11  (v1.0 subject-length discriminator)
         *   bits  [20:8] subject_id[13]
         *   bit     [7]  0
         *   bits   [6:0] source_node_id
         * NB: we deliberately do NOT use canard_publish_16b() (Cyphal v1.1,
         * bit 7 = 1): the current Python-side tooling (pycyphal 1.27.x,
         * yakut 0.14.x) implements only the v1.0 CAN layout and rejects
         * v1.1 frames ("wrong protocol"). Subject 7509 fits in 13 bits, so
         * v1.0 is fully sufficient for the heartbeat.
         */
        (void)canard_publish_13b(&canard_inst, now + 5000000, 1U, canard_prio_fast,
                                 SUBJECT_HEARTBEAT, hb_transfer_id++, chain, NULL);
      }
      last_hb_us = now;
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }

    /* Publish the two sun aspect angles as uavcan.si.sample.angle.Scalar.1.0
     * on subjects 7510 (alpha) and 7515 (beta) at 1 Hz, serialized back-to-
     * back from the same sample. No solar sensor is attached to this board:
     * the values below are a synthetic sawtooth drift within +/-60 deg (a
     * typical coarse sun sensor field of view), alpha and beta on different
     * periods so the two traces look independent -- replace with a real
     * photodiode/ADC readout. Sample timestamps stay 0 (no network time
     * sync in this demo). */
    if ((now - last_data_us) >= 1000000U) {
      const float fov = 1.0471975512f; /* +/-(60 deg) in radians */

      uavcan_si_sample_angle_Scalar_1_0 alpha = {0};
      alpha.radian = fov * ((float)(data_seq % 60) / 60.0f - 0.5f) * 2.0f;

      uavcan_si_sample_angle_Scalar_1_0 beta = {0};
      beta.radian = fov * ((float)(data_seq % 45) / 45.0f - 0.5f) * 2.0f;
      data_seq++;

      uint8_t buf[32];
      size_t  sz = sizeof(buf);
      if (uavcan_si_sample_angle_Scalar_1_0_serialize_(&alpha, buf, &sz) >= 0) {
        const canard_bytes_chain_t chain = {
            .bytes = {.size = sz, .data = buf},
            .next  = NULL,
        };
        (void)canard_publish_13b(&canard_inst, now + 5000000, 1U, canard_prio_nominal,
                                 SUBJECT_SOLAR_ALPHA, data_alpha_tid++, chain, NULL);
      }

      sz = sizeof(buf);
      if (uavcan_si_sample_angle_Scalar_1_0_serialize_(&beta, buf, &sz) >= 0) {
        const canard_bytes_chain_t chain = {
            .bytes = {.size = sz, .data = buf},
            .next  = NULL,
        };
        (void)canard_publish_13b(&canard_inst, now + 5000000, 1U, canard_prio_nominal,
                                 SUBJECT_SOLAR_BETA, data_beta_tid++, chain, NULL);
      }
      last_data_us = now;
    }

    /* Drain the TX pipeline. iface 0 is always "ready": with NART=1 every
     * one-shot frame frees its mailbox on its own (ACKed or not), so a failed
     * attempt can never wedge TX. canard_tx_cb() returns false only while all
     * three mailboxes are momentarily busy, and libcanard retries the same
     * frame on the next canard_poll() (see tx_eject_pending). */
    node_getinfo_poll(&canard_inst);
    canard_poll(&canard_inst, 1U);
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