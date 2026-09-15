/**
  ******************************************************************************
  * @file    main.h
  * @brief   Header for STM32L100RCT6 + STLM75DS2F + Flipper Zero project.
  *
  *  Hardware mapping:
  *    - I2C1:   PB6=SCL, PB7=SDA  -> STLM75DS2F temperature sensor (addr 0x48)
  *    - USART2: PA2=TX,  PA3=RX   -> Flipper Zero (115200 8N1)
  *    - LED:    PC13              -> Nucleo on-board LED (optional status)
  ******************************************************************************
  */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l1xx_hal.h"

/* ---- STLM75DS2F I2C ---- */
#define STLM75_I2C             hi2c1
#define STLM75_I2C_ADDR        (0x48U << 1)   /* 7-bit addr shifted for HAL */

/* ---- USART to Flipper Zero ---- */
#define FLIPPER_UART           huart2
#define FLIPPER_BAUDRATE       115200U

/* ---- Status LED ---- */
#define LED_GPIO_PORT          GPIOC
#define LED_GPIO_PIN           GPIO_PIN_13

/* ---- Application timing ---- */
#define TEMP_REPORT_INTERVAL_MS   1000U   /* report temperature every 1 s */

/* Function prototypes */
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
