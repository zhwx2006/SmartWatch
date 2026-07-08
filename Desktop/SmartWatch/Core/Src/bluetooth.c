#include "bluetooth.h"
#include "usart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================
// 外部变量
// ============================================================
extern UART_HandleTypeDef huart1;

// ============================================================
// 内部变量
// ============================================================
static uint8_t rx_buf[BT_RX_BUF_SIZE];
static uint16_t rx_head = 0;
static uint16_t rx_tail = 0;
static uint8_t cmd_buffer[32];
static uint8_t cmd_index = 0;

// ============================================================
// 初始化
// ============================================================
void Bluetooth_Init(void) {
    memset(rx_buf, 0, BT_RX_BUF_SIZE);
    rx_head = 0;
    rx_tail = 0;
    cmd_index = 0;

    HAL_UART_Receive_IT(&huart1, rx_buf, 1);
}

// ============================================================
// 发送原始字符串
// ============================================================
void Bluetooth_SendString(char *str) {
    uint16_t len = strlen(str);
    HAL_UART_Transmit(&huart1, (uint8_t*)str, len, 100);
}

// ============================================================
// 发送完整数据包（规整化格式）
// 格式: T:12:30:45,S:1234,A:0.0,0.0,1.0,P:0.0,R:0.0,Tmp:25.5C\r\n
// ============================================================
void Bluetooth_SendFullData(TimeData_t *time, SensorData_t *sensor) {
    char txt_buf[128];

    int ax = (int)(sensor->accel_x * 10);
    int ay = (int)(sensor->accel_y * 10);
    int az = (int)(sensor->accel_z * 10);
    int p  = (int)(sensor->pitch * 10);
    int r  = (int)(sensor->roll * 10);
    int t  = (int)(sensor->temp * 10);

    sprintf(txt_buf,
            "T:%02d:%02d:%02d,S:%lu,AX:%d.%d,AY:%d.%d,AZ:%d.%d,P:%d.%d,R:%d.%d,Tmp:%d.%dC\r\n",
            time->hour, time->min, time->sec,
            sensor->steps,
            ax/10, abs(ax%10),
            ay/10, abs(ay%10),
            az/10, abs(az%10),
            p/10, abs(p%10),
            r/10, abs(r%10),
            t/10, abs(t%10));

    HAL_UART_Transmit(&huart1, (uint8_t*)txt_buf, strlen(txt_buf), 100);
}

// ============================================================
// 处理接收数据（解析手机发来的命令）
// ============================================================
void Bluetooth_ProcessReceived(void) {
    uint8_t byte;

    while (rx_head != rx_tail) {
        byte = rx_buf[rx_tail];
        rx_tail = (rx_tail + 1) % BT_RX_BUF_SIZE;

        if (byte == '\n' || byte == '\r') {
            if (cmd_index > 0) {
                cmd_buffer[cmd_index] = '\0';
                cmd_index = 0;
            }
        } else {
            if (cmd_index < 31) {
                cmd_buffer[cmd_index++] = byte;
            }
        }
    }
}

// ============================================================
// 检查蓝牙连接状态（默认返回 1）
// ============================================================
uint8_t Bluetooth_IsConnected(void) {
    return 1;
}

// ============================================================
// 串口接收中断回调（在 stm32f1xx_it.c 中调用）
// ============================================================
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        uint16_t next_head = (rx_head + 1) % BT_RX_BUF_SIZE;
        if (next_head != rx_tail) {
            rx_buf[rx_head] = rx_buf[0];
            rx_head = next_head;
        }
        HAL_UART_Receive_IT(&huart1, rx_buf, 1);
    }
}
