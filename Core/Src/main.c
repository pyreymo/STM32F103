/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DHT11.h"
#include "ssd1306_fonts.h"
#include "ssd1306_tests.h"

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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static inline void OLED_Printf(uint8_t x, uint8_t y, SSD1306_Font_t font, SSD1306_COLOR color, const char* fmt, ...) {
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

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  /* 启动日志 */
  UART_Printf(&huart1, "[USER] STM32 USART1 is working!\r\n");

  /* 初始化DHT相关 */
  DHT11_InitTypeDef dht;
  HAL_DHT11_Init(&dht, DHT11_GPIO_Port, DHT11_Pin, &htim2);

  /* 初始化屏幕 */
  ssd1306_Init();
  // ssd1306_TestDrawBitmap();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    /* LED 闪烁 */
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

    /* 获取 DHT11 读数 */
    (void)HAL_DHT11_ReadData(&dht);

    int32_t humi_int, humi_frac;
    int32_t temp_int, temp_frac;
    float_to_int_parts(dht.Humidity, &humi_int, &humi_frac);
    float_to_int_parts(dht.Temperature, &temp_int, &temp_frac);

    ssd1306_Fill(Black);

    OLED_Printf(0, 0, Font_7x10, White, "System Info");
    OLED_Printf(0, 20, Font_7x10, White, "Humi: %ld.%02ld %%", humi_int, humi_frac);
    OLED_Printf(0, 32, Font_7x10, White, "Temp: %ld.%02ld C", temp_int, temp_frac);

    ssd1306_UpdateScreen();

    HAL_Delay(500);
  }

  HAL_DHT11_DeInit(&dht);
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
