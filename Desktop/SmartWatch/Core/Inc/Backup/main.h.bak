/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

// 页面枚举
typedef enum {
    PAGE_TIME = 0,
    PAGE_SENSOR,
    PAGE_SPORT,
    PAGE_SYSTEM,
    PAGE_COUNT
} Page_t;

// 时间数据结构
typedef struct {
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
} TimeData_t;

// 传感器数据结构
typedef struct {
    float accel_x, accel_y, accel_z;
    float pitch, roll;
    float temp;
    uint32_t steps;
} SensorData_t;

// 菜单消息结构
typedef struct {
    int16_t encoder_delta;
    uint8_t button_event;   // 0=无, 1=短按, 2=长按
} MenuMsg_t;

// ============================================================
// Phase 5: 多级菜单系统数据结构
// ============================================================

// 前向声明
struct MenuItem_t;

// 菜单项结构体
typedef struct MenuItem_t {
    const char *name;                       // 菜单项名称
    uint8_t sub_items;                      // 子项数量（0表示叶子节点）
    const struct MenuItem_t *sub_menu;      // 子菜单指针
    void (*action)(void);                   // 叶子节点执行的动作
} MenuItem_t;

// 菜单状态
typedef struct {
    const MenuItem_t *current_menu;         // 当前菜单指针
    uint8_t current_item;                   // 当前选中项索引
    uint8_t depth;                          // 当前层级深度
    uint8_t prev_item;                      // 返回时用
    uint8_t edit_mode;                      // 0=浏览, 1=编辑
    uint8_t edit_value;                     // 编辑中的值
} MenuState_t;

// 菜单事件
typedef enum {
    MENU_EVENT_NONE = 0,
    MENU_EVENT_NEXT,
    MENU_EVENT_PREV,
    MENU_EVENT_ENTER,
    MENU_EVENT_BACK
} MenuEvent_t;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
