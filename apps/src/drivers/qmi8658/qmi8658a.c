#include "qmi8658a.h"
#include "../../board/board_pins.h"
#include "../i2c/i2c_bus.h"
#include "qmi8658_reg.h"

int I2C_ReadByte(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data) {
  int ret = i2c_bus_read_reg8(dev_addr, reg_addr);
  if (ret < 0) {
    return ret;
  }
  *data = (uint8_t)ret;
  return 0;
}

int I2C_WriteByte(uint8_t dev_addr, uint8_t reg_addr, uint8_t data) {
  uint8_t buf[2] = {reg_addr, data};
  return i2c_bus_write_buf(dev_addr, buf, 2);
}

int I2C_ReadBuf(uint8_t dev_addr, uint8_t reg_addr, uint8_t *buf, uint8_t len) {
  return i2c_bus_read_buf(dev_addr, reg_addr, buf, len);
}

static int QMI8658_WriteRegBits(uint8_t reg, uint8_t mask, uint8_t value) {
  uint8_t current;
  int ret = I2C_ReadByte(BOARD_IMU_I2C_ADDR7, reg, &current);
  if (ret < 0) {
    return ret;
  }
  uint8_t new_val = (current & ~mask) | (value & mask);
  return I2C_WriteByte(BOARD_IMU_I2C_ADDR7, reg, new_val);
}

// 初始化QMI8658A
void QMI8658A_Init(void) {
  // 读取WHOAMI寄存器，确认设备连接
  uint8_t whoami;

  int ret = I2C_ReadByte(BOARD_IMU_I2C_ADDR7, QMI8658_REG_WHO_AM_I, &whoami);
  if (ret < 0) {
    printf("qmi8658 read WHOAMI failed: %d\n", ret);
    return;
  }
  printf("qmi8658 who am i: 0X%02X\n", whoami);

  // 读取id
  uint8_t id;
  I2C_ReadByte(BOARD_IMU_I2C_ADDR7, QMI8658_REG_REVISION_ID, &id);
  printf("qmi8658 id: 0X%02X\n", id);

  // 使能地址自动递增
  I2C_WriteByte(BOARD_IMU_I2C_ADDR7, QMI8658_REG_CTRL1, QMI8658_CTRL1_ADDR_AI_EN);

  // 加速度设置 ±4G, 512Hz
  QMI8658_WriteRegBits(QMI8658_REG_CTRL2, QMI8658_CTRL2_ACC_RANGE_Msk, QMI8658_CTRL2_ACC_RANGE_4G);
  QMI8658_WriteRegBits(QMI8658_REG_CTRL2, QMI8658_CTRL2_ACC_ODR_Msk, QMI8658_CTRL2_ACC_ODR_512HZ);

  // 陀螺仪设置 ±2000dps, 512Hz
  QMI8658_WriteRegBits(QMI8658_REG_CTRL3, QMI8658_CTRL3_GYR_RANGE_Msk, QMI8658_CTRL3_GYR_RANGE_2000DPS);
  QMI8658_WriteRegBits(QMI8658_REG_CTRL3, QMI8658_CTRL3_GYR_ODR_Msk, QMI8658_CTRL3_GYR_ODR_512HZ);


  // 滤波器配置（启动低通滤波）
  I2C_WriteByte(BOARD_IMU_I2C_ADDR7, QMI8658_REG_CTRL5, 0x00);

  // 使能传感器（加速度计 + 陀螺仪）
  QMI8658_WriteRegBits(QMI8658_REG_CTRL7, QMI8658_CTRL7_ACC_EN_Msk, QMI8658_CTRL7_ACC_EN);
  QMI8658_WriteRegBits(QMI8658_REG_CTRL7, QMI8658_CTRL7_GYR_EN_Msk, QMI8658_CTRL7_GYR_EN);
}

// 读取QMI8658A数据
void QMI8658A_ReadData(QMI8658A_Data_t *data) {
  uint8_t buf[12];

  // 读取加速度和陀螺仪数据
  I2C_ReadBuf(BOARD_IMU_I2C_ADDR7, QMI8658_REG_AX_L, buf, 12);
  put_buf(buf, 12);

  // 组合数据
  data->acc_x = (buf[1] << 8) | buf[0];
  data->acc_y = (buf[3] << 8) | buf[2];
  data->acc_z = (buf[5] << 8) | buf[4];
  data->gyr_x = (buf[7] << 8) | buf[6];
  data->gyr_y = (buf[9] << 8) | buf[8];
  data->gyr_z = (buf[11] << 8) | buf[10];
}
