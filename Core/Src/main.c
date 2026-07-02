/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body - SmartWatch
  ******************************************************************************
  * @attention
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include <string.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

// 页面枚举（硬件文档 第八节）
typedef enum {
    PAGE_MAIN = 0,      // 主页：时间 + 步数 + 电池
    PAGE_SENSOR,        // 传感器页面：AX/AY/AZ
    PAGE_SPORT,         // 运动页面：步数 + 距离
    PAGE_SYSTEM         // 系统页面：电池 + I2C状态
} Page_t;

// 共享数据结构（硬件文档 第六节）
typedef struct {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    int16_t ax, ay, az;
    int16_t gx, gy, gz;

    uint32_t steps;
    uint8_t current_page;
    uint8_t bluetooth_connected;
    float battery_voltage;
} WatchData_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define OLED_ADDRESS    0x3C    // OLED I2C地址（7位）
#define MPU6050_ADDR    0x68    // MPU6050 I2C地址（7位）
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

// ============================================================
// 1. 全局共享数据（硬件文档 第六节）
// ============================================================
WatchData_t g_watchData = {
    .hour = 12,
    .minute = 0,
    .second = 0,
    .current_page = 0,
    .steps = 0,
    .battery_voltage = 3.8f,
    .bluetooth_connected = 0
};

// ============================================================
// 2. I2C 互斥锁（硬件文档 第六节）
// ============================================================
SemaphoreHandle_t xI2CMutex;

// ============================================================
// 3. 蓝牙接收缓冲区
// ============================================================
static char bt_rx_buffer[32];
static uint8_t bt_rx_index = 0;
static uint8_t bt_data_ready = 0;

// ============================================================
// 4. EC11 轮询用静态变量（KeyTask 中使用）
// ============================================================
static uint8_t encoder_a_last = 1;   // PA0 上一次状态
static uint32_t key_press_start = 0; // 按键按下起始时刻
static uint8_t key_is_pressed = 0;   // 按键是否正在被按下

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);

/* USER CODE BEGIN PFP */

// 模块函数声明
void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowString(uint8_t x, uint8_t y, char* str);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len);
void OLED_Update(void);

uint8_t MPU6050_Init(void);
void MPU6050_Read_Accel(int16_t *ax, int16_t *ay, int16_t *az);
void MPU6050_Read_Gyro(int16_t *gx, int16_t *gy, int16_t *gz);
uint32_t MPU6050_GetSteps(void);

void Bluetooth_Process(void);
void Bluetooth_SendData(char *data);

void Page_Refresh(void);

// FreeRTOS 任务函数（供 freertos.c 调用）
void StartTimeTask_Impl(void *argument);
void StartKeyTask_Impl(void *argument);
void StartSensorTask_Impl(void *argument);
void StartUITask_Impl(void *argument);
void StartBluetoothTask_Impl(void *argument);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// ============================================================
// 1. OLED 驱动函数 (I2C接口，含 6x8 字库)
// ============================================================
static void OLED_WriteCmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDRESS<<1, buf, 2, HAL_MAX_DELAY);
}

static void OLED_WriteData(uint8_t dat) {
    uint8_t buf[2] = {0x40, dat};
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDRESS<<1, buf, 2, HAL_MAX_DELAY);
}

void OLED_Init(void) {
    HAL_Delay(100);
    OLED_WriteCmd(0xAE); // 关闭显示
    OLED_WriteCmd(0xD5); OLED_WriteCmd(0x80);
    OLED_WriteCmd(0xA8); OLED_WriteCmd(0x3F);
    OLED_WriteCmd(0xD3); OLED_WriteCmd(0x00);
    OLED_WriteCmd(0x40);
    OLED_WriteCmd(0x8D); OLED_WriteCmd(0x14);
    OLED_WriteCmd(0x20); OLED_WriteCmd(0x02);
    OLED_WriteCmd(0xA1);
    OLED_WriteCmd(0xC8);
    OLED_WriteCmd(0xDA); OLED_WriteCmd(0x12);
    OLED_WriteCmd(0x81); OLED_WriteCmd(0xCF);
    OLED_WriteCmd(0xD9); OLED_WriteCmd(0xF1);
    OLED_WriteCmd(0xDB); OLED_WriteCmd(0x40);
    OLED_WriteCmd(0xA4);
    OLED_WriteCmd(0xA6);
    OLED_WriteCmd(0x2E);
    OLED_WriteCmd(0xAF); // 开启显示
    OLED_Clear();
}

void OLED_Clear(void) {
    for(uint8_t page = 0; page < 8; page++) {
        OLED_WriteCmd(0xB0 + page);
        OLED_WriteCmd(0x00);
        OLED_WriteCmd(0x10);
        for(uint8_t col = 0; col < 128; col++) {
            OLED_WriteData(0x00);
        }
    }
}

// 简易 6x8 字库（ASCII 32~127）
static const uint8_t Font6x8[][6] = {
    {0x00,0x00,0x00,0x00,0x00,0x00}, // 空格 (32)
    {0x00,0x00,0x00,0x2F,0x00,0x00}, // ! (33)
    {0x00,0x00,0x07,0x00,0x07,0x00}, // " (34)
    {0x00,0x14,0x7F,0x14,0x7F,0x14}, // # (35)
    {0x00,0x24,0x2A,0x7F,0x2A,0x12}, // $ (36)
    {0x00,0x23,0x13,0x08,0x64,0x62}, // % (37)
    {0x00,0x36,0x49,0x55,0x22,0x50}, // & (38)
    {0x00,0x00,0x05,0x03,0x00,0x00}, // ' (39)
    {0x00,0x00,0x1C,0x22,0x41,0x00}, // ( (40)
    {0x00,0x00,0x41,0x22,0x1C,0x00}, // ) (41)
    {0x00,0x14,0x08,0x3E,0x08,0x14}, // * (42)
    {0x00,0x08,0x08,0x3E,0x08,0x08}, // + (43)
    {0x00,0x00,0x50,0x30,0x00,0x00}, // , (44)
    {0x00,0x08,0x08,0x08,0x08,0x08}, // - (45)
    {0x00,0x00,0x60,0x60,0x00,0x00}, // . (46)
    {0x00,0x20,0x10,0x08,0x04,0x02}, // / (47)
    {0x00,0x3E,0x51,0x49,0x45,0x3E}, // 0 (48)
    {0x00,0x00,0x42,0x7F,0x40,0x00}, // 1 (49)
    {0x00,0x42,0x61,0x51,0x49,0x46}, // 2 (50)
    {0x00,0x21,0x41,0x45,0x4B,0x31}, // 3 (51)
    {0x00,0x18,0x14,0x12,0x7F,0x10}, // 4 (52)
    {0x00,0x27,0x45,0x45,0x45,0x39}, // 5 (53)
    {0x00,0x3C,0x4A,0x49,0x49,0x30}, // 6 (54)
    {0x00,0x01,0x71,0x09,0x05,0x03}, // 7 (55)
    {0x00,0x36,0x49,0x49,0x49,0x36}, // 8 (56)
    {0x00,0x06,0x49,0x49,0x29,0x1E}, // 9 (57)
    {0x00,0x00,0x36,0x36,0x00,0x00}, // : (58)
    {0x00,0x00,0x56,0x36,0x00,0x00}, // ; (59)
    {0x00,0x08,0x14,0x22,0x41,0x00}, // < (60)
    {0x00,0x14,0x14,0x14,0x14,0x14}, // = (61)
    {0x00,0x00,0x41,0x22,0x14,0x08}, // > (62)
    {0x00,0x02,0x01,0x51,0x09,0x06}, // ? (63)
    {0x00,0x32,0x49,0x79,0x41,0x3E}, // @ (64)
    {0x00,0x7E,0x11,0x11,0x11,0x7E}, // A (65)
    {0x00,0x7F,0x49,0x49,0x49,0x36}, // B (66)
    {0x00,0x3E,0x41,0x41,0x41,0x22}, // C (67)
    {0x00,0x7F,0x41,0x41,0x22,0x1C}, // D (68)
    {0x00,0x7F,0x49,0x49,0x49,0x41}, // E (69)
    {0x00,0x7F,0x09,0x09,0x09,0x01}, // F (70)
    {0x00,0x3E,0x41,0x49,0x49,0x7A}, // G (71)
    {0x00,0x7F,0x08,0x08,0x08,0x7F}, // H (72)
    {0x00,0x00,0x41,0x7F,0x41,0x00}, // I (73)
    {0x00,0x20,0x40,0x41,0x3F,0x01}, // J (74)
    {0x00,0x7F,0x08,0x14,0x22,0x41}, // K (75)
    {0x00,0x7F,0x40,0x40,0x40,0x40}, // L (76)
    {0x00,0x7F,0x02,0x0C,0x02,0x7F}, // M (77)
    {0x00,0x7F,0x04,0x08,0x10,0x7F}, // N (78)
    {0x00,0x3E,0x41,0x41,0x41,0x3E}, // O (79)
    {0x00,0x7F,0x09,0x09,0x09,0x06}, // P (80)
    {0x00,0x3E,0x41,0x51,0x21,0x5E}, // Q (81)
    {0x00,0x7F,0x09,0x19,0x29,0x46}, // R (82)
    {0x00,0x46,0x49,0x49,0x49,0x31}, // S (83)
    {0x00,0x01,0x01,0x7F,0x01,0x01}, // T (84)
    {0x00,0x3F,0x40,0x40,0x40,0x3F}, // U (85)
    {0x00,0x1F,0x20,0x40,0x20,0x1F}, // V (86)
    {0x00,0x3F,0x40,0x38,0x40,0x3F}, // W (87)
    {0x00,0x63,0x14,0x08,0x14,0x63}, // X (88)
    {0x00,0x07,0x08,0x70,0x08,0x07}, // Y (89)
    {0x00,0x61,0x51,0x49,0x45,0x43}, // Z (90)
    {0x00,0x00,0x7F,0x41,0x41,0x00}, // [ (91)
    {0x00,0x02,0x04,0x08,0x10,0x20}, // \ (92)
    {0x00,0x00,0x41,0x41,0x7F,0x00}, // ] (93)
    {0x00,0x04,0x02,0x01,0x02,0x04}, // ^ (94)
    {0x00,0x40,0x40,0x40,0x40,0x40}, // _ (95)
    {0x00,0x00,0x01,0x02,0x04,0x00}, // ` (96)
    {0x00,0x20,0x54,0x54,0x54,0x78}, // a (97)
    {0x00,0x7F,0x48,0x44,0x44,0x38}, // b (98)
    {0x00,0x38,0x44,0x44,0x44,0x20}, // c (99)
    {0x00,0x38,0x44,0x44,0x48,0x7F}, // d (100)
    {0x00,0x38,0x54,0x54,0x54,0x18}, // e (101)
    {0x00,0x08,0x7E,0x09,0x01,0x02}, // f (102)
    {0x00,0x0C,0x52,0x52,0x52,0x3E}, // g (103)
    {0x00,0x7F,0x08,0x04,0x04,0x78}, // h (104)
    {0x00,0x00,0x44,0x7D,0x40,0x00}, // i (105)
    {0x00,0x20,0x40,0x44,0x3D,0x00}, // j (106)
    {0x00,0x7F,0x10,0x28,0x44,0x00}, // k (107)
    {0x00,0x00,0x41,0x7F,0x40,0x00}, // l (108)
    {0x00,0x7C,0x04,0x18,0x04,0x78}, // m (109)
    {0x00,0x7C,0x08,0x04,0x04,0x78}, // n (110)
    {0x00,0x38,0x44,0x44,0x44,0x38}, // o (111)
    {0x00,0x7C,0x14,0x14,0x14,0x08}, // p (112)
    {0x00,0x08,0x14,0x14,0x18,0x7C}, // q (113)
    {0x00,0x7C,0x08,0x04,0x04,0x08}, // r (114)
    {0x00,0x48,0x54,0x54,0x54,0x20}, // s (115)
    {0x00,0x04,0x3F,0x44,0x40,0x20}, // t (116)
    {0x00,0x3C,0x40,0x40,0x20,0x7C}, // u (117)
    {0x00,0x1C,0x20,0x40,0x20,0x1C}, // v (118)
    {0x00,0x3C,0x40,0x30,0x40,0x3C}, // w (119)
    {0x00,0x44,0x28,0x10,0x28,0x44}, // x (120)
    {0x00,0x0C,0x50,0x50,0x50,0x3C}, // y (121)
    {0x00,0x44,0x64,0x54,0x4C,0x44}, // z (122)
    {0x00,0x00,0x08,0x77,0x41,0x00}, // { (123)
    {0x00,0x00,0x00,0x7F,0x00,0x00}, // | (124)
    {0x00,0x00,0x41,0x77,0x08,0x00}, // } (125)
    {0x00,0x02,0x01,0x02,0x01,0x02}  // ~ (126)
};

// 显示字符串（6x8 字体）
void OLED_ShowString(uint8_t x, uint8_t y, char* str) {
    uint8_t page = y / 8;
    uint8_t col = x;

    if(y % 8 != 0) return; // 只支持 8 的倍数行

    while(*str && col < 128) {
        uint8_t c = *str++;
        if(c < 32 || c > 126) c = 32; // 无效字符显示为空格

        // 写入 6 列数据
        for(uint8_t i = 0; i < 6; i++) {
            OLED_WriteCmd(0xB0 + page);
            OLED_WriteCmd(0x00 + (col % 16));
            OLED_WriteCmd(0x10 + (col / 16));
            OLED_WriteData(Font6x8[c - 32][i]);
            col++;
        }
        // 列间距 1 像素
        OLED_WriteCmd(0xB0 + page);
        OLED_WriteCmd(0x00 + (col % 16));
        OLED_WriteCmd(0x10 + (col / 16));
        OLED_WriteData(0x00);
        col++;
    }
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len) {
    char buf[16];
    sprintf(buf, "%0*d", len, num);
    OLED_ShowString(x, y, buf);
}

void OLED_Update(void) {
    // 不需要额外操作（直接写模式）
}

// ============================================================
// 2. MPU6050 驱动函数
// ============================================================
uint8_t MPU6050_Init(void) {
    uint8_t check;
    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR<<1, 0x75, I2C_MEMADD_SIZE_8BIT, &check, 1, 100);
    if(check != 0x68) return 0;

    uint8_t data = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR<<1, 0x6B, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
    return 1;
}

void MPU6050_Read_Accel(int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t buf[6];
    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR<<1, 0x3B, I2C_MEMADD_SIZE_8BIT, buf, 6, 100);
    if(ax) *ax = (buf[0]<<8) | buf[1];
    if(ay) *ay = (buf[2]<<8) | buf[3];
    if(az) *az = (buf[4]<<8) | buf[5];
}

void MPU6050_Read_Gyro(int16_t *gx, int16_t *gy, int16_t *gz) {
    uint8_t buf[6];
    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR<<1, 0x43, I2C_MEMADD_SIZE_8BIT, buf, 6, 100);
    if(gx) *gx = (buf[0]<<8) | buf[1];
    if(gy) *gy = (buf[2]<<8) | buf[3];
    if(gz) *gz = (buf[4]<<8) | buf[5];
}

// 简易计步算法（检测Z轴加速度峰值）
uint32_t MPU6050_GetSteps(void) {
    static uint32_t stepCount = 0;
    static int16_t lastAz = 0;
    int16_t az;
    MPU6050_Read_Accel(NULL, NULL, &az);

    if((lastAz < 0 && az > 500) || (lastAz > 0 && az < -500)) {
        stepCount++;
    }
    lastAz = az;
    return stepCount;
}

// ============================================================
// 3. 蓝牙通信函数（硬件文档 第七节）
// ============================================================
void Bluetooth_SendData(char *data) {
    HAL_UART_Transmit(&huart1, (uint8_t*)data, strlen(data), 100);
    HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, 100);
}

void Bluetooth_Process(void) {
    if(!bt_data_ready) return;
    bt_data_ready = 0;

    if(strstr(bt_rx_buffer, "GET") != NULL) {
        char sendBuf[64];
        sprintf(sendBuf, "TIME=%02d:%02d:%02d,STEP=%lu,AX=%d,AY=%d,AZ=%d",
                g_watchData.hour, g_watchData.minute, g_watchData.second,
                g_watchData.steps,
                g_watchData.ax, g_watchData.ay, g_watchData.az);
        Bluetooth_SendData(sendBuf);
    }
    else if(strstr(bt_rx_buffer, "RESETSTEP") != NULL) {
        g_watchData.steps = 0;
    }
    else if(strstr(bt_rx_buffer, "SETTIME") != NULL) {
        int h, m, s;
        if(sscanf(bt_rx_buffer, "SETTIME,%d,%d,%d", &h, &m, &s) == 3) {
            g_watchData.hour = h;
            g_watchData.minute = m;
            g_watchData.second = s;
        }
    }
    else if(strstr(bt_rx_buffer, "PAGE") != NULL) {
        int p;
        if(sscanf(bt_rx_buffer, "PAGE,%d", &p) == 1) {
            if(p >= 0 && p < 4) g_watchData.current_page = p;
        }
    }
}

// ============================================================
// 4. 页面刷新函数（硬件文档 第八节）
// ============================================================
void Page_Refresh(void) {
    char line[24];
    OLED_Clear();

    switch(g_watchData.current_page) {
        case PAGE_MAIN:  // 主页
            OLED_ShowString(0, 0, "SmartWatch");
            sprintf(line, "Time:%02d:%02d:%02d", g_watchData.hour, g_watchData.minute, g_watchData.second);
            OLED_ShowString(0, 16, line);
            sprintf(line, "Steps:%lu", g_watchData.steps);
            OLED_ShowString(0, 32, line);
            sprintf(line, "Bat:%.2fV", g_watchData.battery_voltage);
            OLED_ShowString(0, 48, line);
            break;

        case PAGE_SENSOR:  // 传感器页面
            OLED_ShowString(0, 0, "--- Sensor ---");
            sprintf(line, "AX:%6d", g_watchData.ax);
            OLED_ShowString(0, 16, line);
            sprintf(line, "AY:%6d", g_watchData.ay);
            OLED_ShowString(0, 32, line);
            sprintf(line, "AZ:%6d", g_watchData.az);
            OLED_ShowString(0, 48, line);
            break;

        case PAGE_SPORT:   // 运动页面
            OLED_ShowString(0, 0, "--- Sport ---");
            sprintf(line, "Steps:%lu", g_watchData.steps);
            OLED_ShowString(0, 16, line);
            sprintf(line, "Dist:%.2fkm", g_watchData.steps * 0.0008f);
            OLED_ShowString(0, 32, line);
            break;

        case PAGE_SYSTEM:  // 系统页面
            OLED_ShowString(0, 0, "--- System ---");
            sprintf(line, "Bat:%.2fV", g_watchData.battery_voltage);
            OLED_ShowString(0, 16, line);
            OLED_ShowString(0, 32, "I2C:OK");
            OLED_ShowString(0, 48, "UART:OK");
            break;

        default:
            break;
    }
    OLED_Update();
}

/* USER CODE END 0 */

// ============================================================
// 5. 主函数
// ============================================================
int main(void)
{
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();

  /* USER CODE BEGIN 2 */

  // 创建 I2C 互斥锁（硬件文档 第六节）
  xI2CMutex = xSemaphoreCreateMutex();

  // 初始化 OLED
  OLED_Init();
  OLED_ShowString(0, 0, "SmartWatch");
  OLED_ShowString(0, 16, "Starting...");
  OLED_Update();

  // 启动串口接收中断
  HAL_UART_Receive_IT(&huart1, (uint8_t*)bt_rx_buffer, 1);

  /* USER CODE END 2 */

  MX_FREERTOS_Init();
  osKernelStart();

  while (1)
  {
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
  }
}

// ============================================================
// 6. 系统时钟配置
// ============================================================
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) Error_Handler();

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) Error_Handler();
}

/* USER CODE BEGIN 4 */

// ============================================================
// 7. FreeRTOS 任务函数（供 freertos.c 调用）
// 硬件文档 第五节：任务划分
// ============================================================

// 任务1: 时间任务 (周期 1000ms)
void StartTimeTask_Impl(void *argument) {
    for(;;) {
        g_watchData.second++;
        if(g_watchData.second >= 60) {
            g_watchData.second = 0;
            g_watchData.minute++;
            if(g_watchData.minute >= 60) {
                g_watchData.minute = 0;
                g_watchData.hour++;
                if(g_watchData.hour >= 24) g_watchData.hour = 0;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// 任务2: 按键/编码器任务 (周期 10ms，轮询方式)
void StartKeyTask_Impl(void *argument) {
    uint8_t a_val, b_val, sw_val;

    for(;;) {
        // 1. 读取 EC11 引脚状态
        a_val = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
        b_val = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1);
        sw_val = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2);

        // 2. 旋转检测（检测 PA0 下降沿）
        if ((encoder_a_last == 1) && (a_val == 0)) {
            if (b_val == 1) {
                // 顺时针 -> 下一页
                g_watchData.current_page = (g_watchData.current_page + 1) % 4;
            } else {
                // 逆时针 -> 上一页
                if (g_watchData.current_page > 0) {
                    g_watchData.current_page--;
                } else {
                    g_watchData.current_page = 3;
                }
            }
        }
        encoder_a_last = a_val;

        // 3. 按键检测（长按回主页，短按清零步数）
        if (sw_val == 0) {  // 按下（低电平）
            if (!key_is_pressed) {
                key_is_pressed = 1;
                key_press_start = HAL_GetTick();
            }
        } else {  // 释放
            if (key_is_pressed) {
                uint32_t duration = HAL_GetTick() - key_press_start;
                if (duration > 1000) {
                    // 长按 > 1秒：返回主页
                    g_watchData.current_page = 0;
                } else if (duration > 30) {
                    // 短按（>30ms 消抖）：清零步数
                    g_watchData.steps = 0;
                }
                key_is_pressed = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// 任务3: 传感器任务 (周期 100ms)
void StartSensorTask_Impl(void *argument) {
    // 初始化 MPU6050
    if(MPU6050_Init() == 0) {
        char msg[] = "MPU6050 Init Failed!\r\n";
        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
    }

    for(;;) {
        // 获取 I2C 互斥锁
        if (xSemaphoreTake(xI2CMutex, portMAX_DELAY) == pdTRUE) {
            // 读取 MPU6050
            MPU6050_Read_Accel(&g_watchData.ax, &g_watchData.ay, &g_watchData.az);
            MPU6050_Read_Gyro(&g_watchData.gx, &g_watchData.gy, &g_watchData.gz);
            g_watchData.steps = MPU6050_GetSteps();

            // 读取电池电压 (ADC)
            HAL_ADC_Start(&hadc1);
            if(HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
                uint16_t adc_val = HAL_ADC_GetValue(&hadc1);
                g_watchData.battery_voltage = adc_val * (3.3f / 4096.0f) * 2.0f;
            }
            HAL_ADC_Stop(&hadc1);

            xSemaphoreGive(xI2CMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// 任务4: UI 显示任务 (周期 200ms)
void StartUITask_Impl(void *argument) {
    for(;;) {
        // 获取 I2C 互斥锁
        if (xSemaphoreTake(xI2CMutex, portMAX_DELAY) == pdTRUE) {
            Page_Refresh();
            xSemaphoreGive(xI2CMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// 任务5: 蓝牙任务 (周期 500ms)
void StartBluetoothTask_Impl(void *argument) {
    char tx_buf[64];
    uint32_t lastSend = 0;

    for(;;) {
        // 处理接收到的指令
        Bluetooth_Process();

        // 每 500ms 主动上传数据
        uint32_t now = HAL_GetTick();
        if (now - lastSend >= 500) {
            lastSend = now;
            sprintf(tx_buf, "TIME=%02d:%02d,STEP=%lu",
                    g_watchData.hour, g_watchData.minute, g_watchData.steps);
            Bluetooth_SendData(tx_buf);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ============================================================
// 8. 串口接收中断回调
// ============================================================
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        // 接收单字节，存入缓冲区
        if (bt_rx_buffer[0] == '\n' || bt_rx_buffer[0] == '\r') {
            // 换行符：结束当前命令
            if (bt_rx_index > 0) {
                bt_rx_buffer[bt_rx_index] = '\0';
                bt_data_ready = 1;
                bt_rx_index = 0;
            }
        } else {
            if (bt_rx_index < 31) {
                bt_rx_buffer[bt_rx_index++] = bt_rx_buffer[0];
            }
        }
        // 继续接收下一个字节
        HAL_UART_Receive_IT(&huart1, (uint8_t*)bt_rx_buffer, 1);
    }
}

/* USER CODE END 4 */

// ============================================================
// 9. 系统回调与错误处理
// ============================================================
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2) {
    HAL_IncTick();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
