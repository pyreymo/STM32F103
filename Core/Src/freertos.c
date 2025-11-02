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
static inline void float_to_int_parts(float f, int32_t* int_part, int32_t* frac_part);
static void DrawModernThermometer(u8g2_t *u8g2, int32_t temp_int, int32_t temp_frac, int x, int y);
static void DrawModernHumidity(u8g2_t *u8g2, int32_t humi_int, int32_t humi_frac, int x, int y);
static void DrawProgressBar(u8g2_t *u8g2, int x, int y, int width, int height, int percentage, uint8_t color);
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

  /* Initialize U8g2 for SSD1306 128x64 OLED display */
  u8g2_Setup_ssd1306_i2c_128x64_noname_f_hal(&u8g2, U8G2_R0);
  u8g2_InitDisplay(&u8g2);
  u8g2_SetPowerSave(&u8g2, 0);
  u8g2_ClearDisplay(&u8g2);

  UART_Printf(&huart1, "[USER] SSD1306 display initialized\r\n");

  /* Infinite loop */
  for (;;) {
    /* 队列存在信息时会解除阻塞；平时会阻塞在这里，不会占用CPU */
    status = osMessageQueueGet(sensorDataQueueHandle, &receivedData, NULL, osWaitForever);

    if (status == osOK) {
      float_to_int_parts(receivedData.humidity, &humi_int, &humi_frac);
      float_to_int_parts(receivedData.temperature, &temp_int, &temp_frac);

      if (osMutexAcquire(screenUpdateMutexHandle, 100) == osOK) {
        /* Clear the display buffer */
        u8g2_ClearBuffer(&u8g2);

        /* Draw modern UI elements */
        DrawModernThermometer(&u8g2, temp_int, temp_frac, 10, 10);
        DrawModernHumidity(&u8g2, humi_int, humi_frac, 10, 40);

        /* Draw title */
        u8g2_SetFont(&u8g2, u8g2_font_helvB12_tf);
        u8g2_DrawStr(&u8g2, 25, 8, "DHT11 Monitor");

        /* Draw separator line */
        u8g2_DrawHLine(&u8g2, 0, 15, 128);

        /* Send the buffer to the display */
        u8g2_SendBuffer(&u8g2);

        if (osMutexRelease(screenUpdateMutexHandle) != osOK) {
            UART_Printf(&huart1, "[USER] [FATAL] screenUpdateMutexHandle release failed!\r\n");
            break;
        }
      }

      UART_Printf(&huart1, "[USER] Temperature: %d.%02d°C, Humidity: %d.%02d%%\r\n",
                  temp_int, temp_frac, humi_int, humi_frac);
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

static inline void float_to_int_parts(float f, int32_t* int_part, int32_t* frac_part) {
    *int_part = (int32_t)f;
    *frac_part = abs((int32_t)((f * 100.0f)) % 100);
}

/**
  * @brief  Draw modern thermometer icon with temperature value
  * @param  u8g2: U8g2 structure pointer
  * @param  temp_int: Integer part of temperature
  * @param  temp_frac: Fractional part of temperature
  * @param  x: X position
  * @param  y: Y position
  * @retval None
  */
static void DrawModernThermometer(u8g2_t *u8g2, int32_t temp_int, int32_t temp_frac, int x, int y) {
    char temp_str[16];

    // Draw thermometer icon
    u8g2_DrawCircle(u8g2, x + 5, y + 20, 4, U8G2_DRAW_ALL);
    u8g2_DrawVLine(u8g2, x + 5, y + 5, 15);
    u8g2_DrawCircle(u8g2, x + 5, y + 10, 2, U8G2_DRAW_ALL);

    // Draw temperature value
    u8g2_SetFont(u8g2, u8g2_font_helvB12_tf);
    snprintf(temp_str, sizeof(temp_str), "%d.%02d", (int)temp_int, (int)temp_frac);
    u8g2_DrawStr(u8g2, x + 15, y + 15, temp_str);

    // Draw degree symbol and C
    u8g2_SetFont(u8g2, u8g2_font_helvB08_tf);
    u8g2_DrawCircle(u8g2, x + 45, y + 8, 1, U8G2_DRAW_ALL);
    u8g2_DrawStr(u8g2, x + 50, y + 15, "C");

    // Draw temperature progress bar (0-50°C range)
    int temp_percentage = (int)((temp_int * 100 + temp_frac) / 50); // Assuming 0-50°C range
    if (temp_percentage > 100) temp_percentage = 100;
    if (temp_percentage < 0) temp_percentage = 0;

    DrawProgressBar(u8g2, x + 15, y + 20, 40, 4, temp_percentage, 1);
}

/**
  * @brief  Draw modern humidity icon with humidity value
  * @param  u8g2: U8g2 structure pointer
  * @param  humi_int: Integer part of humidity
  * @param  humi_frac: Fractional part of humidity
  * @param  x: X position
  * @param  y: Y position
  * @retval None
  */
static void DrawModernHumidity(u8g2_t *u8g2, int32_t humi_int, int32_t humi_frac, int x, int y) {
    char humi_str[16];

    // Draw water drop icon
    u8g2_DrawCircle(u8g2, x + 5, y + 5, 4, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT);
    u8g2_DrawVLine(u8g2, x + 5, y + 5, 4);
    u8g2_DrawHLine(u8g2, x + 2, y + 8, 6);

    // Draw humidity value
    u8g2_SetFont(u8g2, u8g2_font_helvB12_tf);
    snprintf(humi_str, sizeof(humi_str), "%d.%02d", (int)humi_int, (int)humi_frac);
    u8g2_DrawStr(u8g2, x + 15, y + 15, humi_str);

    // Draw percentage symbol
    u8g2_SetFont(u8g2, u8g2_font_helvB08_tf);
    u8g2_DrawCircle(u8g2, x + 45, y + 8, 1, U8G2_DRAW_ALL);
    u8g2_DrawVLine(u8g2, x + 47, y + 5, 6);

    // Draw humidity progress bar (0-100% range)
    int humi_percentage = (int)humi_int;
    if (humi_percentage > 100) humi_percentage = 100;
    if (humi_percentage < 0) humi_percentage = 0;

    DrawProgressBar(u8g2, x + 15, y + 20, 40, 4, humi_percentage, 1);
}

/**
  * @brief  Draw progress bar
  * @param  u8g2: U8g2 structure pointer
  * @param  x: X position
  * @param  y: Y position
  * @param  width: Bar width
  * @param  height: Bar height
  * @param  percentage: Progress percentage (0-100)
  * @param  color: Bar color (0=background, 1=foreground)
  * @retval None
  */
static void DrawProgressBar(u8g2_t *u8g2, int x, int y, int width, int height, int percentage, uint8_t color) {
    int fill_width = (width * percentage) / 100;

    // Draw background
    u8g2_DrawFrame(u8g2, x, y, width, height);

    // Draw filled portion
    if (fill_width > 0) {
        u8g2_DrawBox(u8g2, x + 1, y + 1, fill_width - 2, height - 2);
    }
}
/* USER CODE END Application */

