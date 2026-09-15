/**
 ******************************************************************************
 * @file    main.c
 * @brief   STM32L100RCT6 reads STLM75DS2F temperature sensor over I2C and
 *          reports it over USART2 to a Flipper Zero at 115200 8N1.
 *
 *  Message format (newline-terminated ASCII):
 *      TEMP:<value>\n     STM32 -> Flipper (sent every second)
 *      PING\n             Flipper -> STM32 (STM32 replies PONG\n)
 *      READ\n             Flipper -> STM32 (STM32 replies TEMP:...\n on next
 * tick)
 ******************************************************************************
 */

#include "main.h"
#include "stlm75.h"
#include "stm32l1xx_it.h"
#include "uart_protocol.h"

/* Private variables -------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

static float current_temp = 0.0f;

/* Private function prototypes ---------------------------------------------*/
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);

/**
 * @brief  The application entry point.
 */
int main(void) {
  /* Reset of all peripherals, initialises the Flash interface and the SysTick
   */
  HAL_Init();

  /* Configure the system clock (HSI 16MHz) */
  SystemClock_Config();

  /* Initialise all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();

  /* Start UART RX line collection */
  UART_Protocol_Init(&huart2);

  /* Initialise the STLM75 temperature sensor.
     The device address depends on the A2/A1/A0 strapping on the board, so
     scan the whole range and use whatever actually ACKs. */
  uint16_t stlm75_addr = STLM75_ScanAddress(&hi2c1);
  uint8_t sensor_ok = 0;
  if (stlm75_addr != 0U) {
    sensor_ok = 1;
  }

  /* Blink LED once to indicate startup */
  HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_RESET);
  HAL_Delay(50U);
  HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_SET);

  if (sensor_ok) {
    UART_Protocol_SendAddr(stlm75_addr >> 1);
  } else {
    UART_Protocol_SendError();
  }

  uint32_t last_beat = HAL_GetTick();
  uint32_t last_report = HAL_GetTick();

  /* Infinite loop */
  while (1) {
    /* Handle any incoming commands from the Flipper */
    UART_Protocol_Poll();

    /* Feed the on-board watchdog... none here. Poll ticks. */

    /* Heartbeat every 250 ms, proof the firmware is alive */
    if ((HAL_GetTick() - last_beat) >= 250U) {
      last_beat = HAL_GetTick();
      HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
    }

    /* Periodically read the sensor and report it */
    if ((HAL_GetTick() - last_report) >= TEMP_REPORT_INTERVAL_MS) {
      last_report = HAL_GetTick();

      if (sensor_ok &&
          (STLM75_ReadTemp(&hi2c1, stlm75_addr, &current_temp) == HAL_OK)) {
        UART_Protocol_SendTemp(current_temp);
      } else {
        /* Sensor not responding: report the I2C HAL error code for diagnosis */
        UART_Protocol_SendI2CError(hi2c1.ErrorCode);
      }
    }
  }
}

/**
 * @brief  System Clock Configuration (HSI 16MHz, no external crystal).
 */
static void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /* Enable HSI and disable HSE/MSI/PLL/LSE/LSI */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /* Select HSI as system clock source, no bus dividers */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief  GPIO Initialization: PC13 = LED output, REMAP I2C1/USART2 on PB6/PB7
 * and PA2/PA3.
 */
static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* Configure PC13 as output (status LED) */
  HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_SET);
  GPIO_InitStruct.Pin = LED_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);
}

/**
 * @brief  I2C1 Initialization on PB6 (SCL) / PB7 (SDA). 100 kHz.
 */
static void MX_I2C1_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* I2C1 clock enable */
  __HAL_RCC_I2C1_CLK_ENABLE();

  /* I2C1 GPIO configuration: PB6 -> SCL, PB7 -> SDA, AF4 */
  GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* I2C1 peripheral configuration at the STLM75's native 100kHz. */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000U;
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

/**
 * @brief  USART2 Initialization on PA2 (TX) / PA3 (RX). 115200 8N1.
 */
static void MX_USART2_UART_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* USART2 clock enable */
  __HAL_RCC_USART2_CLK_ENABLE();

  /* USART2 GPIO configuration: PA2 -> TX, PA3 -> RX, AF7 */
  GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USART2 peripheral configuration */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = FLIPPER_BAUDRATE;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;

  if (HAL_UART_Init(&huart2) != HAL_OK) {
    Error_Handler();
  }

  /* Enable the USART2 interrupt */
  HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART2_IRQn);
}

/**
 * @brief  This function is executed in case of error occurrence.
 */
void Error_Handler(void) {
  /* User may add here some code to report the error */
  __disable_irq();
  while (1) {
  }
}
