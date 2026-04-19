/**
 * @file qmi8658_driver.c
 * @brief QMI8658：寄存器读与探测（共用 g_i2c_bus0）。
 */
#include "qmi8658_driver.h"
#include "../i2c/i2c_bus_bb.h"
#include "../../board/board_pins.h"

#define QMI8658_REG_WHO_AM_I 0x00u
#define QMI8658_WHO_AM_I_EXPECT 0x05u

static qmi8658_data_ready_cb_t s_data_ready_cb;

int qmi8658_hw_probe(void)
{
    u8 tx = QMI8658_REG_WHO_AM_I;
    u8 rx = 0;

    if (i2c_bus_bb_write_read7(BOARD_IMU_I2C_ADDR7, &tx, 1, &rx, 1) != 0) {
        return 0;
    }
    return rx == QMI8658_WHO_AM_I_EXPECT ? 1 : 0;
}

void qmi8658_driver_init_default(void)
{
    /* 后续：量程、ODR、FIFO 等 */
}

int qmi8658_driver_read_reg_u8(u8 reg, u8 *out)
{
    u8 tx;

    if (out == NULL) {
        return -1;
    }
    tx = reg;
    return i2c_bus_bb_write_read7(BOARD_IMU_I2C_ADDR7, &tx, 1, out, 1);
}

void qmi8658_driver_register_data_ready(qmi8658_data_ready_cb_t cb)
{
    s_data_ready_cb = cb;
    (void)s_data_ready_cb;
}
