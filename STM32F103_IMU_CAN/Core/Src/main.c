/* IMU unit (spacecraft unit 2/6) -- node-ID 52, Heartbeat.1.0 (7509) + si.sample.angular_velocity.Vector3.1.0 (7511) + si.sample.acceleration.Vector3.1.0 (7512).
 * Merged from STM32F103_IMU (MPU-9250 + SSD1306 + CSV stream), derived from the
 * STM32F103_CAN template. One I2C1 bus carries the MPU-9250 (0x68) and the
 * SSD1306 OLED (0x3C); USART1 streams CSV for visualize.py. The same 10 Hz
 * sample feeds all three sinks: Cyphal (SI units: rad/s, m/s^2), the OLED
 * mirror, and the CSV line (original driver units: g, deg/s, degC).
 * Without a sensor on the bus the unit heartbeats but stays silent.
 */

#include "main.h"

#include <canard.h>
#include <uavcan/node/Heartbeat_1_0.h>
#include <uavcan/si/sample/angular_velocity/Vector3_1_0.h>
#include <uavcan/si/sample/acceleration/Vector3_1_0.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "mpu9250.h"
#include "ssd1306.h"

CAN_HandleTypeDef hcan1;
I2C_HandleTypeDef  hi2c1;
UART_HandleTypeDef huart1;
MPU9250_t         imu;
SSD1306_t         oled;

static canard_t             canard_inst;
#define CANARD_NODE_ID 52U /* fixed local node ID (Cyphal 7-bit, 1..127) */
static const uint16_t       SUBJECT_HEARTBEAT = 7509U;
static uint8_t                    hb_transfer_id;
static uint32_t                   hb_uptime_sec;
static canard_us_t                last_hb_us;
static const uint16_t       SUBJECT_GYRO  = 7511U;
static const uint16_t       SUBJECT_ACCEL = 7512U;
static uint8_t                    data_gyro_tid;
static uint8_t                    data_accel_tid;
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
 * Float-to-fixed string without newlib %f (which is broken in this
 * toolchain: even a literal 12.34f prints "-0.00"). Scale by 10^dec with
 * rounding, then format as integers so only integer printf paths are used.
 * Copied verbatim from STM32F103_IMU.
 * --------------------------------------------------------- */

static void fmt_f(char *out, size_t n, float v, int dec) {
  static const float pw[4] = {1e0f, 1e1f, 1e2f, 1e3f};
  float s = (dec >= 0 && dec <= 3) ? pw[dec] : 100.0f;
  int32_t sc = (int32_t)(v * s + (v < 0.0f ? -0.5f : 0.5f));
  int32_t ip = sc / (int32_t)s;
  int32_t fp = sc % (int32_t)s;
  if (fp < 0)
    fp = -fp;
  if (dec > 0)
    snprintf(out, n, "%d.%0*d", (int)ip, dec, (int)fp);
  else
    snprintf(out, n, "%d", (int)ip);
}

/* Join three fmt_f values into "a,b,c" for the OLED lines. */
static void fmt_f3(char *out, size_t n, float a, float b, float c) {
  char ta[16], tb[16], tc[16];
  fmt_f(ta, sizeof(ta), a, 3);
  fmt_f(tb, sizeof(tb), b, 3);
  fmt_f(tc, sizeof(tc), c, 3);
  snprintf(out, n, "%s,%s,%s", ta, tb, tc);
}

/* I2C1 (PB6 SCL / PB7 SDA) -- MPU-9250 + SSD1306 share the bus. The GPIOs
 * are set up by HAL_I2C_MspInit() in stm32f1xx_hal_msp.c. */
static void MX_I2C1_Init(void) {
  hi2c1.Instance             = I2C1;
  hi2c1.Init.ClockSpeed      = 100000U;
  hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1     = 0;
  hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
    Error_Handler();
  }
}

/* USART1 (PA9 TX / PA10 RX) -- CSV stream for visualize.py (TX only). */
static void MX_USART1_UART_Init(void) {
  huart1.Instance          = USART1;
  huart1.Init.BaudRate     = CSV_BAUDRATE;
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
  MX_I2C1_Init();
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

  last_hb_us = canard_now_cb(NULL);

  /* MPU-9250 WHO_AM_I at boot: if the sensor is absent or fails to answer
     (mpu_ok == 0) the unit keeps heartbeating but publishes nothing on
     7511/7512 and emits no CSV or OLED traffic -- wire it and power-cycle
     to go live. The OLED is initialised unconditionally; with no display
     on the bus its writes just NACK and fail fast. */
  const int mpu_ok = (MPU9250_Init(&imu, &hi2c1, MPU9250_ADDR) == HAL_OK);
  SSD1306_Init(&oled, &hi2c1);
  SSD1306_Clear(&oled);

  uint32_t tick = 0;

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

    /* Every IMU_REPORT_INTERVAL_MS (100 ms = 10 Hz): read the MPU-9250 and
     * report the same sample three ways -- Cyphal subjects 7511/7512 in SI
     * units (rad/s, m/s^2), the CSV line on USART1 in the driver's original
     * units (g, deg/s, degC) for visualize.py, and the SSD1306 mirror.
     * Sample timestamps stay 0 (no network time sync in this demo).
     * A failed sensor keeps the unit on heartbeat-only until re-probed. */
    if ((now - last_data_us) >= ((canard_us_t)IMU_REPORT_INTERVAL_MS * 1000U)) {
      last_data_us = now;

      MPU9250_Data_t data;
      if (mpu_ok && (MPU9250_ReadRaw(&imu, &data) == HAL_OK)) {
        MPU9250_Compute(&imu, &data); /* accel [g], gyro [deg/s], temp [degC] */

        /* --- Cyphal: convert to SI units, publish 7511/7512 --- */
        uavcan_si_sample_angular_velocity_Vector3_1_0 gyro = {0};
        gyro.radian_per_second[0] = data.gx * (3.14159265358979f / 180.0f);
        gyro.radian_per_second[1] = data.gy * (3.14159265358979f / 180.0f);
        gyro.radian_per_second[2] = data.gz * (3.14159265358979f / 180.0f);

        uavcan_si_sample_acceleration_Vector3_1_0 acc = {0};
        acc.meter_per_second_per_second[0] = data.ax * 9.80665f;
        acc.meter_per_second_per_second[1] = data.ay * 9.80665f;
        acc.meter_per_second_per_second[2] = data.az * 9.80665f;

        uint8_t buf[32];
        size_t  sz = sizeof(buf);
        if (uavcan_si_sample_angular_velocity_Vector3_1_0_serialize_(&gyro, buf, &sz) >= 0) {
          const canard_bytes_chain_t chain = {
              .bytes = {.size = sz, .data = buf},
              .next  = NULL,
          };
          (void)canard_publish_13b(&canard_inst, now + 5000000, 1U, canard_prio_nominal,
                                   SUBJECT_GYRO, data_gyro_tid++, chain, NULL);
        }

        sz = sizeof(buf);
        if (uavcan_si_sample_acceleration_Vector3_1_0_serialize_(&acc, buf, &sz) >= 0) {
          const canard_bytes_chain_t chain = {
              .bytes = {.size = sz, .data = buf},
              .next  = NULL,
          };
          (void)canard_publish_13b(&canard_inst, now + 5000000, 1U, canard_prio_nominal,
                                   SUBJECT_ACCEL, data_accel_tid++, chain, NULL);
        }

        /* --- CSV line for visualize.py: "ax,ay,az,gx,gy,gz,temp\r\n" --- */
        const float v[7] = {data.ax, data.ay, data.az,
                            data.gx, data.gy, data.gz, data.temp};
        char   csv[80];
        size_t off = 0;
        for (int i = 0; i < 7; i++) {
          char num[16];
          fmt_f(num, sizeof(num), v[i], 3);
          off += (size_t)snprintf(csv + off, sizeof(csv) - off, "%s%s",
                                  i ? "," : "", num);
        }
        snprintf(csv + off, sizeof(csv) - off, "\r\n");
        HAL_UART_Transmit(&huart1, (uint8_t *)csv, (uint16_t)strlen(csv), 100U);

        /* --- SSD1306 mirror (original STM32F103_IMU layout) --- */
        /* 48 bytes per line: silences -Wformat-truncation (fmt_f3's worst
           case of three 15-byte fields) and matches DrawPixel's clipping
           when a full-scale gyro overflows the 21-char display width. */
        char l0[48], l1[48], l2[16], l3[16];
        fmt_f3(l0, sizeof(l0), data.ax, data.ay, data.az);
        fmt_f3(l1, sizeof(l1), data.gx, data.gy, data.gz);
        fmt_f(l2, sizeof(l2), data.temp, 3);
        snprintf(l3, sizeof(l3), "t=%lu", (unsigned long)tick);
        SSD1306_Clear(&oled);
        SSD1306_DrawString(&oled, 0, 0, l0, 1);
        SSD1306_DrawString(&oled, 0, 8, l1, 1);
        SSD1306_DrawString(&oled, 0, 16, l2, 1);
        SSD1306_DrawString(&oled, 0, 24, l3, 1);
        SSD1306_Update(&oled);
        tick++;
      }
      /* mpu_ok == 0 or read error: no publish, no CSV, no OLED update. */
    }

    /* Drain the TX pipeline. iface 0 is always "ready": with NART=1 every
     * one-shot frame frees its mailbox on its own (ACKed or not), so a failed
     * attempt can never wedge TX. canard_tx_cb() returns false only while all
     * three mailboxes are momentarily busy, and libcanard retries the same
     * frame on the next canard_poll() (see tx_eject_pending). */
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