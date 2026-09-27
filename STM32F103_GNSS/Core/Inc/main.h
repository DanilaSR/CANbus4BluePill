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
 * CAN1 (1 Mbit/s, Cyphal/UAVCAN via libcanard):
 *   PB8  - CAN1_RX (AF push-pull, remap CAN1_3)
 *   PB9  - CAN1_TX (AF push-pull, remap CAN1_3)
 *     -> external CAN transceiver (TJA1050 etc.), 120R termination
 *
 * GPIO:
 *   PC13 - On-board LED (active low, output push-pull)
 *  ============================================================ */

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */