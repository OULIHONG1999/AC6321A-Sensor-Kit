/**
 * @file i2c_bus_bb.c
 * @brief GPIO 位带 I2C：写、写后读：供 OLED / QMI8658 等共用（由 mw_runtime.c
 * include 参与编译）。
 */
#include "i2c_bus_bb.h"
#include "../../board/board_pins.h"
#include "asm/iic_soft.h"
#include "os/os_cpu.h"
#include "system/includes.h"

#define I2C_BUS_INDEX 0

// 定义软件I2C配置
const struct soft_iic_config soft_iic_cfg[] = {
    // iic0 data
    {
        .scl = BOARD_I2C_SCL, // IIC CLK脚
        .sda = BOARD_I2C_SDA, // IIC DAT脚
        .delay = 1,          // 软件IIC延时参数，影响通讯时钟频率
        .io_pu = 1, // 是否打开上拉电阻，如果外部电路没有焊接上拉电阻需要置1
    }};

void board_i2c_bus0_init(void) {
  // 使用SDK的软件I2C初始化
  soft_iic_init(I2C_BUS_INDEX);
}

int i2c_bus_bb_write7(u8 addr7, const u8 *tx, unsigned tx_len) {
  if (tx_len == 0) {
    return -1;
  }

  // OS_ENTER_CRITICAL();
  // 使用SDK的软件I2C发送
  soft_iic_start(I2C_BUS_INDEX);
  soft_iic_tx_byte(I2C_BUS_INDEX, (u8)((addr7 << 1) | 0));
  soft_iic_write_buf(I2C_BUS_INDEX, tx, tx_len);
  soft_iic_stop(I2C_BUS_INDEX);
  // OS_EXIT_CRITICAL();
  return 0;
}

int i2c_bus_bb_write_read7(u8 addr7, const u8 *tx, unsigned tx_len, u8 *rx,
                           unsigned rx_len) {
  if (tx_len == 0 || rx == NULL || rx_len == 0) {
    return -1;
  }

  // OS_ENTER_CRITICAL();
  // 写入数据
  soft_iic_start(I2C_BUS_INDEX);
  soft_iic_tx_byte(I2C_BUS_INDEX, (u8)((addr7 << 1) | 0));
  soft_iic_write_buf(I2C_BUS_INDEX, tx, tx_len);

  // 重复起始信号并读取数据
  soft_iic_start(I2C_BUS_INDEX);
  soft_iic_tx_byte(I2C_BUS_INDEX, (u8)((addr7 << 1) | 1));
  soft_iic_read_buf(I2C_BUS_INDEX, rx, rx_len);
  soft_iic_stop(I2C_BUS_INDEX);
  // OS_EXIT_CRITICAL();
  return 0;
}

// 扫描iic总线所有设备，并打印设备地址
void i2c_bus_bb_scan(void) {
  int ret = 0;
  u8 count = 0;
  printf("i2c_bus_bb_scan\n");

  // 只扫描有效的I2C地址范围（0x00-0x7F）
  for (u8 addr7 = 0; addr7 < 128; addr7++) {
    // 使用临界区保护I2C操作
    OS_ENTER_CRITICAL();

    // 发送起始信号
    soft_iic_start(I2C_BUS_INDEX);
    // 发送设备地址和写命令
    ret = soft_iic_tx_byte(I2C_BUS_INDEX, (u8)((addr7 << 1) | 0));
    // 发送停止信号
    soft_iic_stop(I2C_BUS_INDEX);

    OS_EXIT_CRITICAL();

    // 检查设备是否应答（返回1表示设备应答，返回0表示设备未应答）
    if (ret == 1) {
      printf("i2c addr7: 0x%02x\n", addr7);
      count++;
    }
  }
  printf("i2c_bus_bb_scan count: %d\n", count);
}
