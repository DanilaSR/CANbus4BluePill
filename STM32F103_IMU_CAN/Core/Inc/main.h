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
 *   PA11 - CAN1_RX (AF input, default pins, no remap)
 *   PA12 - CAN1_TX (AF push-pull, default pins, no remap)
 *     -> external CAN transceiver (TJA1050 etc.), 120R termination
 *
 * I2C1 (100 kHz) - MPU-9250/6500 IMU AND 0.91" SSD1306 OLED (shared bus):
 *   PB6  - I2C1_SCL   (AF open-drain)
 *   PB7  - I2C1_SDA   (AF open-drain)
 *     -> MPU-9250 on 0x68, OLED SSD1306 on 0x3C
 *
 * USART1 (115200 baud, CSV stream for visualize.py):
 *   PA9  - USART1_TX  (AF push-pull)
 *   PA10 - USART1_RX  (input floating)
 *
 * GPIO:
 *   PC13 - On-board LED (active low, output push-pull)
 *
 * Power:
 *   3V3 / GND - supply to MPU and OLED modules
 *  ============================================================ */

/* CSV/OLED/Cyphal report cadence: 100 ms = 10 Hz (MPU read + publish) */
#define IMU_REPORT_INTERVAL_MS 100U

/* USART1 baud rate for the CSV stream consumed by visualize.py */
#define CSV_BAUDRATE           115200U

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */