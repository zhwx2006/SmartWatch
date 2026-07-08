#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "main.h"
#include "mpu6050.h"
#include <stdint.h>
#include <string.h>

// ============================================================
// 缓冲区大小
// ============================================================
#define BT_TX_BUF_SIZE               64
#define BT_RX_BUF_SIZE               256

// ============================================================
// 函数声明
// ============================================================

/**
  * @brief 初始化蓝牙模块（开启 USART1 接收中断）
  */
void Bluetooth_Init(void);

/**
  * @brief 发送完整数据包（时间 + 步数 + 传感器数据）
  * @param time   时间结构体指针
  * @param sensor 传感器数据（含步数）结构体指针
  */
void Bluetooth_SendFullData(TimeData_t *time, SensorData_t *sensor);

/**
  * @brief 处理接收到的数据（在 main 循环中调用）
  */
void Bluetooth_ProcessReceived(void);

/**
  * @brief 发送原始字符串（调试用）
  */
void Bluetooth_SendString(char *str);

/**
  * @brief 检查蓝牙是否已连接（默认返回 1）
  */
uint8_t Bluetooth_IsConnected(void);

#endif
