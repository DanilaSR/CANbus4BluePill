#include "mpu9250.h"

static HAL_StatusTypeDef MPU9250_WriteReg(MPU9250_t *imu, uint8_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(imu->hi2c, imu->addr, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, 100);
}

static HAL_StatusTypeDef MPU9250_ReadRegs(MPU9250_t *imu, uint8_t reg, uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Read(imu->hi2c, imu->addr, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
}

HAL_StatusTypeDef MPU9250_Init(MPU9250_t *imu, I2C_HandleTypeDef *hi2c, uint8_t addr)
{
    uint8_t who = 0;

    imu->hi2c = hi2c;
    imu->addr = addr;

    if (MPU9250_ReadRegs(imu, MPU9250_WHO_AM_I, &who, 1) != HAL_OK)
        return HAL_ERROR;

    /* MPU-9250 = 0x71, MPU-6500 = 0x70 */
    if (who != MPU9250_WHO_AM_I_VAL && who != MPU6500_WHO_AM_I_VAL)
        return HAL_ERROR;

    /* Wake up device (longer settle: some clone modules need >30ms after
     * wakeup before sensor data registers update) */
    if (MPU9250_WriteReg(imu, MPU9250_PWR_MGMT_1, 0x00) != HAL_OK)
        return HAL_ERROR;
    HAL_Delay(50);

    /* Disable I2C master / bypass */
    if (MPU9250_WriteReg(imu, MPU9250_PWR_MGMT_2, 0x00) != HAL_OK)
        return HAL_ERROR;

    /* Set sample rate divider to 0 (default 1kHz) */
    if (MPU9250_WriteReg(imu, 0x19, 0x00) != HAL_OK)
        return HAL_ERROR;

    /* Config: DLPF 184Hz, gyro at 1kHz */
    if (MPU9250_WriteReg(imu, MPU9250_CONFIG, 0x01) != HAL_OK)
        return HAL_ERROR;

    /* Gyro full scale: +-2000 deg/s */
    if (MPU9250_WriteReg(imu, MPU9250_GYRO_CONFIG, MPU9250_GYRO_FS_2000) != HAL_OK)
        return HAL_ERROR;
    imu->gyro_range = 2000.0f / 32768.0f;

    /* Accel full scale: +-16g */
    if (MPU9250_WriteReg(imu, MPU9250_ACCEL_CONFIG, MPU9250_ACCEL_FS_16G) != HAL_OK)
        return HAL_ERROR;
    imu->accel_range = 16.0f / 32768.0f;

    /* Accel DLPF config */
    if (MPU9250_WriteReg(imu, MPU9250_ACCEL_CONFIG2, 0x01) != HAL_OK)
        return HAL_ERROR;

    return HAL_OK;
}

HAL_StatusTypeDef MPU9250_ReadRaw(MPU9250_t *imu, MPU9250_Data_t *data)
{
    uint8_t buf[14];

    /* 14 bytes: accel(6) + temp(2) + gyro(6) */
    if (MPU9250_ReadRegs(imu, MPU9250_ACCEL_XOUT_H, buf, 14) != HAL_OK)
        return HAL_ERROR;

    int16_t ax = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t ay = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t az = (int16_t)((buf[4] << 8) | buf[5]);
    int16_t temp = (int16_t)((buf[6] << 8) | buf[7]);
    int16_t gx = (int16_t)((buf[8] << 8) | buf[9]);
    int16_t gy = (int16_t)((buf[10] << 8) | buf[11]);
    int16_t gz = (int16_t)((buf[12] << 8) | buf[13]);

    if (data)
    {
        data->ax = (float)ax;
        data->ay = (float)ay;
        data->az = (float)az;
        data->gx = (float)gx;
        data->gy = (float)gy;
        data->gz = (float)gz;
        data->temp = (float)temp;
    }

    return HAL_OK;
}

void MPU9250_Compute(MPU9250_t *imu, MPU9250_Data_t *data)
{
    data->ax *= imu->accel_range;
    data->ay *= imu->accel_range;
    data->az *= imu->accel_range;

    /* Gyro in deg/s */
    float g_sf = imu->gyro_range;
    data->gx *= g_sf;
    data->gy *= g_sf;
    data->gz *= g_sf;

    /* Temperature: raw/333.87 + 21 */
    data->temp = data->temp / 333.87f + 21.0f;
}
