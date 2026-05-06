#ifndef U8G2_PORT_H
#define U8G2_PORT_H

#include "system/includes.h"
#include "../../../hal/hal_gpio.h"
#include "../../../drivers/i2c/i2c_bus.h"
#include "../csrc/u8g2.h"

// 使用现有的 I2C 总线，无需重新定义引脚
// I2C 地址已在 board_pins.h 中定义为 BOARD_OLED_I2C_ADDR7 (0x3C)

// 延迟函数声明
void u8g2_delay_ms(uint32_t ms);
void u8g2_delay_us(uint32_t us);

// u8g2 回调函数声明
uint8_t u8g2_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8g2_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

#endif /* U8G2_PORT_H */
