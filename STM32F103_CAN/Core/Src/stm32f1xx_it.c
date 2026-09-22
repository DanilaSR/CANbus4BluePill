#include "main.h"

/* CAN1 handle defined in main.c. */
extern CAN_HandleTypeDef hcan1;

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