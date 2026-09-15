/**
  ******************************************************************************
  * @file    stlm75.c
  * @brief   Driver for the STLM75DS2F temperature sensor over I2C.
  ******************************************************************************
  */

#include "stlm75.h"

/**
  * @brief  Initialize the STLM75 in continuous conversion mode.
  * @param  hi2c: pointer to I2C handle.
  * @param  dev_addr: 8-bit (shifted) I2C address.
  * @retval HAL status.
  */
HAL_StatusTypeDef STLM75_Init(I2C_HandleTypeDef *hi2c, uint16_t dev_addr)
{
  uint8_t conf = 0x00U; /* continuous mode, comparator mode, active-low, 1 fault */

  return HAL_I2C_Mem_Write(hi2c, dev_addr, STLM75_REG_CONF,
                           I2C_MEMADD_SIZE_8BIT, &conf, 1U, STLM75_I2C_TIMEOUT);
}

/**
  * @brief  Check if the STLM75 is present/responsive on the I2C bus.
  * @param  hi2c: pointer to I2C handle.
  * @param  dev_addr: 8-bit (shifted) I2C address.
  * @retval HAL status.
  */
HAL_StatusTypeDef STLM75_IsReady(I2C_HandleTypeDef *hi2c, uint16_t dev_addr)
{
  return HAL_I2C_IsDeviceReady(hi2c, dev_addr, 10U, STLM75_I2C_TIMEOUT);
}

/**
  * @brief  Scan 0x48..0x4F for a responding STLM75 and return its address.
  */
uint16_t STLM75_ScanAddress(I2C_HandleTypeDef *hi2c)
{
  uint16_t addr;
  for (addr = (0x48U << 1); addr <= (0x4FU << 1); addr += 2U)
  {
    if (HAL_I2C_IsDeviceReady(hi2c, addr, 3U, STLM75_I2C_TIMEOUT) == HAL_OK)
    {
      return addr;
    }
  }
  return 0U;
}

/**
  * @brief  Read the temperature in degrees Celsius.
  * @param  hi2c: pointer to I2C handle.
  * @param  dev_addr: 8-bit (shifted) I2C address.
  * @param  temp: pointer to output temperature (Celsius).
  * @retval HAL status.
  */
HAL_StatusTypeDef STLM75_ReadTemp(I2C_HandleTypeDef *hi2c, uint16_t dev_addr, float *temp)
{
  HAL_StatusTypeDef status;
  uint8_t buf[2];
  int16_t raw;

  status = HAL_I2C_Mem_Read(hi2c, dev_addr, STLM75_REG_TEMP,
                            I2C_MEMADD_SIZE_8BIT, buf, 2U, STLM75_I2C_TIMEOUT);
  if (status != HAL_OK)
  {
    return status;
  }

  /* 9-bit two's complement value in the upper 9 bits */
  raw = (int16_t)((buf[0] << 8) | buf[1]);
  raw >>= 7;                /* shift down: bit 8 becomes sign bit */

  *temp = (float)raw * 0.5f;

  return HAL_OK;
}
