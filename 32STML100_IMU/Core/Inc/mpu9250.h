#ifndef __MPU9250_H
#define __MPU9250_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

/* MPU-9250 / MPU-6500 I2C address (AD0 = GND) */
#define MPU9250_ADDR         (0x68U << 1)
#define MPU9250_ADDR_AD0     (0x69U << 1)

/* Register map */
#define MPU9250_WHO_AM_I     0x75U
#define MPU9250_ACCEL_XOUT_H 0x3BU
#define MPU9250_ACCEL_XOUT_L 0x3CU
#define MPU9250_ACCEL_YOUT_H 0x3DU
#define MPU9250_ACCEL_YOUT_L 0x3EU
#define MPU9250_ACCEL_ZOUT_H 0x3FU
#define MPU9250_ACCEL_ZOUT_L 0x40U
#define MPU9250_GYRO_XOUT_H  0x43U
#define MPU9250_GYRO_XOUT_L  0x44U
#define MPU9250_GYRO_YOUT_H  0x45U
#define MPU9250_GYRO_YOUT_L  0x46U
#define MPU9250_GYRO_ZOUT_H  0x47U
#define MPU9250_GYRO_ZOUT_L  0x48U
#define MPU9250_TEMP_OUT_H   0x41U
#define MPU9250_TEMP_OUT_L   0x42U
#define MPU9250_CONFIG       0x1AU
#define MPU9250_GYRO_CONFIG  0x1BU
#define MPU9250_ACCEL_CONFIG 0x1CU
#define MPU9250_ACCEL_CONFIG2 0x1DU
#define MPU9250_PWR_MGMT_1   0x6BU
#define MPU9250_PWR_MGMT_2   0x6CU
#define MPU9250_INT_PIN_CFG  0x37U
#define MPU9250_INT_ENABLE   0x38U

/* I2C master (for AK8963 magnetometer on a real MPU-9250) */
#define MPU9250_I2C_MST_CTRL 0x24U
#define MPU9250_I2C_MST_STATUS 0x36U

/* Gyro full-scale ranges */
#define MPU9250_GYRO_FS_250   0x00U
#define MPU9250_GYRO_FS_500   0x08U
#define MPU9250_GYRO_FS_1000  0x10U
#define MPU9250_GYRO_FS_2000  0x18U

/* Accel full-scale ranges */
#define MPU9250_ACCEL_FS_2G   0x00U
#define MPU9250_ACCEL_FS_4G   0x08U
#define MPU9250_ACCEL_FS_8G   0x10U
#define MPU9250_ACCEL_FS_16G  0x18U

/* WHO_AM_I expected value: MPU-9250 = 0x71, MPU-6500 = 0x70 */
#define MPU9250_WHO_AM_I_VAL  0x71U
#define MPU6500_WHO_AM_I_VAL  0x70U

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t addr;
    float accel_range;
    float gyro_range;
} MPU9250_t;

typedef struct {
    float ax, ay, az;
    float gx, gy, gz;
    float temp;
} MPU9250_Data_t;

HAL_StatusTypeDef MPU9250_Init(MPU9250_t *imu, I2C_HandleTypeDef *hi2c, uint8_t addr);
HAL_StatusTypeDef MPU9250_ReadRaw(MPU9250_t *imu, MPU9250_Data_t *data);
void MPU9250_Compute(MPU9250_t *imu, MPU9250_Data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* __MPU9250_H */
