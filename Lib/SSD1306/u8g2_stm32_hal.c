/**
 ******************************************************************************
 * @file    u8g2_stm32_hal.c
 * @brief   U8g2 hardware abstraction layer implementation for STM32 HAL
 ******************************************************************************
 */

#include "u8g2_stm32_hal.h"

#include "main.h"

#define SSD1306_I2C_ADDRESS 0x78  // 0x3C << 1

/**
 * @brief  U8x8 byte-level I2C communication callback for STM32 HAL
 * @param  u8x8: U8x8 structure pointer
 * @param  msg: Message type
 * @param  arg_int: Integer argument
 * @param  arg_ptr: Pointer argument
 * @retval Status byte
 */
uint8_t u8x8_byte_stm32_hw_i2c(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
  static uint8_t buffer[32];
  static uint8_t buffer_index = 0;

  switch (msg) {
    case U8X8_MSG_BYTE_INIT:
      // I2C already initialized in MX_I2C1_Init()
      break;

    case U8X8_MSG_BYTE_SET_DC:
      // DC pin is not used in I2C mode
      break;

    case U8X8_MSG_BYTE_START_TRANSFER:
      buffer_index = 0;
      break;

    case U8X8_MSG_BYTE_SEND:
      // Copy data to buffer
      for (uint8_t i = 0; i < arg_int; i++) {
        if (buffer_index < sizeof(buffer)) {
          buffer[buffer_index++] = ((uint8_t*)arg_ptr)[i];
        }
      }
      break;

    case U8X8_MSG_BYTE_END_TRANSFER:
      // Send accumulated data via I2C
      if (buffer_index > 0) {
        HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDRESS, buffer, buffer_index, HAL_MAX_DELAY);
        buffer_index = 0;
      }
      break;

    default:
      return 0;
  }
  return 1;
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