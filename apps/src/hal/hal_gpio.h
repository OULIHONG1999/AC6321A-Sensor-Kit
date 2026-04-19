/**
 * @file hal_gpio.h
 * @brief 与 MCU/SDK 相关的 GPIO 抽象。换平台时替换实现文件，接口保持不变。
 */
#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include "asm/cpu.h"
typedef int hal_pin_t;
#define HAL_PIN_NONE (-1)

int hal_pin_valid(hal_pin_t pin);
void hal_gpio_write(hal_pin_t pin, int level_high);
void hal_gpio_i2c_bb_init(hal_pin_t scl, hal_pin_t sda);
void hal_gpio_input_floating(hal_pin_t pin);
u8 hal_gpio_read(hal_pin_t pin);
void hal_gpio_direction_output(hal_pin_t pin, int level_high);

#endif /* HAL_GPIO_H */
