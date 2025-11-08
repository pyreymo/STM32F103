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
#include "face.h"
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
static inline void UART_Printf(UART_HandleTypeDef* huart, const char* fmt, ...);
static void DrawThermometer(u8g2_t *u8g2, int32_t temp_int, int x, int y);
static void DrawHumidity(u8g2_t *u8g2, int32_t humi_int, int x, int y);
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

  /* 试了半天，发现硬件实际上是sh1106 */
  u8g2_Setup_sh1106_i2c_128x64_noname_f_hal(&u8g2, U8G2_R0);
  u8g2_InitDisplay(&u8g2);
  u8g2_SetPowerSave(&u8g2, 0);

  Face_Init();

  UART_Printf(&huart1, "[USER] SSD1306 display initialized\r\n");

  /* Infinite loop */
  for (;;) {
    uint32_t currentTime = osKernelGetTickCount();

    Face_Update(currentTime);

    status = osMessageQueueGet(sensorDataQueueHandle, &receivedData, NULL, 16);

    if (status == osOK || status == osErrorTimeout) {
      if (osMutexAcquire(screenUpdateMutexHandle, 100) == osOK) {
        u8g2_ClearBuffer(&u8g2);

        Face_Draw(&u8g2, currentTime);

        u8g2_SendBuffer(&u8g2);

        if (osMutexRelease(screenUpdateMutexHandle) != osOK) {
          UART_Printf(&huart1, "[USER] [FATAL] screenUpdateMutexHandle release failed!\r\n");
          break;
        }
      }
    }
  }
  /* USER CODE END StartDisplayTask */
}

/* Private application code --------------------------------------------------*/
static inline void UART_Printf(UART_HandleTypeDef* huart, const char* fmt, ...) {
  static char uart_buffer[256];

  va_list args;
  va_start(args, fmt);

  int len = vsnprintf(uart_buffer, sizeof(uart_buffer), fmt, args);

  va_end(args);

  if (len > 0) {
    HAL_UART_Transmit(huart, (uint8_t*)uart_buffer, len, HAL_MAX_DELAY);
  }
}

// 水滴/湿度 图标 (16x16)
static const uint8_t image_humidity_drop_bits[] = {
    0x20, 0x00, 0x20, 0x00, 0x30, 0x00, 0x70, 0x00, 0x78, 0x00, 0xf8, 0x00, 
    0xfc, 0x01, 0xfc, 0x01, 0x7e, 0x03, 0xfe, 0x02, 0xff, 0x06, 0xff, 0x07, 
    0xfe, 0x03, 0xfe, 0x03, 0xfc, 0x01, 0xf0, 0x00
};

// 温度计 图标 (16x16)
static const uint8_t image_thermometer_bits[] = {
    0x38, 0x00, 0x44, 0x40, 0xd4, 0xa0, 0x54, 0x40, 0xd4, 0x1c, 0x54, 0x06, 
    0xd4, 0x02, 0x54, 0x02, 0x54, 0x06, 0x92, 0x1c, 0x39, 0x01, 0x75, 0x01, 
    0x7d, 0x01, 0x39, 0x01, 0x82, 0x00, 0x7c, 0x00
};

static void DrawThermometer(u8g2_t* u8g2, int32_t temp_int, int x, int y) {
  char temp_str[16];

  u8g2_SetBitmapMode(u8g2, 1);
  u8g2_SetFontMode(u8g2, 1);

  // 绘制温度图标
  u8g2_DrawXBM(u8g2, x, y, 16, 16, image_thermometer_bits);

  // 绘制温度数值和单位
  u8g2_SetFont(u8g2, u8g2_font_helvB12_tf);
  snprintf(temp_str, sizeof(temp_str), "%ld°C", temp_int);
  u8g2_DrawUTF8(u8g2, x + 20, y + 15, temp_str);
}

static void DrawHumidity(u8g2_t* u8g2, int32_t humi_int, int x, int y) {
  char humi_str[16];

  u8g2_SetBitmapMode(u8g2, 1);
  u8g2_SetFontMode(u8g2, 1);

  // 绘制湿度图标
  u8g2_DrawXBM(u8g2, x, y, 16, 16, image_humidity_drop_bits);

  // 绘制湿度数值和单位
  u8g2_SetFont(u8g2, u8g2_font_helvB12_tf);
  snprintf(humi_str, sizeof(humi_str), "%ld%%", humi_int);
  u8g2_DrawStr(u8g2, x + 20, y + 15, humi_str);
}
/* USER CODE END Application */

