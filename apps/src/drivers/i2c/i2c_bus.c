/**
 * @file i2c_bus.c
 * @brief GPIO模拟I2C驱动实现 - 供OLED、QMI8658等设备共用
 */
#include "i2c_bus.h"
#include "../../board/board_pins.h"
#include "asm/iic_soft.h"
#include "asm/iic_hw.h"
#include "os/os_cpu.h"
#include "system/includes.h"

#define I2C_BUS_INDEX 0
#define HW_I2C_BUS_INDEX 0

// 通过定义USE_HARDWARE_I2C宏来选择使用硬件I2C还是软件I2C
#define USE_HARDWARE_I2C

#ifdef USE_HARDWARE_I2C
    #define IIC_START                  hw_iic_start
    #define IIC_STOP                   hw_iic_stop
    #define IIC_TX_BYTE                hw_iic_tx_byte
    #define IIC_RX_BYTE                hw_iic_rx_byte
    #define IIC_WRITE_BUF              hw_iic_write_buf
    #define IIC_READ_BUF               hw_iic_read_buf
    #define IIC_INIT                   hw_iic_init
    #define IIC_INDEX                  HW_I2C_BUS_INDEX
#else
    #define IIC_START                  soft_iic_start
    #define IIC_STOP                   soft_iic_stop
    #define IIC_TX_BYTE                soft_iic_tx_byte
    #define IIC_RX_BYTE                soft_iic_rx_byte
    #define IIC_WRITE_BUF              soft_iic_write_buf
    #define IIC_READ_BUF               soft_iic_read_buf
    #define IIC_INIT                   soft_iic_init
    #define IIC_INDEX                  I2C_BUS_INDEX
#endif

const struct soft_iic_config soft_iic_cfg[] = {
    {
        .scl = BOARD_I2C_SCL,
        .sda = BOARD_I2C_SDA,
        .delay = 1,
        .io_pu = 0,
    }};

const struct hw_iic_config hw_iic_cfg[] = {
    //iic0 data
    {
        //         SCL          SDA
        .port = {BOARD_I2C_SCL, BOARD_I2C_SDA},
        .baudrate = 100000,      //IIC通讯波特率
        .hdrive = 0,             //是否打开IO口强驱
        .io_filter = 1,          //是否打开滤波器（去纹波）
        .io_pu = 1,              //是否打开上拉电阻，如果外部电路没有焊接上拉电阻需要置1
        .role = IIC_MASTER,
    },
};

void board_i2c_bus0_init(void)
{
    IIC_INIT(IIC_INDEX);
}

int i2c_bus_write_reg8(u8 addr7, u8 reg, u8 data)
{
    OS_ENTER_CRITICAL();

    IIC_START(IIC_INDEX);

    u8 ack = IIC_TX_BYTE(IIC_INDEX, (u8)((addr7 << 1) | 0));
    if (ack != 1)
    {
        IIC_STOP(IIC_INDEX);
        OS_EXIT_CRITICAL();
        return -2;
    }

    ack = IIC_TX_BYTE(IIC_INDEX, reg);
    if (ack != 1)
    {
        IIC_STOP(IIC_INDEX);
        OS_EXIT_CRITICAL();
        return -3;
    }

    ack = IIC_TX_BYTE(IIC_INDEX, data);
    if (ack != 1)
    {
        IIC_STOP(IIC_INDEX);
        OS_EXIT_CRITICAL();
        return -3;
    }

    IIC_STOP(IIC_INDEX);
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

    IIC_START(IIC_INDEX);

    u8 ack = IIC_TX_BYTE(IIC_INDEX, (u8)((addr7 << 1) | 0));
    if (ack != 1)
    {
        IIC_STOP(IIC_INDEX);
        OS_EXIT_CRITICAL();
        return -2;
    }

    int ret = IIC_WRITE_BUF(IIC_INDEX, tx, tx_len);
    IIC_STOP(IIC_INDEX);
    OS_EXIT_CRITICAL();

    return ret;
}

int i2c_bus_read_reg8(u8 addr7, u8 reg)
{
    u8 data;

    OS_ENTER_CRITICAL();

    IIC_START(IIC_INDEX);

    u8 ack = IIC_TX_BYTE(IIC_INDEX, (u8)(addr7 << 1));
    if (ack != 1)
    {
        IIC_STOP(IIC_INDEX);
        OS_EXIT_CRITICAL();
        return -2;
    }

    ack = IIC_TX_BYTE(IIC_INDEX, reg);
    if (ack != 1)
    {
        IIC_STOP(IIC_INDEX);
        OS_EXIT_CRITICAL();
        return -3;
    }

    IIC_START(IIC_INDEX);

    ack = IIC_TX_BYTE(IIC_INDEX, (u8)((addr7 << 1) | 1));
    if (ack != 1)
    {
        IIC_STOP(IIC_INDEX);
        OS_EXIT_CRITICAL();
        return -2;
    }

    data = IIC_RX_BYTE(IIC_INDEX, 0);
    IIC_STOP(IIC_INDEX);

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

    IIC_START(IIC_INDEX);

    u8 ack = IIC_TX_BYTE(IIC_INDEX, (u8)(addr7 << 1));
    if (ack != 1)
    {
        IIC_STOP(IIC_INDEX);
        OS_EXIT_CRITICAL();
        return -2;
    }

    ack = IIC_TX_BYTE(IIC_INDEX, reg);
    if (ack != 1)
    {
        IIC_STOP(IIC_INDEX);
        OS_EXIT_CRITICAL();
        return -3;
    }

    IIC_START(IIC_INDEX);

    ack = IIC_TX_BYTE(IIC_INDEX, (u8)((addr7 << 1) | 1));
    if (ack != 1)
    {
        IIC_STOP(IIC_INDEX);
        OS_EXIT_CRITICAL();
        return -2;
    }

    if (rx_len > 1)
    {
        for (int i = 0; i < rx_len - 1; i++)
        {
            rx[i] = IIC_RX_BYTE(IIC_INDEX, 1);
        }
        rx[rx_len - 1] = IIC_RX_BYTE(IIC_INDEX, 0);
    }
    else
    {
        rx[0] = IIC_RX_BYTE(IIC_INDEX, 0);
    }

    IIC_STOP(IIC_INDEX);
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

    IIC_START(IIC_INDEX);

    u8 ack = IIC_TX_BYTE(IIC_INDEX, (u8)((addr7 << 1) | 0));
    if (ack != 1)
    {
        IIC_STOP(IIC_INDEX);
        OS_EXIT_CRITICAL();
        return -2;
    }

    int ret = IIC_WRITE_BUF(IIC_INDEX, tx, tx_len);
    if (ret < 0)
    {
        IIC_STOP(IIC_INDEX);
        OS_EXIT_CRITICAL();
        return ret;
    }

    IIC_START(IIC_INDEX);

    ack = IIC_TX_BYTE(IIC_INDEX, (u8)((addr7 << 1) | 1));
    if (ack != 1)
    {
        IIC_STOP(IIC_INDEX);
        OS_EXIT_CRITICAL();
        return -2;
    }

    ret = IIC_READ_BUF(IIC_INDEX, rx, rx_len);
    IIC_STOP(IIC_INDEX);

    OS_EXIT_CRITICAL();

    return ret;
}

void i2c_bus_scan(void)
{
    int ret = 0;
    u8 count = 0;
    printf("i2c_bus_scan\n");

    // ✅ 优化：只扫描常用地址范围（0x08-0x77），跳过保留地址
    // 0x00-0x07: 保留地址
    // 0x78-0x7F: 10位地址保留
    for (u8 addr7 = 0x08; addr7 < 0x78; addr7++)
    {
        OS_ENTER_CRITICAL();

        IIC_START(IIC_INDEX);
        ret = IIC_TX_BYTE(IIC_INDEX, (u8)((addr7 << 1) | 0));
        IIC_STOP(IIC_INDEX);

        OS_EXIT_CRITICAL();

        if (ret == 1)
        {
            printf("i2c addr7: 0x%02x\n", addr7);
            count++;
        }
    }
    printf("i2c_bus_scan count: %d\n", count);
}