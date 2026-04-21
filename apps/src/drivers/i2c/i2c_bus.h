/**
 * @file i2c_bus.h
 * @brief GPIO模拟I2C总线驱动 - 供OLED、QMI8658等设备共用
 */
#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "../../hal/hal_gpio.h"
#include "typedef.h"

void board_i2c_bus0_init(void);

int i2c_bus_write_reg8(u8 addr7, u8 reg, u8 data);

int i2c_bus_write_buf(u8 addr7, const u8 *tx, unsigned tx_len);

int i2c_bus_read_reg8(u8 addr7, u8 reg);

int i2c_bus_read_buf(u8 addr7, u8 reg, u8 *rx, unsigned rx_len);

int i2c_bus_write_read(u8 addr7, const u8 *tx, unsigned tx_len, u8 *rx,
                       unsigned rx_len);

void i2c_bus_scan(void);

#endif /* I2C_BUS_H */