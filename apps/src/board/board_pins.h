/**
 * @file board_pins.h
 * @brief 板级外设引脚：移植时主要改此文件（或做 board_pins_xxx.h 多板选择）。
 */
#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "asm/gpio.h"
#include "system/includes.h"

/* 电源使能引脚 */
#define BOARD_POWER_EN IO_PORTB_04

/* SSD1306 类 OLED，GPIO 模拟 I2C */
#define BOARD_I2C_SCL IO_PORTA_00
#define BOARD_I2C_SDA IO_PORTA_01
/* 无硬件复位脚时保持 -1，HAL 会跳过 RES 时序 */
#define BOARD_OLED_I2C_RES (-1)
/* SSD1306 7-bit 从机地址（与 0x78 写地址一致） */
#define BOARD_OLED_I2C_ADDR7 0x3c //78:0111 1000   7e:0111 1110   3d:0011 1101

/* 与 OLED 共用一组 SCL/SDA；
QMI8658 7-bit 地址依 SA0 接法，默认 0x6B */
#define BOARD_IMU_I2C_ADDR7 0x6A
// 添加中断引脚
#define BOARD_IMU_INT1 IO_PORTB_05
#define BOARD_IMU_INT2 IO_PORTB_06


#endif /* BOARD_PINS_H */
