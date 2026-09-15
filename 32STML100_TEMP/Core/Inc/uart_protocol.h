/**
  ******************************************************************************
  * @file    uart_protocol.h
  * @brief   Simple newline-terminated ASCII protocol to/from Flipper Zero.
  *
  *  Conventions (Flipper community standard):
  *    - 115200 baud, 8N1
  *    - frames terminated with '\n'
  *    - STM32 -> Flipper:   TEMP:23.5\n
  *    - Flipper -> STM32:   PING\n  (STM32 replies PONG\n)
  *                          READ\n  (STM32 replies TEMP:<value>\n)
  ******************************************************************************
  */

#ifndef __UART_PROTOCOL_H
#define __UART_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l1xx_hal.h"
#include <stdint.h>

#define UART_PROTOCOL_RXBUF_SIZE    16U

/**
  * @brief  Initialize the UART receive (into the internal ring/line buffer).
  * @param  huart: pointer to UART handle.
  * @retval None.
  */
void UART_Protocol_Init(UART_HandleTypeDef *huart);

/**
  * @brief  Send a temperature reading to the Flipper (TEMP:<value>\n).
  * @param  temp: temperature in Celsius.
  * @retval None.
  */
void UART_Protocol_SendTemp(float temp);

/**
  * @brief  Send a heartbeat (BEAT\n). Use to prove the UART link is alive.
  * @retval None.
  */
void UART_Protocol_SendBeat(void);

/**
  * @brief  Send a sensor-error message (ERR\n).
  * @retval None.
  */
void UART_Protocol_SendError(void);

/**
  * @brief  Report the detected STLM75 I2C address (ADDR:0x%02x\n).
  * @param  addr: 7-bit device address.
  * @retval None.
  */
void UART_Protocol_SendAddr(uint16_t addr);

/**
  * @brief  Send an I2C diagnostic (ERROR:x\n) with the HAL error code.
  * @param  i2c_err: hi2c.ErrorCode value.
  * @retval None.
  */
void UART_Protocol_SendI2CError(uint32_t i2c_err);

/**
  * @brief  Send a PONG response.
  * @retval None.
  */
void UART_Protocol_SendPong(void);

/**
  * @brief  Poll for a complete received line and handle known commands.
  *         Call periodically from the main loop.
  * @retval None.
  */
void UART_Protocol_Poll(void);

/**
  * @brief  UART RX-complete callback (called from USARTx_IRQHandler / HAL).
  * @param  huart: pointer to UART handle.
  * @retval None.
  */
void UART_Protocol_HAL_RxCpltCallback(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif /* __UART_PROTOCOL_H */
