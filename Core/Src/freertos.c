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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "DHT11.h"
#include "ssd1306_fonts.h"
#include "tim.h"
#include "usart.h"
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
static inline void ssd1306_Printf(uint8_t x, uint8_t y, SSD1306_Font_t font, SSD1306_COLOR color, const char* fmt, ...);
static inline void UART_Printf(UART_HandleTypeDef* huart, const char* fmt, ...);
static inline void float_to_int_parts(float f, int32_t* int_part, int32_t* frac_part);
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
  DHT11_DATA_S receivedData;
  osStatus_t status;

  int32_t humi_int, humi_frac;
  int32_t temp_int, temp_frac;

  ssd1306_Init();

  /* Infinite loop */
  for (;;) {
    /* 仅当 */
    status = osMessageQueueGet(sensorDataQueueHandle, &receivedData, NULL, osWaitForever);

    if (status == osOK) {
      float_to_int_parts(receivedData.humidity, &humi_int, &humi_frac);
      float_to_int_parts(receivedData.temperature, &temp_int, &temp_frac);

      ssd1306_Fill(Black);

      ssd1306_Printf(0, 0, Font_7x10, White, "System Info");
      ssd1306_Printf(0, 20, Font_7x10, White, "Humi: %ld.%02ld %%", humi_int, humi_frac);
      ssd1306_Printf(0, 32, Font_7x10, White, "Temp: %ld.%02ld C", temp_int, temp_frac);

      if (osMutexAcquire(screenUpdateMutexHandle, 0) == osOK) {
          ssd1306_UpdateScreen();
          if (osMutexRelease(screenUpdateMutexHandle) != osOK) {
              UART_Printf(&huart1, "[USER] [FATAL] screenUpdateMutexHandle release failed!\r\n");
              break;
          }
      }

      UART_Printf(&huart1, "[USER] screen updated!\r\n");
    }
  }
  /* USER CODE END StartDisplayTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
static inline void ssd1306_Printf(uint8_t x, uint8_t y, SSD1306_Font_t font, SSD1306_COLOR color, const char* fmt, ...) {
    static char oled_buffer[128];

    va_list args;
    va_start(args, fmt);
    vsnprintf(oled_buffer, sizeof(oled_buffer), fmt, args);
    va_end(args);

    ssd1306_SetCursor(x, y);
    ssd1306_WriteString(oled_buffer, font, color);
}

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

static inline void float_to_int_parts(float f, int32_t* int_part, int32_t* frac_part) {
    *int_part = (int32_t)f;
    *frac_part = abs((int32_t)((f * 100.0f)) % 100);
}
/* USER CODE END Application */

