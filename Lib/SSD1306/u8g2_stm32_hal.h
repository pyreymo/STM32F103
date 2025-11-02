/**
  ******************************************************************************
  * @file    u8g2_stm32_hal.h
  * @brief   U8g2 hardware abstraction layer for STM32 HAL
  ******************************************************************************
  */

#ifndef __U8G2_STM32_HAL_H
#define __U8G2_STM32_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "u8g2.h"
#include "i2c.h"

/* Function prototypes -------------------------------------------------------*/
uint8_t u8x8_byte_stm32_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8x8_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

void u8g2_Setup_ssd1306_i2c_128x64_noname_f_hal(u8g2_t *u8g2, const u8g2_cb_t *rotation);

#ifdef __cplusplus
}
#endif

#endif /* __U8G2_STM32_HAL_H */