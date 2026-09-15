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
 * L: Reserved; do not modify
 * ...
 *
 * USART1 (115200 baud, debug output):
 *   PA9  - USART1_TX  (AF push-pull)
 *   PA10 - USART1_RX  (input floating)
 *
 * I2C1 (400 kHz) - MPU-9250/6500 IMU AND 0.91" SSD1306 OLED (shared bus):
 *   PB6  - I2C1_SCL   (AF open-drain)
 *   PB7  - I2C1_SDA   (AF open-drain)
 *     -> MPU-9250 on 0x68, OLED SSD1306 on 0x3C
 *
 * GPIO:
 *   PC13 - On-board LED (active low, output push-pull)
 *
 * Power:
 *   3V3 / GND - supply to MPU and OLED modules
 *  ============================================================ */

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
