#include "main.h"
#include "mpu9250.h"
#include "ssd1306.h"
#include <stdio.h>
#include <string.h>

I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;
MPU9250_t imu;
SSD1306_t oled;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);

/* SysTick ISR: advance the HAL time base. Required for HAL_InitTick(),
 * HAL_Delay(), and every HAL timeout/timeout-based wait to work. */
void SysTick_Handler(void) { HAL_IncTick(); }

/* Float-to-fixed string without newlib %f (which is broken in this toolchain:
 * even a literal 12.34f prints "-0.00"). Scale by 10^dec with rounding, then
 * format as integers so only integer printf paths are used. */
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

int main(void) {
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();

  MPU9250_Data_t data;
  int mpu_ok = (MPU9250_Init(&imu, &hi2c1, MPU9250_ADDR) == HAL_OK);

  SSD1306_Init(&oled, &hi2c1);
  SSD1306_Clear(&oled);

  uint32_t tick = 0;
  while (1) {
    char buf[64];

    if (mpu_ok) {
      if (MPU9250_ReadRaw(&imu, &data) == HAL_OK) {
        MPU9250_Compute(&imu, &data);

        /* Stream CSV to UART: ax,ay,az,gx,gy,gz,temp (3 decimals) */
        fmt_f(buf, sizeof(buf), data.ax, 3);
        size_t csv_off = strlen(buf);
        buf[csv_off++] = ',';
        fmt_f(buf + csv_off, sizeof(buf) - csv_off, data.ay, 3);
        csv_off += strlen(buf + csv_off);
        buf[csv_off++] = ',';
        fmt_f(buf + csv_off, sizeof(buf) - csv_off, data.az, 3);
        csv_off += strlen(buf + csv_off);
        buf[csv_off++] = ',';
        fmt_f(buf + csv_off, sizeof(buf) - csv_off, data.gx, 3);
        csv_off += strlen(buf + csv_off);
        buf[csv_off++] = ',';
        fmt_f(buf + csv_off, sizeof(buf) - csv_off, data.gy, 3);
        csv_off += strlen(buf + csv_off);
        buf[csv_off++] = ',';
        fmt_f(buf + csv_off, sizeof(buf) - csv_off, data.gz, 3);
        csv_off += strlen(buf + csv_off);
        buf[csv_off++] = ',';
        fmt_f(buf + csv_off, sizeof(buf) - csv_off, data.temp, 3);
        csv_off += strlen(buf + csv_off);
        buf[csv_off++] = '\r';
        buf[csv_off++] = '\n';
        buf[csv_off] = '\0';
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), 100);

        /* OLED: mirror the UART CSV, split at comma boundaries into the
         * accel / gyro / temp groups (the 7-fields CSV won't fit a 128px
         * line of ~21 chars). Comma -> '\0' surgery is temporary. */
        char *g[7];
        int gi = 1;
        g[0] = buf;
        for (char *p = buf; *p && gi < 7; p++)
          if (*p == ',')
            g[gi++] = p + 1;
        char c3 = *(g[3] - 1), c6 = *(g[6] - 1);
        *(g[3] - 1) = '\0';
        *(g[6] - 1) = '\0';
        SSD1306_DrawString(&oled, 0, 0, buf, 1);
        SSD1306_DrawString(&oled, 0, 8, g[3], 1);
        SSD1306_DrawString(&oled, 0, 16, g[6], 1);
        *(g[3] - 1) = c3;
        *(g[6] - 1) = c6;
        snprintf(buf, sizeof(buf), "t=%lu", (unsigned long)tick);
        SSD1306_DrawString(&oled, 0, 24, buf, 1);
        SSD1306_Update(&oled);
      }
    }

    // HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); /* LED blinks => MCU alive */
    HAL_Delay(100);
    tick++;
  }
}

void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /* -------- Simplest, hang-free clock config: internal HSI, NO PLL --------
   * Enabling the PLL (from HSE or HSI) hangs on this board at
   * HAL_RCC_OscConfig. Running straight from HSI @ 8 MHz is fully
   * sufficient for I2C 400kHz, UART and the MPU/OLED. */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE; /* no PLL at all */
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI; /* 8 MHz direct */
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1; /* 8 MHz (<=36MHz ok) */
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1; /* 8 MHz */
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
    Error_Handler();
  }
}

static void MX_I2C1_Init(void) {
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed =
      100000; /* 100kHz standard mode (robust with module pull-ups) */
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
    Error_Handler();
  }
}

static void MX_USART1_UART_Init(void) {
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK) {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();

  /* PC13 = LED (active low: SET = off, RESET = on) */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); /* LED off */
}

void Error_Handler(void) {
  __disable_irq();
  while (1) {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {
  (void)file;
  (void)line;
}
#endif
