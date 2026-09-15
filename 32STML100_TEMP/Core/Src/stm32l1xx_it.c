/**
  ******************************************************************************
  * @file    stm32l1xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  */

#include "main.h"
#include "stm32l1xx_it.h"
#include "uart_protocol.h"

extern UART_HandleTypeDef huart2;

/******************************************************************************/
/*           Cortex-M3 Processor Interruption and Exception Handlers         */
/******************************************************************************/

/**
  * @brief  This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief  This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief  This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief  This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
}

/******************************************************************************/
/*                 STM32L1xx Peripheral Interrupt Handlers                    */
/*  Add here the interrupt handlers for the used peripherals                 */
/******************************************************************************/

/**
  * @brief  This function handles USART2 global interrupt.
  */
void USART2_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart2);
}

/**
  * @brief  UART RX complete callback - forwarded to the protocol handler.
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  UART_Protocol_HAL_RxCpltCallback(huart);
}
