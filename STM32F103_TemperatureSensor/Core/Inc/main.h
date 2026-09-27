#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

/* ============================================================
 * Pin configuration (STM32F103C8T6 Blue Pill)
 * ============================================================
 *
 * CAN1 (500 kbit/s, Cyphal/UAVCAN via libcanard):
 *   PA11 - CAN1_RX (input, pull-up; default pins, no remap)
 *   PA12 - CAN1_TX (AF push-pull)
 *     -> external CAN transceiver (TJA1050 etc.), 120R termination
 *
 * I2C1 (100 kHz, STLM75 temperature sensor):
 *   PB6  - I2C1_SCL (AF open-drain, pull-up; external pull-ups required)
 *   PB7  - I2C1_SDA (AF open-drain, pull-up; external pull-ups required)
 *
 * USART2 (115200 8N1, Flipper Zero ASCII bridge):
 *   PA2  - USART2_TX (AF push-pull)
 *   PA3  - USART2_RX (AF push-pull, pull-up)
 *
 * GPIO:
 *   PC13 - On-board LED (active low, output push-pull)
 *  ============================================================ */

/* Flipper Zero UART link speed and sensor read/report cadence. */
#define FLIPPER_BAUDRATE         115200U
#define TEMP_REPORT_INTERVAL_MS  1000U

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */