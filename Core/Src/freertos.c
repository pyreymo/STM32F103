/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DHT11.h"
#include "tim.h"
#include "u8g2.h"
#include "u8g2_stm32_hal.h"
#include "usart.h"
#include "face_wrapper.h"
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
static u8g2_t u8g2;
/* USER CODE END Variables */
/* Definitions for ledTask */
osThreadId_t ledTaskHandle;
const osThreadAttr_t ledTask_attributes = {
  .name = "ledTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for sensorTask */
osThreadId_t sensorTaskHandle;
const osThreadAttr_t sensorTask_attributes = {
  .name = "sensorTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for displayTask */
osThreadId_t displayTaskHandle;
const osThreadAttr_t displayTask_attributes = {
  .name = "displayTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for sensorDataQueue */
osMessageQueueId_t sensorDataQueueHandle;
const osMessageQueueAttr_t sensorDataQueue_attributes = {
  .name = "sensorDataQueue"
};
/* Definitions for screenUpdateMutex */
osMutexId_t screenUpdateMutexHandle;
const osMutexAttr_t screenUpdateMutex_attributes = {
  .name = "screenUpdateMutex"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartLedTask(void *argument);
void StartSensorTask(void *argument);
void StartDisplayTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  UART_Printf(&huart1, "[USER] STM32 USART1 is working!\r\n");
  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of screenUpdateMutex */
  screenUpdateMutexHandle = osMutexNew(&screenUpdateMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of sensorDataQueue */
  sensorDataQueueHandle = osMessageQueueNew (1, 8, &sensorDataQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of ledTask */
  ledTaskHandle = osThreadNew(StartLedTask, NULL, &ledTask_attributes);

  /* creation of sensorTask */
  sensorTaskHandle = osThreadNew(StartSensorTask, NULL, &sensorTask_attributes);

  /* creation of displayTask */
  displayTaskHandle = osThreadNew(StartDisplayTask, NULL, &displayTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartLedTask */
/**
  * @brief  Blink the LED every 500ms
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartLedTask */
void StartLedTask(void *argument)
{
  /* USER CODE BEGIN StartLedTask */
  /* Infinite loop */
  for(;;)
  {
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    osDelay(500);
  }
  /* USER CODE END StartLedTask */
}

/* USER CODE BEGIN Header_StartSensorTask */
/**
* @brief Read humidity and temperature from DHT11 sensor every 2s
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSensorTask */
void StartSensorTask(void *argument)
{
  /* USER CODE BEGIN StartSensorTask */
  DHT11_InitTypeDef dht;
  HAL_DHT11_Init(&dht, DHT11_GPIO_Port, DHT11_Pin, &htim2);
  DHT11_DATA_S sensorData;
  DHT11_StatusTypeDef ret;

  /* Infinite loop */
  for (;;) {
    ret = HAL_DHT11_ReadData(&dht);
    if (ret == DHT11_OK) {
      sensorData.humidity = dht.Humidity;
      sensorData.temperature = dht.Temperature;

      osMessageQueuePut(sensorDataQueueHandle, &sensorData, 0U, 0U);
    } else {
      UART_Printf(&huart1, "[USER] read data from DHT11 failed, ret %u\r\n", ret);
    }

    osDelay(2000);
  }

  HAL_DHT11_DeInit(&dht);
  /* USER CODE END StartSensorTask */
}

/* USER CODE BEGIN Header_StartDisplayTask */
/**
* @brief Function implementing the displayTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDisplayTask */
void StartDisplayTask(void *argument)
{
  /* USER CODE BEGIN StartDisplayTask */
  DHT11_DATA_S receivedData = {0};
  osStatus_t status;

  u8g2_Setup_sh1106_i2c_128x64_noname_f_hal(&u8g2, U8G2_R0);
  u8g2_InitDisplay(&u8g2);
  u8g2_SetPowerSave(&u8g2, 0);

  FaceHandle myFace = Face_Create();
  if (myFace == NULL) {
    UART_Printf(&huart1, "[USER] Face creation failed!\r\n");
    osThreadTerminate(NULL);
  }
  Face_Init(myFace);

  UART_Printf(&huart1, "[USER] SH1106 display initialized\r\n");

  const uint32_t FRAME_RATE = 20;
  const uint32_t FRAME_PERIOD_MS = 1000 / FRAME_RATE;

  uint32_t lastWakeTime = osKernelGetTickCount();

  /* Infinite loop */
  for (;;) {
    uint32_t nextWakeTime = lastWakeTime + FRAME_PERIOD_MS;
    uint32_t currentTime = osKernelGetTickCount();

    if (nextWakeTime > currentTime) {
      osDelay(nextWakeTime - currentTime);
    }

    lastWakeTime = nextWakeTime;

    currentTime = osKernelGetTickCount();

    status = osMessageQueueGet(sensorDataQueueHandle, &receivedData, NULL, 0);
    if (status == osOK) {  // do nothing for now
    }

    Face_Update(myFace, currentTime);

    if (osMutexAcquire(screenUpdateMutexHandle, 10) == osOK) {
      u8g2_ClearBuffer(&u8g2);
      Face_Draw(myFace, &u8g2, currentTime);
      u8g2_SendBuffer(&u8g2);

      if (osMutexRelease(screenUpdateMutexHandle) != osOK) {
        UART_Printf(&huart1, "[USER] [FATAL] screenUpdateMutexHandle release failed!\r\n");
        break;
      }
    } else {
      UART_Printf(&huart1, "[USER] [WARN] Skip frame, mutex busy.\r\n");
    }

    // UART_Printf(&huart1, "Frame time: %d ms\r\n", osKernelGetTickCount() - currentTime);
  }

  Face_Destroy(myFace);
  /* USER CODE END StartDisplayTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

