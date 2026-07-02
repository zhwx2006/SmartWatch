/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* 外部引用 main.c 中定义的互斥锁句柄（硬件文档 第六节） */
extern SemaphoreHandle_t xI2CMutex;

/* USER CODE END Variables */

/* 任务句柄（必须与 CubeMX 中创建的任务名称一致） */
osThreadId Task_TimeHandle;
osThreadId Task_KeyHandle;
osThreadId Task_SensorHandle;
osThreadId Task_UIHandle;
osThreadId Task_BluetoothHandle;

/* 互斥锁句柄（CubeMX 中创建） */
osMutexId I2C_MutexHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/* 任务函数声明（与 main.c 中实现的函数匹配） */
void StartTimeTask(void const * argument);
void StartKeyTask(void const * argument);
void StartSensorTask(void const * argument);
void StartUITask(void const * argument);
void StartBluetoothTask(void const * argument);

void MX_FREERTOS_Init(void);

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* ============================================================
     创建互斥锁（硬件文档 第六节）
     ============================================================ */
  osMutexDef(I2C_Mutex);
  I2C_MutexHandle = osMutexCreate(osMutex(I2C_Mutex));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* ============================================================
     创建 5 个任务（硬件文档 第五节）
     ============================================================ */

  /* 任务1: 时间任务 (周期 1000ms，优先级 Normal) */
  osThreadDef(Task_Time, StartTimeTask, osPriorityNormal, 0, 128);
  Task_TimeHandle = osThreadCreate(osThread(Task_Time), NULL);

  /* 任务2: 按键/编码器任务 (周期 10ms，优先级 High) */
  osThreadDef(Task_Key, StartKeyTask, osPriorityHigh, 0, 128);
  Task_KeyHandle = osThreadCreate(osThread(Task_Key), NULL);

  /* 任务3: 传感器任务 (周期 100ms，优先级 High) */
  osThreadDef(Task_Sensor, StartSensorTask, osPriorityNormal, 0, 128);
  Task_SensorHandle = osThreadCreate(osThread(Task_Sensor), NULL);

  /* 任务4: UI 显示任务 (周期 200ms，优先级 Normal) */
  osThreadDef(Task_UI, StartUITask, osPriorityNormal, 0, 128);
  Task_UIHandle = osThreadCreate(osThread(Task_UI), NULL);

  /* 任务5: 蓝牙任务 (周期 500ms，优先级 Low) */
  osThreadDef(Task_Bluetooth, StartBluetoothTask, osPriorityLow, 0, 256);
  Task_BluetoothHandle = osThreadCreate(osThread(Task_Bluetooth), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* ============================================================
   任务函数体（调用 main.c 中已实现的具体逻辑）
   ============================================================ */

/* USER CODE BEGIN Header_StartTimeTask */
/**
  * @brief  Function implementing the Task_Time thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartTimeTask */
void StartTimeTask(void const * argument)
{
  /* USER CODE BEGIN StartTimeTask */
  /* 调用 main.c 中实现的时间任务 */
  extern void StartTimeTask_Impl(void *argument);
  StartTimeTask_Impl((void*)argument);
  /* USER CODE END StartTimeTask */
}

/* USER CODE BEGIN Header_StartKeyTask */
/**
  * @brief  Function implementing the Task_Key thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartKeyTask */
void StartKeyTask(void const * argument)
{
  /* USER CODE BEGIN StartKeyTask */
  /* 调用 main.c 中实现的按键/编码器任务 */
  extern void StartKeyTask_Impl(void *argument);
  StartKeyTask_Impl((void*)argument);
  /* USER CODE END StartKeyTask */
}

/* USER CODE BEGIN Header_StartSensorTask */
/**
  * @brief  Function implementing the Task_Sensor thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartSensorTask */
void StartSensorTask(void const * argument)
{
  /* USER CODE BEGIN StartSensorTask */
  /* 调用 main.c 中实现的传感器任务 */
  extern void StartSensorTask_Impl(void *argument);
  StartSensorTask_Impl((void*)argument);
  /* USER CODE END StartSensorTask */
}

/* USER CODE BEGIN Header_StartUITask */
/**
  * @brief  Function implementing the Task_UI thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartUITask */
void StartUITask(void const * argument)
{
  /* USER CODE BEGIN StartUITask */
  /* 调用 main.c 中实现的 UI 显示任务 */
  extern void StartUITask_Impl(void *argument);
  StartUITask_Impl((void*)argument);
  /* USER CODE END StartUITask */
}

/* USER CODE BEGIN Header_StartBluetoothTask */
/**
  * @brief  Function implementing the Task_Bluetooth thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartBluetoothTask */
void StartBluetoothTask(void const * argument)
{
  /* USER CODE BEGIN StartBluetoothTask */
  /* 调用 main.c 中实现的蓝牙任务 */
  extern void StartBluetoothTask_Impl(void *argument);
  StartBluetoothTask_Impl((void*)argument);
  /* USER CODE END StartBluetoothTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
