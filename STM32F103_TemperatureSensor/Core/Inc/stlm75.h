/**
  ******************************************************************************
  * @file    stlm75.h
  * @brief   Driver for the STLM75DS2F temperature sensor over I2C.
  *
  *  Datasheet facts:
  *    - I2C 7-bit base address 0x48 (A2:A0 low)
  *    - Register 0x00 = Temperature, 9-bit two's complement @ 0.5C resolution
  *    - Bits: MSB[7:0] integer, LSB[7] = 0.5C fractional, LSB[6:0] don't care
  ******************************************************************************
  */

#ifndef __STLM75_H
#define __STLM75_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

/* STLM75 registers */
#define STLM75_REG_TEMP       0x00U   /* Temperature register (read)      */
#define STLM75_REG_CONF       0x01U   /* Configuration register (R/W)     */
#define STLM75_REG_THYS       0x02U   /* Hysteresis limit register (R/W)  */
#define STLM75_REG_TOS        0x03U   /* Overtemp shutdown limit (R/W)    */

/* Configuration register bits */
#define STLM75_CONF_SHUTDOWN    0x01U
#define STLM75_CONF_INTMODE     0x02U
#define STLM75_CONF_POLARITY    0x04U
#define STLM75_CONF_FAULT_1     0x00U
#define STLM75_CONF_FAULT_2     0x08U
#define STLM75_CONF_FAULT_4     0x10U
#define STLM75_CONF_FAULT_6     0x18U

/* I2C timeout */
#define STLM75_I2C_TIMEOUT      100U    /* milliseconds */

/**
  * @brief  Initialize the STLM75 in continuous conversion mode.
  * @param  hi2c: pointer to I2C handle.
  * @retval HAL status.
  */
HAL_StatusTypeDef STLM75_Init(I2C_HandleTypeDef *hi2c, uint16_t dev_addr);

/**
  * @brief  Check if the STLM75 is present/responsive on the I2C bus.
  * @param  hi2c: pointer to I2C handle.
  * @param  dev_addr: 8-bit (shifted) I2C address.
  * @retval HAL status.
  */
HAL_StatusTypeDef STLM75_IsReady(I2C_HandleTypeDef *hi2c, uint16_t dev_addr);

/**
  * @brief  Scan the 0x48..0x4F range (STLM75 address strapping) and return
  *         the first 8-bit (shifted) address that ACKs, or 0 if none found.
  * @param  hi2c: pointer to I2C handle.
  * @retval Found 8-bit address (e.g. 0x90 = 0x48<<1), or 0 if none.
  */
uint16_t STLM75_ScanAddress(I2C_HandleTypeDef *hi2c);

/**
  * @brief  Read the temperature in degrees Celsius.
  * @param  hi2c: pointer to I2C handle.
  * @param  dev_addr: 8-bit (shifted) I2C address.
  * @param  temp: pointer to output temperature (Celsius).
  * @retval HAL status.
  */
HAL_StatusTypeDef STLM75_ReadTemp(I2C_HandleTypeDef *hi2c, uint16_t dev_addr, float *temp);

#ifdef __cplusplus
}
#endif

#endif /* __STLM75_H */
