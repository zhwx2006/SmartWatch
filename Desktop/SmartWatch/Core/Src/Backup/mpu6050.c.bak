#include "mpu6050.h"
#include "i2c.h"
#include <math.h>

// ============================================================
// 底层 I2C 读写函数（使用 I2C2）
// ============================================================
static uint8_t MPU6050_WriteReg(uint8_t reg, uint8_t value) {
    return HAL_I2C_Mem_Write(&hi2c2, MPU6050_ADDR << 1, reg,
                             I2C_MEMADD_SIZE_8BIT, &value, 1, 100) == HAL_OK;
}

static uint8_t MPU6050_ReadReg(uint8_t reg, uint8_t *value) {
    return HAL_I2C_Mem_Read(&hi2c2, MPU6050_ADDR << 1, reg,
                            I2C_MEMADD_SIZE_8BIT, value, 1, 100) == HAL_OK;
}

static uint8_t MPU6050_ReadRegs(uint8_t reg, uint8_t *buf, uint8_t len) {
    return HAL_I2C_Mem_Read(&hi2c2, MPU6050_ADDR << 1, reg,
                            I2C_MEMADD_SIZE_8BIT, buf, len, 100) == HAL_OK;
}

// ============================================================
// 初始化函数
// ============================================================
uint8_t MPU6050_Init(void) {
    uint8_t whoami;

    if (!MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x80)) return 0;
    HAL_Delay(100);

    if (!MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x00)) return 0;
    HAL_Delay(100);

    if (!MPU6050_WriteReg(0x19, 0x04)) return 0;
    if (!MPU6050_WriteReg(0x1A, 0x03)) return 0;
    if (!MPU6050_WriteReg(0x1C, 0x00)) return 0;
    if (!MPU6050_WriteReg(0x1B, 0x00)) return 0;

    if (!MPU6050_ReadReg(MPU6050_WHO_AM_I, &whoami)) return 0;
    if (whoami != 0x68) return 0;

    return 1;
}

// ============================================================
// 读取全部数据
// ============================================================
void MPU6050_ReadAll(MPU6050_Data *data) {
    uint8_t buf[14];

    if (!MPU6050_ReadRegs(MPU6050_ACCEL_XOUT_H, buf, 14)) {
        data->accel_x = 0;
        data->accel_y = 0;
        data->accel_z = 0;
        data->gyro_x = 0;
        data->gyro_y = 0;
        data->gyro_z = 0;
        data->temp = 0;
        data->pitch = 0;
        data->roll = 0;
        return;
    }

    int16_t raw_accel_x = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw_accel_y = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t raw_accel_z = (int16_t)((buf[4] << 8) | buf[5]);
    int16_t raw_temp    = (int16_t)((buf[6] << 8) | buf[7]);
    int16_t raw_gyro_x  = (int16_t)((buf[8] << 8) | buf[9]);
    int16_t raw_gyro_y  = (int16_t)((buf[10] << 8) | buf[11]);
    int16_t raw_gyro_z  = (int16_t)((buf[12] << 8) | buf[13]);

    data->accel_x = raw_accel_x / 16384.0f;
    data->accel_y = raw_accel_y / 16384.0f;
    data->accel_z = raw_accel_z / 16384.0f;
    data->gyro_x  = raw_gyro_x / 131.0f;
    data->gyro_y  = raw_gyro_y / 131.0f;
    data->gyro_z  = raw_gyro_z / 131.0f;
    data->temp    = raw_temp / 340.0f + 36.53f;

    float accel_pitch = atan2f(data->accel_y, data->accel_z) * 57.2958f;
    float accel_roll  = atan2f(-data->accel_x,
                               sqrtf(data->accel_y*data->accel_y + data->accel_z*data->accel_z)) * 57.2958f;

    data->pitch = accel_pitch;
    data->roll  = accel_roll;
    data->yaw   = 0.0f;
}

// ============================================================
// Phase 5: 增强计步算法（合成加速度 + 动态阈值 + 步间隔检查）
// ============================================================
uint32_t MPU6050_GetSteps(void) {
    static uint32_t stepCount = 0;
    static float last_mag = 0;
    static uint8_t above_threshold = 0;
    static float peak_value = 0;
    static uint32_t last_step_time = 0;
    static uint8_t sample_cnt = 0;
    static float min_val = 10.0f, max_val = 0.0f;
    static float step_threshold = 1.2f;
    static uint8_t initialized = 0;

    MPU6050_Data data;
    MPU6050_ReadAll(&data);

    // 合成加速度
    float mag = sqrtf(data.accel_x * data.accel_x +
                      data.accel_y * data.accel_y +
                      data.accel_z * data.accel_z);

    // 首次采样，初始化 min/max
    if (!initialized) {
        min_val = mag;
        max_val = mag;
        initialized = 1;
    }

    // 动态阈值更新（每 20 次采样更新一次，适应 50ms 周期）
    sample_cnt++;
    if (mag < min_val) min_val = mag;
    if (mag > max_val) max_val = mag;
    if (sample_cnt >= 20) {
        float range = max_val - min_val;
        if (range < 0.2f) {
            step_threshold = 1.2f; // 无运动，保持默认
        } else {
            step_threshold = (min_val + max_val) / 2.0f + 0.25f;
            if (step_threshold < 0.8f) step_threshold = 0.8f;
            if (step_threshold > 2.0f) step_threshold = 2.0f;
        }
        sample_cnt = 0;
        min_val = mag;
        max_val = mag;
    }

    // 峰值检测 + 步间隔检查（150ms 适应快速步行）
    uint32_t now = HAL_GetTick();
    if (mag > step_threshold && !above_threshold) {
        above_threshold = 1;
        peak_value = mag;
    } else if (mag > peak_value && above_threshold) {
        peak_value = mag;
    } else if (mag < step_threshold && above_threshold) {
        above_threshold = 0;
        if (peak_value > step_threshold &&
            (now - last_step_time) >= 150) {
            stepCount++;
            last_step_time = now;
        }
    }

    last_mag = mag;
    return stepCount;
}
