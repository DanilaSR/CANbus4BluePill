/* Flywheel unit (spacecraft unit 3/6) -- node-ID 53.
 * Derived from the STM32F103_CAN template.
 *
 * Reaction-wheel controller API (custom DSDL namespace spacecraft.flywheel.*,
 * sources in ~/.cyphal/dsdl, generated C in DSDLtypes/):
 *
 *   service 100 SetSpeed.1.0      -- spin to a target speed [rpm]. The response
 *      comes back IMMEDIATELY with acceptance verdict + ETA (a full spin-up takes
 *      about a minute), never when the speed is reached; the supervisor tracks
 *      progress by subscribing to Status (target_reached/speed_rpm) at whatever
 *      rate its control loop needs. Requests that exceed the maximum wheel speed
 *      are clamped and reported as OUTCOME_AT_MAX_SPEED, which also covers
 *      "already rotating at the max speed".
 *   service 101 CreateTorque.1.0  -- apply torque [N*m] for dt [s]: the impulse
 *      primitive (equivalently a global acceleration for a while; the response
 *      translates the impulse into the expected speed change).
 *   service 102 StartSelfTest.1.0 -- sine-wiggle actuator self-test for parts of
 *      the orbit where attitude does not matter (shadow portion). The response
 *      only acknowledges; the verdict (PASSED/FAILED + RMS tracking error) is
 *      published in Status, and the speed telemetry lets the supervisor compare
 *      the measured speed against the commanded sine independently.
 *   message 7516 Status.1.0       -- speed, target, temperature, angular momentum,
 *      state, over-temperature trip count, self-test verdict. Published at 1 Hz
 *      and immediately after every state change: at zero wheel speed this (plus
 *      the heartbeat) is what proves the node is alive -- liveness never depends
 *      on the wheel rotating.
 *   message 7509 Heartbeat.1.0    -- health ladder + vendor_specific_status_code
 *      mirror the fault history: 1 trip=ADVISORY, 2 trips=CAUTION, active fault,
 *      3+ trips or a FAILED self-test = WARNING (switch to the backup wheel).
 *
 * Critical-temperature protection is enforced INSIDE this node: at >=80 C the
 * drive is cut (torque 0), commands are rejected, the trip counter increments
 * and the state latches until the wheel cools below 70 C. Frequently tripping
 * wheels are considered faulty -> the supervisor switches to the backup unit
 * (the momentum dump that follows is a spacecraft-level concern, not this node's).
 *
 * The wheel is a firmware plant model (no hardware is attached): momentum wheel
 * J = 5e-4 kg*m^2, max torque 5 mN*m -> 0..6000 rpm in ~63 s (spin-up on the
 * order of a minute, per the specification), thermal model with Newton cooling
 * and a zero-speed heat gain (locked-rotor style heating).
 *
 * Service transfers are received in the CAN RX0 IRQ, where libcanard must not
 * be re-entered: the request is copied into a lock-free SPSC queue and answered
 * from the main loop.
 */

#include "main.h"

#include <canard.h>
#include <node_getinfo.h>
#include <spacecraft/flywheel/CreateTorque_1_0.h>
#include <spacecraft/flywheel/SetSpeed_1_0.h>
#include <spacecraft/flywheel/StartSelfTest_1_0.h>
#include <spacecraft/flywheel/Status_1_0.h>
#include <uavcan/node/Heartbeat_1_0.h>

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

CAN_HandleTypeDef hcan1;

static canard_t             canard_inst;
#define CANARD_NODE_ID 53U /* fixed local node ID (Cyphal 7-bit, 1..127) */
static const uint16_t       SUBJECT_HEARTBEAT = 7509U;
static uint8_t                    hb_transfer_id;
static uint32_t                   hb_uptime_sec;
static canard_us_t                last_hb_us;
static uint8_t                    status_transfer_id;
static canard_us_t                last_status_us;

static uint32_t                   can_rx_count;
static uint32_t                   can_tx_count;

/* ---------------------------------------------------------
 * Simulated momentum-wheel plant and protection limits.
 * --------------------------------------------------------- */

#define RPM_TO_RAD_S        0.10471975512f /* 2*pi/60  */
#define RAD_S_TO_RPM        9.54929658551f /* 60/(2*pi) */

#define WHEEL_J             0.0005f    /* moment of inertia, kg*m^2 */
#define WHEEL_TAU_MAX       0.005f     /* max drive torque, N*m */
#define WHEEL_TAU_MAX_SQ    2.5e-5f    /* (N*m)^2, for the (tau/tau_max)^2 heat term */
#define WHEEL_MAX_RPM       6000.0f    /* max wheel speed, rpm */
#define WHEEL_OMEGA_MAX     628.3185f  /* WHEEL_MAX_RPM in rad/s */
#define WHEEL_ACCEL_MAX     10.0f      /* WHEEL_TAU_MAX / WHEEL_J, rad/s^2 */
#define WHEEL_KP            0.005f     /* speed-loop gain, N*m per rad/s (saturation band 1 rad/s) */
#define WHEEL_B_DRAG        4.0e-7f    /* viscous drag, N*m*s */
#define WHEEL_RPM_TOL       30.0f      /* target_reached tolerance, rpm */
#define WHEEL_T_AMB_C       25.0f      /* ambient temperature, degC */
#define WHEEL_T_CRIT_C      80.0f      /* critical temperature -> fault, degC */
#define WHEEL_T_RECOVER_C   70.0f      /* recovery temperature (hysteresis), degC */
#define WHEEL_HEAT_TAU      0.5f       /* degC/s at full torque at speed */
#define WHEEL_HEAT_FRIC     0.01f      /* degC/s at max speed without torque */
#define WHEEL_HEAT_STALL    4.0f       /* extra zero-speed heat gain: 1 + G*(...)^2 */
#define WHEEL_STALL_SPAN    300.0f     /* zero-speed heat window, rpm */
#define WHEEL_COOL          0.0015f    /* Newton cooling coefficient, 1/s */
#define WHEEL_TORQUE_MAX_S  120.0f     /* max CreateTorque duration, s */
#define SELFTEST_MAX_AMP    600.0f     /* max self-test amplitude, rpm */
#define SELFTEST_MAX_FREQ   0.2f       /* max self-test frequency, Hz */
#define SELFTEST_MAX_TIME   60.0f      /* max self-test duration, s */
#define SELFTEST_RMS_TOL    0.1f       /* pass threshold: RMS error <= 10% of amplitude */

/* Wheel state (all values owned by the main loop; the IRQ only fills the queue). */
static float    wheel_omega;       /* current speed, rad/s (signed) */
static float    wheel_target_rpm;  /* commanded target, rpm (0 while idle/faulted) */
static float    wheel_temp_c = WHEEL_T_AMB_C;
static float    torque_fixed;      /* CreateTorque: fixed torque while active, N*m */
static float    torque_left_s;     /* CreateTorque: remaining time, s */
static bool     fault_active;      /* latched over-temperature fault */
static bool     coast_after_fault; /* drive released after recovery until the next command */
static uint8_t  overtemp_count;    /* lifetime critical-temperature trips */
static bool     status_dirty;      /* publish Status on the next loop pass */

/* Self-test state; values follow spacecraft_flywheel_Status_1_0_SELFTEST_*. */
static uint8_t  selftest_state;
static float    selftest_amp_rpm;
static float    selftest_freq_hz;
static float    selftest_total_s;
static float    selftest_left_s;
static float    selftest_omega0;   /* speed around which the sine is driven */
static float    selftest_prev_target_rpm; /* speed target to resume after the test */
static float    selftest_sqerr;    /* accumulated squared tracking error */
static uint32_t selftest_samples;
static float    selftest_error_rpm;

/* ---------------------------------------------------------
 * Service request queue: SPSC ring, produced by the RX IRQ
 * callback (on_request) and consumed by process_requests()
 * in the main loop. Libcanard must not be re-entered from
 * the callback, so responses are deferred to the loop.
 * --------------------------------------------------------- */

#define REQ_QUEUE_LEN 4U
#define REQ_QUEUE_MASK (REQ_QUEUE_LEN - 1U)

typedef struct {
  uint8_t service; /* service port-ID: SetSpeed/CreateTorque/StartSelfTest */
  uint8_t client;  /* requesting node-ID */
  uint8_t tid;     /* request transfer-ID (responses reuse it) */
  uint8_t len;     /* payload bytes actually copied */
  uint8_t data[12]; /* largest request payload (StartSelfTest = 12 bytes) */
} request_msg_t;

static request_msg_t      req_queue[REQ_QUEUE_LEN];
static volatile uint8_t   req_head; /* written by the IRQ only */
static volatile uint8_t   req_tail; /* written by the main loop only */

static float wheel_rpm(void) { return wheel_omega * RAD_S_TO_RPM; }

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

  /* Order preservation -- the subtle part. libcanard hands out the frames of
   * one transfer strictly in order, but bxCAN arbitrates its three TX
   * mailboxes by CAN ID and, for equal IDs, by mailbox number (0 wins over
   * 1 wins over 2). HAL_CAN_AddTxMessage() always picks the *first free*
   * mailbox, so a frame loaded into mailbox 0/1 while an older frame of the
   * same transfer still sits in mailbox 2 is transmitted *ahead* of it --
   * e.g. a 5-frame Status transfer went out as F1,F2,F4,F5,F3. The receiver
   * then reassembles a corrupted payload, the v1 CRC residue check fails and
   * the whole transfer is silently dropped (this is why pycyphal never saw
   * Status while the frames were clearly on the wire).
   *
   * So: never queue a frame while another one is still in the hardware. With
   * at most one frame in flight there is nothing to reorder -- and this is
   * also what makes ARTE safe: a frame that lost arbitration holds its
   * mailbox until it finally wins, so "all three free" is exactly "nothing
   * of ours is on the wire". libcanard simply retries the same frame on the
   * next canard_poll() (tx_eject_pending() only advances its cursor on
   * success), and the transfer deadlines are 5 s, far longer than any
   * arbitration wait. */
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
 * Service request reception (IRQ context): copy into the
 * SPSC queue only; responses are sent from the main loop.
 * --------------------------------------------------------- */

/* canard.h contract: multi-frame transfers hand payload.origin (the
 * rx_payload block) over to the application and it must be released;
 * single-frame transfers arrive with origin.data == NULL. See the same
 * release_payload in STM32F103_CentralProcessor: leaking here would grow
 * the heap into the stack the same way. */
static void release_payload(canard_payload_t* const payload)
{
  if (payload->origin.data != NULL) {
    free(payload->origin.data);
    payload->origin.data = NULL;
    payload->origin.size = 0U;
  }
}

static void on_request(canard_subscription_t* self,
                       canard_us_t            timestamp,
                       canard_prio_t          priority,
                       uint_least8_t          source_node_id,
                       uint_least8_t          transfer_id,
                       canard_payload_t       payload) {
  (void)timestamp;
  (void)priority;
  const uint8_t  max_len = (uint8_t)sizeof(req_queue[0].data);
  const uint8_t  len =
      (uint8_t)((payload.view.size > max_len) ? max_len : payload.view.size);
  const uint8_t  next = (uint8_t)((req_head + 1U) & REQ_QUEUE_MASK);
  if ((next != req_tail) && (source_node_id <= CANARD_NODE_ID_MAX) && (len > 0U)) {
    request_msg_t* const m = &req_queue[req_head];
    m->service = (uint8_t)self->port_id;
    m->client  = (uint8_t)source_node_id;
    m->tid     = (uint8_t)transfer_id;
    m->len     = len;
    memcpy(m->data, payload.view.data, len);
    __DMB(); /* payload before head publication */
    req_head = next;
  } /* else: queue full or anonymous source -> dropped */
  release_payload(&payload);
}

static const canard_subscription_vtable_t request_vt = {.on_message = &on_request};

/* ---------------------------------------------------------
 * Plant model: torque application, rigid-body integration,
 * thermal model and critical-temperature protection.
 * --------------------------------------------------------- */

static void wheel_step(float dt) {
  float tau;

  if (fault_active) {
    tau = 0.0f; /* over-temperature: the wheel is not driven, it just coasts */
  } else if (selftest_state == spacecraft_flywheel_Status_1_0_SELFTEST_RUNNING) {
    /* Commanded sine around the speed captured at self-test start. */
    const float t_el = selftest_total_s - selftest_left_s + dt;
    const float ref  = selftest_omega0 + selftest_amp_rpm * RPM_TO_RAD_S *
                                             sinf(6.28318530718f * selftest_freq_hz * t_el);
    tau = WHEEL_KP * (ref - wheel_omega);
    if (tau > WHEEL_TAU_MAX) {
      tau = WHEEL_TAU_MAX;
    } else if (tau < -WHEEL_TAU_MAX) {
      tau = -WHEEL_TAU_MAX;
    }
  } else if (torque_left_s > 0.0f) {
    /* Speed limit: the actuator must never drive the wheel past its rated max
     * speed, so the accelerating half of an impulse is cut there (a torque
     * that slows the wheel down is still fine). The impulse window keeps
     * ticking down either way and Status.at_max_speed reports the condition. */
    if ((fabsf(wheel_omega) >= WHEEL_OMEGA_MAX) && ((torque_fixed * wheel_omega) > 0.0f)) {
      tau = 0.0f;
    } else {
      tau = torque_fixed;
    }
    torque_left_s -= dt;
    if (torque_left_s < 0.0f) {
      torque_left_s = 0.0f;
    }
  } else if (coast_after_fault) {
    /* Post-recovery coast. The wheel may still be spinning at several thousand
     * RPM while the stator is the thing that just overheated; re-engaging the
     * speed loop with target 0 would brake at full torque, i.e. feed
     * (tau/tau_max)^2 heat right back over the trip threshold, re-latch the
     * fault and flap forever. Leave the drive released until the operator
     * issues the next command (SetSpeed/CreateTorque/StartSelfTest clear this
     * flag) and let the wheel spin down on viscous drag alone. */
    tau = 0.0f;
  } else {
    const float ref = wheel_target_rpm * RPM_TO_RAD_S;
    tau = WHEEL_KP * (ref - wheel_omega);
    if (tau > WHEEL_TAU_MAX) {
      tau = WHEEL_TAU_MAX;
    } else if (tau < -WHEEL_TAU_MAX) {
      tau = -WHEEL_TAU_MAX;
    }
  }

  /* Rigid body: J * dw/dt = tau - b * w. */
  wheel_omega += (tau - WHEEL_B_DRAG * wheel_omega) * (dt / WHEEL_J);

  /* Thermal model:
   *   dT/dt = H_tau*(tau/tau_max)^2 * G(w) + H_fric*(w/w_max)^2 - k*(T - T_amb),
   * where G = 1 + G_stall*max(0, 1 - |rpm|/span)^2 adds the zero-speed (locked
   * rotor) heating. Tuned so that a normal spin-up ends around 55-60 degC while
   * sustained full torque trips the 80 degC protection in about a minute. */
  const float rpm_abs = fabsf(wheel_omega * RAD_S_TO_RPM);
  float       stall   = 1.0f - rpm_abs / WHEEL_STALL_SPAN;
  if (stall < 0.0f) {
    stall = 0.0f;
  }
  const float heat =
      WHEEL_HEAT_TAU * (tau * tau / WHEEL_TAU_MAX_SQ) * (1.0f + WHEEL_HEAT_STALL * stall * stall) +
      WHEEL_HEAT_FRIC * (wheel_omega * wheel_omega) / (WHEEL_OMEGA_MAX * WHEEL_OMEGA_MAX);
  wheel_temp_c += (heat - WHEEL_COOL * (wheel_temp_c - WHEEL_T_AMB_C)) * dt;

  /* Critical-temperature protection: self-contained, does not rely on the
   * supervisor. Trip -> cut the drive, reject commands, count the trip;
   * recovery with hysteresis once the wheel cools below the recover limit. */
  if (wheel_temp_c >= WHEEL_T_CRIT_C) {
    if (!fault_active) {
      fault_active     = true;
      wheel_target_rpm = 0.0f;
      torque_fixed     = 0.0f;
      torque_left_s    = 0.0f;
      coast_after_fault = true;
      if (overtemp_count < 255U) {
        overtemp_count++;
      }
      /* A fault aborts a running self-test: an aborted test reports as not-run
       * (Status.SELFTEST_IDLE), as documented in the DSDL. */
      if (selftest_state == spacecraft_flywheel_Status_1_0_SELFTEST_RUNNING) {
        selftest_state = spacecraft_flywheel_Status_1_0_SELFTEST_IDLE;
      }
      status_dirty = true;
    }
  } else if (fault_active && (wheel_temp_c < WHEEL_T_RECOVER_C)) {
    fault_active = false;
    status_dirty = true;
  }

  /* Self-test progress and verdict (runs only if the test was not aborted above). */
  if (selftest_state == spacecraft_flywheel_Status_1_0_SELFTEST_RUNNING) {
    const float t_el = selftest_total_s - selftest_left_s + dt;
    const float ref  = selftest_omega0 + selftest_amp_rpm * RPM_TO_RAD_S *
                                             sinf(6.28318530718f * selftest_freq_hz * t_el);
    const float err  = wheel_omega - ref;
    selftest_sqerr += err * err;
    selftest_samples++;
    selftest_left_s -= dt;
    if (selftest_left_s <= 0.0f) {
      const float rms = sqrtf(selftest_sqerr / (float)selftest_samples);
      selftest_error_rpm = rms * RAD_S_TO_RPM;
      selftest_state =
          (rms <= SELFTEST_RMS_TOL * selftest_amp_rpm * RPM_TO_RAD_S)
              ? spacecraft_flywheel_Status_1_0_SELFTEST_PASSED
              : spacecraft_flywheel_Status_1_0_SELFTEST_FAILED;
      /* Resume the speed the wheel was holding before the test: a functional
       * self-test must not dump the stored momentum of a running wheel. (A
       * fault abort skips this path and keeps target 0 -- correct, the drive
       * is cut.) */
      wheel_target_rpm = selftest_prev_target_rpm;
      status_dirty = true;
    }
  }
}

/* Estimated seconds to reach target_rpm with the speed loop (accelerating at
 * slightly less than WHEEL_ACCEL_MAX to cover drag, plus a settle margin). */
static uint32_t eta_seconds_to(float target_rpm) {
  const float d_omega = fabsf((target_rpm - wheel_rpm()) * RPM_TO_RAD_S);
  if (d_omega <= WHEEL_RPM_TOL * RPM_TO_RAD_S) {
    return 0U;
  }
  return (uint32_t)(d_omega / (0.9f * WHEEL_ACCEL_MAX) + 1.5f);
}

static void send_response(uint16_t       service_id,
                          uint_least8_t  client,
                          uint_least8_t  tid,
                          const uint8_t* buf,
                          size_t         sz,
                          canard_us_t    now) {
  const canard_bytes_chain_t chain = {
      .bytes = {.size = sz, .data = buf},
      .next  = NULL,
  };
  (void)canard_respond(&canard_inst, now + 5000000, canard_prio_nominal, service_id, client, tid,
                       chain, NULL);
}

/* ---------------------------------------------------------
 * Service request handling (main loop).
 * --------------------------------------------------------- */

static void handle_request(const request_msg_t* r) {
  const canard_us_t now = canard_now_cb(NULL);
  uint8_t           buf[16];
  status_dirty = true; /* every response is followed by fresh telemetry */

  if (r->service == spacecraft_flywheel_SetSpeed_1_0_FIXED_PORT_ID_) {
    spacecraft_flywheel_SetSpeed_Request_1_0  req;
    spacecraft_flywheel_SetSpeed_Response_1_0 resp = {0};
    size_t sz = r->len;
    if (spacecraft_flywheel_SetSpeed_Request_1_0_deserialize_(&req, r->data, &sz) < 0) {
      return;
    }

    float target = req.target_rpm;
    const bool clamped = (fabsf(target) > WHEEL_MAX_RPM);
    if (target > WHEEL_MAX_RPM) {
      target = WHEEL_MAX_RPM;
    } else if (target < -WHEEL_MAX_RPM) {
      target = -WHEEL_MAX_RPM;
    }

    if (fault_active) {
      resp.outcome = spacecraft_flywheel_SetSpeed_Response_1_0_OUTCOME_REJECTED_FAULT;
    } else if (selftest_state == spacecraft_flywheel_Status_1_0_SELFTEST_RUNNING) {
      resp.outcome = spacecraft_flywheel_SetSpeed_Response_1_0_OUTCOME_REJECTED_BUSY;
    } else {
      wheel_target_rpm   = target;
      coast_after_fault  = false; /* an explicit command re-enables the drive */
      const bool at_max_now = (fabsf(wheel_rpm()) >= 0.99f * WHEEL_MAX_RPM);
      resp.outcome =
          (clamped || (at_max_now && (fabsf(target) >= 0.99f * WHEEL_MAX_RPM)))
              ? spacecraft_flywheel_SetSpeed_Response_1_0_OUTCOME_AT_MAX_SPEED
              : spacecraft_flywheel_SetSpeed_Response_1_0_OUTCOME_ACCEPTED;
    }

    resp.current_rpm   = wheel_rpm();
    resp.target_reached = (fabsf(resp.current_rpm - target) <= WHEEL_RPM_TOL);
    resp.eta_seconds =
        ((resp.outcome == spacecraft_flywheel_SetSpeed_Response_1_0_OUTCOME_REJECTED_FAULT) ||
         (resp.outcome == spacecraft_flywheel_SetSpeed_Response_1_0_OUTCOME_REJECTED_BUSY) ||
         resp.target_reached)
            ? 0U
            : eta_seconds_to(target);

    size_t out_sz = sizeof(buf);
    if (spacecraft_flywheel_SetSpeed_Response_1_0_serialize_(&resp, buf, &out_sz) >= 0) {
      send_response(r->service, r->client, r->tid, buf, out_sz, now);
    }
  } else if (r->service == spacecraft_flywheel_CreateTorque_1_0_FIXED_PORT_ID_) {
    spacecraft_flywheel_CreateTorque_Request_1_0  req;
    spacecraft_flywheel_CreateTorque_Response_1_0 resp = {0};
    size_t sz = r->len;
    if (spacecraft_flywheel_CreateTorque_Request_1_0_deserialize_(&req, r->data, &sz) < 0) {
      return;
    }

    float torque = req._torque;
    float dur    = req.duration;
    bool  clamped = false;
    if (fabsf(torque) > WHEEL_TAU_MAX) {
      clamped = true;
      torque  = (torque > 0.0f) ? WHEEL_TAU_MAX : -WHEEL_TAU_MAX;
    }
    if (!(dur > 0.0f)) {
      clamped = true;
      dur     = 0.0f;
    } else if (dur > WHEEL_TORQUE_MAX_S) {
      clamped = true;
      dur     = WHEEL_TORQUE_MAX_S;
    }

    if (fault_active) {
      resp.outcome = spacecraft_flywheel_CreateTorque_Response_1_0_OUTCOME_REJECTED_FAULT;
    } else if (selftest_state == spacecraft_flywheel_Status_1_0_SELFTEST_RUNNING) {
      resp.outcome = spacecraft_flywheel_CreateTorque_Response_1_0_OUTCOME_REJECTED_BUSY;
    } else {
      if (dur > 0.0f) {
        torque_fixed  = torque;
        torque_left_s = dur;
      }
      coast_after_fault = false; /* an explicit command re-enables the drive */
      resp.outcome = clamped ? spacecraft_flywheel_CreateTorque_Response_1_0_OUTCOME_CLAMPED
                             : spacecraft_flywheel_CreateTorque_Response_1_0_OUTCOME_ACCEPTED;
      resp.applied_torque    = torque;
      resp.applied_duration  = dur;
      resp.expected_delta_rpm = (torque * dur / WHEEL_J) * RAD_S_TO_RPM;
    }

    size_t out_sz = sizeof(buf);
    if (spacecraft_flywheel_CreateTorque_Response_1_0_serialize_(&resp, buf, &out_sz) >= 0) {
      send_response(r->service, r->client, r->tid, buf, out_sz, now);
    }
  } else if (r->service == spacecraft_flywheel_StartSelfTest_1_0_FIXED_PORT_ID_) {
    spacecraft_flywheel_StartSelfTest_Request_1_0  req;
    spacecraft_flywheel_StartSelfTest_Response_1_0 resp = {0};
    size_t sz = r->len;
    if (spacecraft_flywheel_StartSelfTest_Request_1_0_deserialize_(&req, r->data, &sz) < 0) {
      return;
    }

    if (fault_active) {
      resp.outcome = spacecraft_flywheel_StartSelfTest_Response_1_0_OUTCOME_REJECTED_FAULT;
    } else if (selftest_state == spacecraft_flywheel_Status_1_0_SELFTEST_RUNNING) {
      resp.outcome = spacecraft_flywheel_StartSelfTest_Response_1_0_OUTCOME_REJECTED_BUSY;
    } else if (!(req.amplitude_rpm > 0.0f) || !(req.frequency_hz > 0.0f) ||
               !(req.duration > 0.0f)) {
      resp.outcome = spacecraft_flywheel_StartSelfTest_Response_1_0_OUTCOME_REJECTED_BAD_PARAMS;
    } else {
      selftest_amp_rpm  = (req.amplitude_rpm > SELFTEST_MAX_AMP) ? SELFTEST_MAX_AMP
                                                                 : req.amplitude_rpm;
      selftest_freq_hz  = (req.frequency_hz > SELFTEST_MAX_FREQ) ? SELFTEST_MAX_FREQ
                                                                 : req.frequency_hz;
      /* The clamps above are per-parameter; the *product* A*f^2 is what the
       * hardware has to follow, so bound that too or the test fails by
       * construction rather than by finding a real fault:
       *  (a) actuator authority -- peak sine acceleration A*(2*pi*f)^2 must stay
       *      below half of tau_max/J, else the drive saturates near the sine
       *      peaks and the tracking error blows past the RMS tolerance;
       *  (b) loop bandwidth -- the P-loop tracks a sine with a relative error of
       *      ~w/sqrt(w_bw^2 + w^2) (w_bw = WHEEL_KP/WHEEL_J = 10 rad/s), so even
       *      a perfectly actuated sine fails the 10% RMS budget above ~0.16 Hz.
       * Each limit budgets half of SELFTEST_RMS_TOL so the verdict keeps margin.
       * The response echoes the frequency actually used. */
      {
        const float amp_rad_s = selftest_amp_rpm * RPM_TO_RAD_S;
        const float budget    = 0.5f * SELFTEST_RMS_TOL; /* 0.05 of amplitude */
        const float f_authority =
            sqrtf(0.5f * (WHEEL_ACCEL_MAX / amp_rad_s)) / 6.28318530718f;
        const float f_bandwidth =
            ((WHEEL_KP / WHEEL_J) * budget / sqrtf(1.0f - budget * budget)) /
            6.28318530718f;
        if (selftest_freq_hz > f_authority) {
          selftest_freq_hz = f_authority;
        }
        if (selftest_freq_hz > f_bandwidth) {
          selftest_freq_hz = f_bandwidth;
        }
      }
      selftest_total_s  = (req.duration > SELFTEST_MAX_TIME) ? SELFTEST_MAX_TIME : req.duration;
      selftest_left_s   = selftest_total_s;
      selftest_omega0   = wheel_omega; /* sine around the current speed */
      selftest_sqerr    = 0.0f;
      selftest_samples  = 0U;
      selftest_error_rpm = 0.0f;
      selftest_state    = spacecraft_flywheel_Status_1_0_SELFTEST_RUNNING;
      selftest_prev_target_rpm = wheel_target_rpm;
      wheel_target_rpm  = 0.0f; /* the speed loop is suspended during the test */
      coast_after_fault = false; /* the test drives the wheel itself */
      torque_fixed      = 0.0f;
      torque_left_s     = 0.0f;
      resp.outcome        = spacecraft_flywheel_StartSelfTest_Response_1_0_OUTCOME_ACCEPTED;
      resp.amplitude_rpm  = selftest_amp_rpm;
      resp.frequency_hz   = selftest_freq_hz;
      resp.test_seconds   = (uint32_t)selftest_total_s;
    }

    size_t out_sz = sizeof(buf);
    if (spacecraft_flywheel_StartSelfTest_Response_1_0_serialize_(&resp, buf, &out_sz) >= 0) {
      send_response(r->service, r->client, r->tid, buf, out_sz, now);
    }
  }
}

static void process_requests(void) {
  while (req_tail != req_head) {
    const request_msg_t r = req_queue[req_tail]; /* copy while the slot is owned */
    __DMB();
    req_tail = (uint8_t)((req_tail + 1U) & REQ_QUEUE_MASK);
    handle_request(&r);
  }
}

/* ---------------------------------------------------------
 * spacecraft.flywheel.Status.1.0 telemetry: 1 Hz + on change.
 * --------------------------------------------------------- */

static void publish_status(canard_us_t now) {
  spacecraft_flywheel_Status_1_0 st = {0};
  st.speed_rpm   = wheel_rpm();
  st.target_rpm  = wheel_target_rpm;
  st.temperature = wheel_temp_c;
  st.momentum    = wheel_omega * WHEEL_J; /* L = J * w, N*m*s */
  st.overtemp_count  = overtemp_count;
  st.target_reached  = (fabsf(st.speed_rpm - st.target_rpm) <= WHEEL_RPM_TOL);
  st.at_max_speed    = (fabsf(st.speed_rpm) >= 0.99f * WHEEL_MAX_RPM);
  st.selftest_state  = selftest_state;
  st.selftest_error_rpm = selftest_error_rpm;
  if (fault_active) {
    st.state = spacecraft_flywheel_Status_1_0_STATE_FAULT_OVERTEMP;
  } else if (selftest_state == spacecraft_flywheel_Status_1_0_SELFTEST_RUNNING) {
    st.state = spacecraft_flywheel_Status_1_0_STATE_SELFTEST;
  } else if ((fabsf(wheel_target_rpm) > WHEEL_RPM_TOL) || (torque_left_s > 0.0f)) {
    st.state = spacecraft_flywheel_Status_1_0_STATE_SPINNING;
  } else {
    st.state = spacecraft_flywheel_Status_1_0_STATE_IDLE;
  }

  uint8_t    buf[32];
  size_t     sz = sizeof(buf);
  if (spacecraft_flywheel_Status_1_0_serialize_(&st, buf, &sz) >= 0) {
    const canard_bytes_chain_t chain = {
        .bytes = {.size = sz, .data = buf},
        .next  = NULL,
    };
    (void)canard_publish_13b(&canard_inst, now + 5000000, 1U, canard_prio_nominal,
                             spacecraft_flywheel_Status_1_0_FIXED_PORT_ID_, status_transfer_id++,
                             chain, NULL);
  }
  last_status_us = now;
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
      .name = "canbus4bluepill.flywheel",
      .unique_id = NULL,
  };
  if (!node_getinfo_init(&canard_inst, &getinfo_cfg)) {
    Error_Handler();
  }

  /* Service request subscriptions (one shared callback; the port-ID identifies
     the service). Extent covers the largest request; the transfer-ID timeout
     satisfies the deduplication requirement for unicast transfers. */
  static canard_subscription_t sub_setspeed;
  static canard_subscription_t sub_createtorque;
  static canard_subscription_t sub_selftest;
  if ((canard_subscribe_request(&canard_inst, &sub_setspeed,
                                spacecraft_flywheel_SetSpeed_1_0_FIXED_PORT_ID_, 64U, 5000000U,
                                &request_vt) == NULL) ||
      (canard_subscribe_request(&canard_inst, &sub_createtorque,
                                spacecraft_flywheel_CreateTorque_1_0_FIXED_PORT_ID_, 64U, 5000000U,
                                &request_vt) == NULL) ||
      (canard_subscribe_request(&canard_inst, &sub_selftest,
                                spacecraft_flywheel_StartSelfTest_1_0_FIXED_PORT_ID_, 64U, 5000000U,
                                &request_vt) == NULL)) {
    Error_Handler();
  }

  if (HAL_CAN_Start(&hcan1) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
    Error_Handler();
  }

  last_hb_us    = canard_now_cb(NULL);
  status_dirty  = true; /* telemetry on the very first loop pass */
  canard_us_t last_step_us = last_hb_us;

  while (1) {
    const canard_us_t now = canard_now_cb(NULL);

    /* Publish uavcan.node.Heartbeat.1.0 every second. Health reflects the wheel
       fault history; vendor_specific_status_code carries the trip counter. */
    if ((now - last_hb_us) >= 1000000) {
      uavcan_node_Heartbeat_1_0 hb = {0};
      hb.uptime   = hb_uptime_sec++;
      hb.mode.value = uavcan_node_Mode_1_0_OPERATIONAL; /* the node itself is fine */
      uint8_t health = uavcan_node_Health_1_0_NOMINAL;
      if (overtemp_count >= 1U) {
        health = uavcan_node_Health_1_0_ADVISORY;
      }
      if (overtemp_count >= 2U) {
        health = uavcan_node_Health_1_0_CAUTION;
      }
      if (fault_active || (overtemp_count >= 3U) ||
          (selftest_state == spacecraft_flywheel_Status_1_0_SELFTEST_FAILED)) {
        health = uavcan_node_Health_1_0_WARNING;
      }
      hb.health.value = health;
      hb.vendor_specific_status_code = overtemp_count;

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

    /* spacecraft.flywheel.Status.1.0: 1 Hz heartbeat of the wheel itself plus
       an immediate update whenever a service response or state change occurred. */
    if (status_dirty || ((now - last_status_us) >= 1000000)) {
      publish_status(now);
      status_dirty = false;
    }

    /* Advance the plant model every 10 ms (dt-capped for correctness after
       transient stalls, e.g. while debugging). */
    if ((now - last_step_us) >= 10000U) {
      float dt = (float)(now - last_step_us) / 1000000.0f;
      if (dt > 0.25f) {
        dt = 0.25f;
      }
      wheel_step(dt);
      last_step_us = now;
    }

    /* Answer queued service requests (the RX callback cannot send from IRQ). */
    process_requests();
    node_getinfo_poll(&canard_inst);

    /* Drain the TX pipeline. canard_tx_cb() returns false while a frame of
     * ours is still in a mailbox -- deliberately, so that only one frame is
     * ever in the hardware and bxCAN cannot reorder the frames of a transfer
     * (see the comment there). With ARTE such a frame may sit in its mailbox
     * for a few bit times while it waits to win arbitration; libcanard
     * retries the same frame on the next canard_poll()
     * (tx_eject_pending() advances its cursor only on success). */
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
