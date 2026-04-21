/**
 * @file i2c_bus.c
 * @brief GPIO模拟I2C驱动实现 - 供OLED、QMI8658等设备共用
 */
#include "i2c_bus.h"
#include "../../board/board_pins.h"
#include "asm/iic_soft.h"
#include "os/os_cpu.h"
#include "system/includes.h"

#define I2C_BUS_INDEX 0

const struct soft_iic_config soft_iic_cfg[] = {
    {
        .scl = BOARD_I2C_SCL,
        .sda = BOARD_I2C_SDA,
        .delay = 1,
        .io_pu = 0,
    }};

void board_i2c_bus0_init(void)
{
    soft_iic_init(I2C_BUS_INDEX);
}

int i2c_bus_write_reg8(u8 addr7, u8 reg, u8 data)
{
    OS_ENTER_CRITICAL();

    soft_iic_start(I2C_BUS_INDEX);

    u8 ack = soft_iic_tx_byte(I2C_BUS_INDEX, (u8)((addr7 << 1) | 0));
    if (ack != 1)
    {
        soft_iic_stop(I2C_BUS_INDEX);
        OS_EXIT_CRITICAL();
        return -2;
    }

    ack = soft_iic_tx_byte(I2C_BUS_INDEX, reg);
    if (ack != 1)
    {
        soft_iic_stop(I2C_BUS_INDEX);
        OS_EXIT_CRITICAL();
        return -3;
    }

    ack = soft_iic_tx_byte(I2C_BUS_INDEX, data);
    if (ack != 1)
    {
        soft_iic_stop(I2C_BUS_INDEX);
        OS_EXIT_CRITICAL();
        return -3;
    }

    soft_iic_stop(I2C_BUS_INDEX);
    OS_EXIT_CRITICAL();

    return 0;
}

int i2c_bus_write_buf(u8 addr7, const u8 *tx, unsigned tx_len)
{
    if (tx_len == 0 || tx == NULL)
    {
        return -1;
    }

    OS_ENTER_CRITICAL();

    soft_iic_start(I2C_BUS_INDEX);

    u8 ack = soft_iic_tx_byte(I2C_BUS_INDEX, (u8)((addr7 << 1) | 0));
    if (ack != 1)
    {
        soft_iic_stop(I2C_BUS_INDEX);
        OS_EXIT_CRITICAL();
        return -2;
    }

    int ret = soft_iic_write_buf(I2C_BUS_INDEX, tx, tx_len);
    soft_iic_stop(I2C_BUS_INDEX);
    OS_EXIT_CRITICAL();

    return ret;
}

int i2c_bus_read_reg8(u8 addr7, u8 reg)
{
    u8 data;

    OS_ENTER_CRITICAL();

    soft_iic_start(I2C_BUS_INDEX);

    u8 ack = soft_iic_tx_byte(I2C_BUS_INDEX, (u8)(addr7 << 1));
    if (ack != 1)
    {
        soft_iic_stop(I2C_BUS_INDEX);
        OS_EXIT_CRITICAL();
        return -2;
    }

    ack = soft_iic_tx_byte(I2C_BUS_INDEX, reg);
    if (ack != 1)
    {
        soft_iic_stop(I2C_BUS_INDEX);
        OS_EXIT_CRITICAL();
        return -3;
    }

    soft_iic_start(I2C_BUS_INDEX);

    ack = soft_iic_tx_byte(I2C_BUS_INDEX, (u8)((addr7 << 1) | 1));
    if (ack != 1)
    {
        soft_iic_stop(I2C_BUS_INDEX);
        OS_EXIT_CRITICAL();
        return -2;
    }

    data = soft_iic_rx_byte(I2C_BUS_INDEX, 0);
    soft_iic_stop(I2C_BUS_INDEX);

    OS_EXIT_CRITICAL();

    return data;
}

int i2c_bus_read_buf(u8 addr7, u8 reg, u8 *rx, unsigned rx_len)
{
    if (rx_len == 0 || rx == NULL)
    {
        return -1;
    }

    OS_ENTER_CRITICAL();

    soft_iic_start(I2C_BUS_INDEX);

    u8 ack = soft_iic_tx_byte(I2C_BUS_INDEX, (u8)(addr7 << 1));
    if (ack != 1)
    {
        soft_iic_stop(I2C_BUS_INDEX);
        OS_EXIT_CRITICAL();
        return -2;
    }

    ack = soft_iic_tx_byte(I2C_BUS_INDEX, reg);
    if (ack != 1)
    {
        soft_iic_stop(I2C_BUS_INDEX);
        OS_EXIT_CRITICAL();
        return -3;
    }

    soft_iic_start(I2C_BUS_INDEX);

    ack = soft_iic_tx_byte(I2C_BUS_INDEX, (u8)((addr7 << 1) | 1));
    if (ack != 1)
    {
        soft_iic_stop(I2C_BUS_INDEX);
        OS_EXIT_CRITICAL();
        return -2;
    }

    if (rx_len > 1)
    {
        for (int i = 0; i < rx_len - 1; i++)
        {
            rx[i] = soft_iic_rx_byte(I2C_BUS_INDEX, 1);
        }
        rx[rx_len - 1] = soft_iic_rx_byte(I2C_BUS_INDEX, 0);
    }
    else
    {
        rx[0] = soft_iic_rx_byte(I2C_BUS_INDEX, 0);
    }

    soft_iic_stop(I2C_BUS_INDEX);
    OS_EXIT_CRITICAL();

    return 0;
}

int i2c_bus_write_read(u8 addr7, const u8 *tx, unsigned tx_len, u8 *rx,
                       unsigned rx_len)
{
    if (tx_len == 0 || tx == NULL || rx == NULL || rx_len == 0)
    {
        return -1;
    }

    OS_ENTER_CRITICAL();

    soft_iic_start(I2C_BUS_INDEX);

    u8 ack = soft_iic_tx_byte(I2C_BUS_INDEX, (u8)((addr7 << 1) | 0));
    if (ack != 1)
    {
        soft_iic_stop(I2C_BUS_INDEX);
        OS_EXIT_CRITICAL();
        return -2;
    }

    int ret = soft_iic_write_buf(I2C_BUS_INDEX, tx, tx_len);
    if (ret < 0)
    {
        soft_iic_stop(I2C_BUS_INDEX);
        OS_EXIT_CRITICAL();
        return ret;
    }

    soft_iic_start(I2C_BUS_INDEX);

    ack = soft_iic_tx_byte(I2C_BUS_INDEX, (u8)((addr7 << 1) | 1));
    if (ack != 1)
    {
        soft_iic_stop(I2C_BUS_INDEX);
        OS_EXIT_CRITICAL();
        return -2;
    }

    ret = soft_iic_read_buf(I2C_BUS_INDEX, rx, rx_len);
    soft_iic_stop(I2C_BUS_INDEX);

    OS_EXIT_CRITICAL();

    return ret;
}

void i2c_bus_scan(void)
{
    int ret = 0;
    u8 count = 0;
    printf("i2c_bus_scan\n");

    for (u8 addr7 = 0; addr7 < 128; addr7++)
    {
        OS_ENTER_CRITICAL();

        soft_iic_start(I2C_BUS_INDEX);
        ret = soft_iic_tx_byte(I2C_BUS_INDEX, (u8)((addr7 << 1) | 0));
        soft_iic_stop(I2C_BUS_INDEX);

        OS_EXIT_CRITICAL();

        if (ret == 1)
        {
            printf("i2c addr7: 0x%02x\n", addr7);
            count++;
        }
    }
    printf("i2c_bus_scan count: %d\n", count);
}