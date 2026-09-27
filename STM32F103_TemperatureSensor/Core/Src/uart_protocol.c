/**
  ******************************************************************************
  * @file    uart_protocol.c
  * @brief   Simple newline-terminated ASCII protocol to/from Flipper Zero.
  ******************************************************************************
  */

#include "uart_protocol.h"
#include <string.h>
#include <stdio.h>

static UART_HandleTypeDef *s_huart;
static volatile uint8_t rx_byte;
static volatile uint8_t rx_line_ready = 0;
static uint8_t rx_buf[UART_PROTOCOL_RXBUF_SIZE];
static volatile uint8_t rx_index = 0;

/**
  * @brief  Initialize the UART receive.
  */
void UART_Protocol_Init(UART_HandleTypeDef *huart)
{
  s_huart = huart;
  rx_index = 0;
  rx_line_ready = 0;
  HAL_UART_Receive_IT(huart, (uint8_t *)&rx_byte, 1U);
}

/**
  * @brief  Send a temperature reading to the Flipper.
  */
void UART_Protocol_SendTemp(float temp)
{
  char line[24];
  int len = snprintf(line, sizeof(line), "TEMP:%.1f\n", (double)temp);
  HAL_UART_Transmit(s_huart, (uint8_t *)line, (uint16_t)len, 100U);
}

/**
  * @brief  Send a PONG response.
  */
void UART_Protocol_SendPong(void)
{
  const char pong[] = "PONG\n";
  HAL_UART_Transmit(s_huart, (uint8_t *)pong, (uint16_t)(sizeof(pong) - 1U), 100U);
}

/**
  * @brief  Send a heartbeat.
  */
void UART_Protocol_SendBeat(void)
{
  const char beat[] = "BEAT\n";
  HAL_UART_Transmit(s_huart, (uint8_t *)beat, (uint16_t)(sizeof(beat) - 1U), 100U);
}

/**
  * @brief  Send a sensor-error message.
  */
void UART_Protocol_SendError(void)
{
  const char err[] = "ERR\n";
  HAL_UART_Transmit(s_huart, (uint8_t *)err, (uint16_t)(sizeof(err) - 1U), 100U);
}

/**
  * @brief  Report the detected STLM75 address.
  */
void UART_Protocol_SendAddr(uint16_t addr)
{
  char line[24];
  int len = snprintf(line, sizeof(line), "ADDR:0x%02x\n", (unsigned)addr);
  HAL_UART_Transmit(s_huart, (uint8_t *)line, (uint16_t)len, 100U);
}

/**
  * @brief  Send an I2C diagnostic with the HAL error code.
  */
void UART_Protocol_SendI2CError(uint32_t i2c_err)
{
  char line[32];
  int len = snprintf(line, sizeof(line), "ERROR:0x%08lx\n", (unsigned long)i2c_err);
  HAL_UART_Transmit(s_huart, (uint8_t *)line, (uint16_t)len, 100U);
}

/**
  * @brief  Handle a single received line (stored in rx_buf).
  */
static void UART_Protocol_HandleLine(void)
{
  /* PING -> respond PONG */
  if (strncmp((char *)rx_buf, "PING", 4U) == 0)
  {
    UART_Protocol_SendPong();
  }
  /* READ -> the main loop will send TEMP on the next tick; nothing to do here */
}

/**
  * @brief  Poll for a complete received line and handle known commands.
  */
void UART_Protocol_Poll(void)
{
  if (rx_line_ready)
  {
    UART_Protocol_HandleLine();
    rx_index = 0;
    rx_line_ready = 0;
  }
}

/**
  * @brief  UART RX-complete callback (called from USARTx_IRQHandler / HAL).
  */
void UART_Protocol_HAL_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance != s_huart->Instance)
  {
    return;
  }

  /* CR or LF terminate a line */
  if ((rx_byte == '\n') || (rx_byte == '\r'))
  {
    if (rx_index > 0U)
    {
      rx_buf[rx_index] = '\0';
      rx_line_ready = 1;
    }
  }
  else if (rx_index < (UART_PROTOCOL_RXBUF_SIZE - 1U))
  {
    rx_buf[rx_index++] = rx_byte;
  }

  /* Re-arm reception for the next byte */
  HAL_UART_Receive_IT(s_huart, (uint8_t *)&rx_byte, 1U);
}
