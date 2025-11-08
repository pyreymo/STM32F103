/**
 ******************************************************************************
 * @file    u8g2_stm32_hal.c
 * @brief   U8g2 hardware abstraction layer implementation for STM32 HAL
 ******************************************************************************
 */

#include "u8g2_stm32_hal.h"

#include <string.h>

#include "main.h"

#define SSD1306_I2C_ADDRESS 0x78
#define MAX_I2C_BUFFER_SIZE 1025

static uint8_t i2c_buffer[MAX_I2C_BUFFER_SIZE];
static uint16_t i2c_buffer_index = 0;

uint8_t u8x8_byte_stm32_hw_i2c(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
  switch (msg) {
    case U8X8_MSG_BYTE_INIT:
      // I2C 已经在 main.c 中初始化
      // 信号量也已经在 main.c 中创建
      break;

    case U8X8_MSG_BYTE_SET_DC:
      // 在 I2C 模式下不使用 DC 引脚
      break;

    case U8X8_MSG_BYTE_START_TRANSFER:
      i2c_buffer_index = 0;  // 重置缓冲区索引
      break;

    case U8X8_MSG_BYTE_SEND:
      // 将数据拷贝到 DMA 缓冲区
      if (i2c_buffer_index + arg_int <= MAX_I2C_BUFFER_SIZE) {
        memcpy(&i2c_buffer[i2c_buffer_index], arg_ptr, arg_int);
        i2c_buffer_index += arg_int;
      }
      break;

    case U8X8_MSG_BYTE_END_TRANSFER:
      // 使用 DMA 发送整个累积的缓冲区
      if (i2c_buffer_index > 0) {
        // 启动 DMA 传输
        if (HAL_I2C_Master_Transmit_DMA(&hi2c1, SSD1306_I2C_ADDRESS, i2c_buffer, i2c_buffer_index) != HAL_OK) {
          // DMA 启动失败，可以在这里处理错误
          // 例如，可以尝试使用阻塞模式作为备用方案
          // HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDRESS, i2c_buffer, i2c_buffer_index, HAL_MAX_DELAY);
          return 0;  // 表示失败
        }

        // 等待 DMA 传输完成信号
        // osWaitForever 会让当前任务进入阻塞状态，不消耗 CPU，直到信号量被释放
        osStatus_t status = osSemaphoreAcquire(i2cDmaSemaphoreHandle, osWaitForever);
        if (status != osOK) {
          // 等待信号量时发生错误
          return 0;
        }
      }
      break;

    default:
      return 0;
  }
  return 1;
}

/**
 * @brief I2C Master Tx Transfer completed callback.
 *        这是 HAL 库在 DMA 传输成功完成后自动调用的中断回调函数。
 * @param hi2c I2C handle.
 * @retval None
 */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef* hi2c) {
  if (hi2c->Instance == hi2c1.Instance) {
    osSemaphoreRelease(i2cDmaSemaphoreHandle);
  }
}

/**
 * @brief  I2C Error Callback.
 * @param  hi2c Pointer to a I2C_HandleTypeDef structure that contains
 *                the configuration information for the specified I2C.
 * @retval None
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef* hi2c) {
  if (hi2c->Instance == hi2c1.Instance) {
    uint32_t error_code = HAL_I2C_GetError(hi2c);
    UART_Printf(&huart1, "I2C Error: 0x%X\r\n", error_code);
    osSemaphoreRelease(i2cDmaSemaphoreHandle);
  }
}

/**
 * @brief  U8x8 GPIO and delay callback for STM32 HAL
 * @param  u8x8: U8x8 structure pointer
 * @param  msg: Message type
 * @param  arg_int: Integer argument
 * @param  arg_ptr: Pointer argument
 * @retval Status byte
 */
uint8_t u8x8_gpio_and_delay_stm32(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
  switch (msg) {
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
      // GPIO already initialized by STM32CubeMX
      break;

    case U8X8_MSG_DELAY_NANO:
      // Not implemented for STM32 HAL
      break;

    case U8X8_MSG_DELAY_100NANO:
      // Not implemented for STM32 HAL
      break;

    case U8X8_MSG_DELAY_MILLI:
      HAL_Delay(arg_int);
      break;

    case U8X8_MSG_DELAY_I2C:
      // Small delay for I2C communication
      HAL_Delay(1);
      break;

    case U8X8_MSG_GPIO_I2C_CLOCK:
      // Clock pin is handled by HAL I2C
      break;

    case U8X8_MSG_GPIO_I2C_DATA:
      // Data pin is handled by HAL I2C
      break;

    default:
      break;
  }
  return 1;
}

/**
 * @brief  Setup U8g2 for SSD1306 128x64 display with I2C interface using STM32 HAL
 * @param  u8g2: U8g2 structure pointer
 * @param  rotation: Display rotation callback
 * @param  buf: Display buffer
 * @param  buf_size: Buffer size
 * @retval None
 */
void u8g2_Setup_ssd1306_i2c_128x64_noname_f_hal(u8g2_t* u8g2, const u8g2_cb_t* rotation) {
  u8g2_Setup_ssd1306_i2c_128x64_noname_f(u8g2, rotation, u8x8_byte_stm32_hw_i2c, u8x8_gpio_and_delay_stm32);
}

void u8g2_Setup_sh1106_i2c_128x64_noname_f_hal(u8g2_t* u8g2, const u8g2_cb_t* rotation) {
  u8g2_Setup_sh1106_i2c_128x64_noname_f(u8g2, rotation, u8x8_byte_stm32_hw_i2c, u8x8_gpio_and_delay_stm32);
}