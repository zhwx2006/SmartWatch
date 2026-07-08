/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"          // ★ 原生队列 API
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>    // 添加：sprintf
#include <string.h>   // 添加：strlen
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

/* USER CODE END Variables */

// ============================================================
// 外部声明：USART1 句柄（定义在 main.c 或 usart.c）
// ============================================================
extern UART_HandleTypeDef huart1;

// ============================================================
// FreeRTOS 句柄
// ============================================================
osThreadId taskTimeKeepHandle;
osThreadId taskSensorHandle;
osThreadId taskDisplayHandle;
osThreadId taskMenuHandle;
osThreadId taskBluetoothHandle;
osMessageQId queueSensorDataHandle;
osMessageQId queueSensorDataBtHandle;
osMessageQId queueTimeUpdateHandle;
osMessageQId queueMenuMsgHandle;
osMessageQId queueBtCmdHandle;
osMutexId mutexI2CHandle;
osSemaphoreId semDisplayUpdateHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

// ============================================================
// 任务函数声明（实现在 main.c 中）
// ============================================================
extern void vTask_TimeKeep(void const * argument);
extern void vTask_Sensor(void const * argument);
extern void vTask_Display(void const * argument);
extern void vTask_Menu(void const * argument);
extern void vTask_Bluetooth(void const * argument);

/* USER CODE END FunctionPrototypes */

void MX_FREERTOS_Init(void);

/* GetIdleTaskMemory prototype */
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
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Create the mutex(es) */
  osMutexDef(mutexI2C);
  mutexI2CHandle = osMutexCreate(osMutex(mutexI2C));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  osSemaphoreDef(semDisplayUpdate);
  semDisplayUpdateHandle = osSemaphoreCreate(osSemaphore(semDisplayUpdate), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* USER CODE END RTOS_TIMERS */

  /* ============================================================
     ★★★ 使用原生 FreeRTOS API 手动创建队列 ★★★
     ============================================================ */
  /* USER CODE BEGIN RTOS_QUEUES */

  // 传感器队列：4 个消息，每个消息大小 = SensorData_t
  queueSensorDataHandle = (osMessageQId)xQueueCreate(4, sizeof(SensorData_t));

  // 蓝牙专用传感器队列
  queueSensorDataBtHandle = (osMessageQId)xQueueCreate(4, sizeof(SensorData_t));

  // 时间更新队列
  queueTimeUpdateHandle = (osMessageQId)xQueueCreate(2, sizeof(TimeData_t));

  // 菜单消息队列
  queueMenuMsgHandle = (osMessageQId)xQueueCreate(4, sizeof(MenuMsg_t));

  // 蓝牙命令队列
  queueBtCmdHandle = (osMessageQId)xQueueCreate(4, 32);

  // 打印队列地址（调试用）
  char dbg[64];
  sprintf(dbg, "Q addr: %p\r\n", (void*)queueSensorDataHandle);
  HAL_UART_Transmit(&huart1, (uint8_t*)dbg, strlen(dbg), 100);

  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  osThreadDef(taskTimeKeep, vTask_TimeKeep, osPriorityHigh, 0, 256);
  taskTimeKeepHandle = osThreadCreate(osThread(taskTimeKeep), NULL);

  osThreadDef(taskSensor, vTask_Sensor, osPriorityAboveNormal, 0, 512);
  taskSensorHandle = osThreadCreate(osThread(taskSensor), NULL);

  osThreadDef(taskDisplay, vTask_Display, osPriorityNormal, 0, 1024);
  taskDisplayHandle = osThreadCreate(osThread(taskDisplay), NULL);

  osThreadDef(taskMenu, vTask_Menu, osPriorityNormal, 0, 512);
  taskMenuHandle = osThreadCreate(osThread(taskMenu), NULL);

  osThreadDef(taskBluetooth, vTask_Bluetooth, osPriorityBelowNormal, 0, 512);
  taskBluetoothHandle = osThreadCreate(osThread(taskBluetooth), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* USER CODE END RTOS_THREADS */
}
