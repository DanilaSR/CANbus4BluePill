/* Temperature sensor unit (spacecraft unit 6/6) -- node-ID 56, Heartbeat.1.0 (7509)
 * + si.sample.temperature.Scalar.1.0 (7514).
 * Merged from the STM32F103_CAN template and STM32DISC_TEMP: reads an STLM75 over
 * I2C1 and reports it both as a Cyphal sample (subject 7514) and as ASCII over
 * USART2 to a Flipper Zero (TEMP:/PING/READ protocol).
 */

#include "main.h"
#include "stlm75.h"
#include "uart_protocol.h"

#include <canard.h>
#include <uavcan/node/Heartbeat_1_0.h>
#include <uavcan/si/sample/temperature/Scalar_1_0.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

CAN_HandleTypeDef hcan1;
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

static canard_t             canard_inst;
#define CANARD_NODE_ID 56U /* fixed local node ID (Cyphal 7-bit, 1..127) */
static const uint16_t       SUBJECT_HEARTBEAT = 7509U;
static uint8_t                    hb_transfer_id;
static uint32_t                   hb_uptime_sec;
static canard_us_t                last_hb_us;
static const uint16_t       SUBJECT_TEMPERATURE = 7514U;
static uint8_t                    data_transfer_id;
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

/* ---------------------------------------------------------
 * I2C1 init (PB6 SCL / PB7 SDA -- STLM75 temperature sensor).
 * --------------------------------------------------------- */

static void MX_I2C1_Init(void) {
  __HAL_RCC_I2C1_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* PB6/PB7 = I2C1 AF open-drain. The internal pull-ups are far too weak
     for a real I2C bus, so external ones (typically on the sensor board)
     are required; the internal ones stay enabled as a safety net. */
  GPIO_InitTypeDef gpio = {0};
  gpio.Pin   = GPIO_PIN_6 | GPIO_PIN_7;
  gpio.Mode  = GPIO_MODE_AF_OD;
  gpio.Pull  = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &gpio);

  hi2c1.Instance             = I2C1;
  hi2c1.Init.ClockSpeed      = 100000U; /* STLM75 native 100 kHz */
  hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1     = 0;
  hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2     = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
    Error_Handler();
  }
}

/* ---------------------------------------------------------
 * USART2 init (PA2 TX / PA3 RX -- Flipper Zero ASCII bridge).
 * --------------------------------------------------------- */

static void MX_USART2_UART_Init(void) {
  __HAL_RCC_USART2_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitTypeDef gpio = {0};
  gpio.Pin   = GPIO_PIN_2 | GPIO_PIN_3;
  gpio.Mode  = GPIO_MODE_AF_PP;
  gpio.Pull  = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &gpio);

  huart2.Instance          = USART2;
  huart2.Init.BaudRate     = FLIPPER_BAUDRATE;
  huart2.Init.WordLength   = UART_WORDLENGTH_8B;
  huart2.Init.StopBits     = UART_STOPBITS_1;
  huart2.Init.Parity       = UART_PARITY_NONE;
  huart2.Init.Mode         = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK) {
    Error_Handler();
  }

  /* RX interrupt feeds UART_Protocol_HAL_RxCpltCallback (see it.c). */
  HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART2_IRQn);
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
  MX_I2C1_Init();
  MX_USART2_UART_Init();

  /* Start UART line reception (PING/READ commands from the Flipper). */
  UART_Protocol_Init(&huart2);

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

  if (HAL_CAN_Start(&hcan1) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
    Error_Handler();
  }

  /* Scan for the STLM75: the I2C address depends on the A2/A1/A0 strapping,
     so probe the whole 0x48..0x4F range and use whatever ACKs. Without a
     sensor on the bus this unit keeps heartbeating but stays silent on
     subject 7514 -- wire the sensor and power-cycle to go live. */
  const uint16_t stlm75_addr = STLM75_ScanAddress(&hi2c1);
  const uint8_t  sensor_ok   = (stlm75_addr != 0U) ? 1U : 0U;
  if (sensor_ok) {
    UART_Protocol_SendAddr(stlm75_addr >> 1); /* ADDR:0xNN to the Flipper */
  } else {
    UART_Protocol_SendError(); /* ERR to the Flipper, once, at boot */
  }

  /* One visible LED pulse to mark startup (the 1 Hz heartbeat toggle takes
     over from here). */
  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
  HAL_Delay(50U);
  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

  last_hb_us   = canard_now_cb(NULL);
  last_data_us = canard_now_cb(NULL);

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

    /* Every TEMP_REPORT_INTERVAL_MS: read the STLM75 and report it twice --
     * as a Cyphal sample on subject 7514 (kelvin) and to the Flipper over
     * USART2 (TEMP:<celsius>). If no sensor was found at boot (sensor_ok ==
     * 0) this stays silent: heartbeat only until one is wired and the board
     * is power-cycled. Sample timestamp stays 0 (no network time sync). */
    if ((now - last_data_us) >= ((canard_us_t)TEMP_REPORT_INTERVAL_MS * 1000U)) {
      last_data_us = now;

      float temp_c = 0.0f;
      if (sensor_ok && (STLM75_ReadTemp(&hi2c1, stlm75_addr, &temp_c) == HAL_OK)) {
        uavcan_si_sample_temperature_Scalar_1_0 sample = {0};
        sample.kelvin = temp_c + 273.15f;

        uint8_t      buf[32];
        size_t       sz = sizeof(buf);
        const int8_t err =
            uavcan_si_sample_temperature_Scalar_1_0_serialize_(&sample, buf, &sz);
        if (err >= 0) {
          const canard_bytes_chain_t chain = {
              .bytes = {.size = sz, .data = buf},
              .next  = NULL,
          };
          (void)canard_publish_13b(&canard_inst, now + 5000000, 1U, canard_prio_nominal,
                                   SUBJECT_TEMPERATURE, data_transfer_id++, chain, NULL);
        }
        UART_Protocol_SendTemp(temp_c);
      } else if (sensor_ok) {
        /* Sensor was found at boot but stopped ACKing: surface the I2C HAL
           error code on the UART for diagnosis. Nothing goes on the bus. */
        UART_Protocol_SendI2CError(hi2c1.ErrorCode);
      }
    }

    /* Drain the TX pipeline. canard_tx_cb() returns false while a frame of
     * ours is still in a mailbox (it may be waiting to win arbitration), so
     * only one frame is ever in the hardware and bxCAN cannot reorder the
     * frames of a transfer. libcanard retries the same frame on the next
     * canard_poll() (see tx_eject_pending). */
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
