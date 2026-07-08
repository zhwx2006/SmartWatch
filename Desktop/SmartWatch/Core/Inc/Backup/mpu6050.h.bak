#ifndef MPU6050_H
#define MPU6050_H

#include "main.h"

// MPU6050 7位地址（AD0 接 GND）
#define MPU6050_ADDR         0x68

// 常用寄存器地址
#define MPU6050_WHO_AM_I     0x75
#define MPU6050_PWR_MGMT_1   0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_TEMP_OUT_H   0x41
#define MPU6050_GYRO_XOUT_H  0x43

// 数据结构
typedef struct {
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float temp;
    float pitch;
    float roll;
    float yaw;
} MPU6050_Data;

// 函数声明
uint8_t MPU6050_Init(void);
void MPU6050_ReadAll(MPU6050_Data *data);
uint32_t MPU6050_GetSteps(void);   // ← 新增计步函数

#endif
