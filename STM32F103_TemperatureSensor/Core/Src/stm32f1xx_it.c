#include "main.h"
#include "uart_protocol.h"

/* CAN1 handle defined in main.c. */
extern CAN_HandleTypeDef hcan1;

/* USART2 handle defined in main.c (Flipper Zero ASCII bridge). */
extern UART_HandleTypeDef huart2;

/* CAN1 interrupt handlers. The callbacks dispatch into libcanard (main.c). */
void CanardRxCallback(void);

void USB_HP_CAN1_TX_IRQHandler(void) { HAL_CAN_IRQHandler(&hcan1); }

void USB_LP_CAN1_RX0_IRQHandler(void) { HAL_CAN_IRQHandler(&hcan1); }

void CAN1_RX1_IRQHandler(void) { HAL_CAN_IRQHandler(&hcan1); }

void CAN1_SCE_IRQHandler(void) { HAL_CAN_IRQHandler(&hcan1); }

/* CAN callbacks wired from the HAL to the CANard stack. */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan) {
  if (hcan->Instance == CAN1) {
    CanardRxCallback();
  }
}

/* USART2 RX interrupt for the Flipper Zero protocol (see uart_protocol.c). */
void USART2_IRQHandler(void) { HAL_UART_IRQHandler(&huart2); }

/* Forward HAL UART RX-complete to the protocol line handler. */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
  UART_Protocol_HAL_RxCpltCallback(huart);
}