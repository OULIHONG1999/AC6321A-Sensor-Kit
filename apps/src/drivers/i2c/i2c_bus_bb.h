/**
 * @file i2c_bus_bb.h
 * @brief 共享 GPIO 模拟 I2C 总线（与具体从机无关）。OLED、QMI8658 等驱动共用。
 */
#ifndef I2C_BUS_BB_H
#define I2C_BUS_BB_H

#include "../../hal/hal_gpio.h"
#include "typedef.h"

void board_i2c_bus0_init(void);

int i2c_bus_bb_write7(u8 addr7, const u8 *tx, unsigned tx_len);

int i2c_bus_bb_write_read7(u8 addr7, const u8 *tx, unsigned tx_len, u8 *rx,
                           unsigned rx_len);
void i2c_bus_bb_scan(void);

#endif /* I2C_BUS_BB_H */
