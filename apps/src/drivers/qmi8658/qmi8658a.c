#include "qmi8658a.h"
#include "../../board/board_pins.h"
#include "../i2c/i2c_bus.h"
#include "qmi8658_reg.h"
#include "os/os_api.h"

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

int QMI8658A_Init(void) {
  int ret;
  
  // 读取WHOAMI寄存器，确认设备连接
  uint8_t whoami;
  ret = I2C_ReadByte(BOARD_IMU_I2C_ADDR7, QMI8658_REG_WHO_AM_I, &whoami);
  if (ret < 0) {
    printf("qmi8658 read WHOAMI failed: %d\n", ret);
    return ret;
  }
  printf("qmi8658 who am i: 0X%02X\n", whoami);

  // 读取id
  uint8_t id;
  ret = I2C_ReadByte(BOARD_IMU_I2C_ADDR7, QMI8658_REG_REVISION_ID, &id);
  if (ret < 0) {
    printf("qmi8658 read REVISION_ID failed: %d\n", ret);
  } else {
    printf("qmi8658 id: 0X%02X\n", id);
  }

  // 使能地址自动递增
  ret = I2C_WriteByte(BOARD_IMU_I2C_ADDR7, QMI8658_REG_CTRL1, QMI8658_CTRL1_ADDR_AI_EN);
  if (ret < 0) {
    printf("qmi8658 write CTRL1 failed: %d\n", ret);
    return ret;
  }

  // 加速度设置 ±4G, 512Hz
  ret = QMI8658_WriteRegBits(QMI8658_REG_CTRL2, QMI8658_CTRL2_ACC_RANGE_Msk, QMI8658_CTRL2_ACC_RANGE_4G);
  if (ret < 0) {
    printf("qmi8658 write CTRL2 ACC_RANGE failed: %d\n", ret);
    return ret;
  }
  ret = QMI8658_WriteRegBits(QMI8658_REG_CTRL2, QMI8658_CTRL2_ACC_ODR_Msk, QMI8658_CTRL2_ACC_ODR_512HZ);
  if (ret < 0) {
    printf("qmi8658 write CTRL2 ACC_ODR failed: %d\n", ret);
    return ret;
  }

  // 陀螺仪设置 ±2000dps, 512Hz
  ret = QMI8658_WriteRegBits(QMI8658_REG_CTRL3, QMI8658_CTRL3_GYR_RANGE_Msk, QMI8658_CTRL3_GYR_RANGE_2000DPS);
  if (ret < 0) {
    printf("qmi8658 write CTRL3 GYR_RANGE failed: %d\n", ret);
    return ret;
  }
  ret = QMI8658_WriteRegBits(QMI8658_REG_CTRL3, QMI8658_CTRL3_GYR_ODR_Msk, QMI8658_CTRL3_GYR_ODR_512HZ);
  if (ret < 0) {
    printf("qmi8658 write CTRL3 GYR_ODR failed: %d\n", ret);
    return ret;
  }

  // 滤波器配置（启动低通滤波）
  ret = I2C_WriteByte(BOARD_IMU_I2C_ADDR7, QMI8658_REG_CTRL5, 0x00);
  if (ret < 0) {
    printf("qmi8658 write CTRL5 failed: %d\n", ret);
    return ret;
  }

  // 使能传感器（加速度计 + 陀螺仪 + 温度）
  ret = QMI8658_WriteRegBits(QMI8658_REG_CTRL7, QMI8658_CTRL7_ACC_EN_Msk, QMI8658_CTRL7_ACC_EN);
  if (ret < 0) {
    printf("qmi8658 write CTRL7 ACC_EN failed: %d\n", ret);
    return ret;
  }
  ret = QMI8658_WriteRegBits(QMI8658_REG_CTRL7, QMI8658_CTRL7_GYR_EN_Msk, QMI8658_CTRL7_GYR_EN);
  if (ret < 0) {
    printf("qmi8658 write CTRL7 GYR_EN failed: %d\n", ret);
    return ret;
  }
  ret = QMI8658_WriteRegBits(QMI8658_REG_CTRL7, QMI8658_CTRL7_TEMP_EN_Msk, QMI8658_CTRL7_TEMP_EN);
  if (ret < 0) {
    printf("qmi8658 write CTRL7 TEMP_EN failed: %d\n", ret);
    return ret;
  }

  return 0;
}

int QMI8658A_ReadData(QMI8658A_Data_t *data) {
  uint8_t buf[12];
  int ret;

  if (data == NULL) {
    return -1;
  }

  ret = I2C_ReadBuf(BOARD_IMU_I2C_ADDR7, QMI8658_REG_AX_L, buf, 12);
  if (ret < 0) {
    printf("qmi8658 read data failed: %d\n", ret);
    return ret;
  }

  data->acc_x = (int16_t)((buf[1] << 8) | buf[0]);
  data->acc_y = (int16_t)((buf[3] << 8) | buf[2]);
  data->acc_z = (int16_t)((buf[5] << 8) | buf[4]);
  data->gyr_x = (int16_t)((buf[7] << 8) | buf[6]);
  data->gyr_y = (int16_t)((buf[9] << 8) | buf[8]);
  data->gyr_z = (int16_t)((buf[11] << 8) | buf[10]);

  return 0;
}

int QMI8658A_ReadTemperature(float *temperature) {
  uint8_t buf[2];
  int ret;
  int16_t raw_temp;

  if (temperature == NULL) {
    return -1;
  }

  ret = I2C_ReadBuf(BOARD_IMU_I2C_ADDR7, QMI8658_REG_TEMP_L, buf, 2);
  if (ret < 0) {
    printf("qmi8658 read temperature failed: %d\n", ret);
    return ret;
  }

  raw_temp = (int16_t)((buf[1] << 8) | buf[0]);
  *temperature = 25.0f + (raw_temp / 512.0f);

  return 0;
}

int QMI8658A_IsDataReady(void) {
  uint8_t status;
  int ret;

  ret = I2C_ReadByte(BOARD_IMU_I2C_ADDR7, QMI8658_REG_STATUSINT, &status);
  if (ret < 0) {
    return ret;
  }

  return (status & (QMI8658_STATUSINT_DRDY_ACC_Msk | QMI8658_STATUSINT_DRDY_GYR_Msk)) != 0;
}

int QMI8658A_WaitForDataReady(int timeout_ms) {
  while (timeout_ms > 0) {
    if (QMI8658A_IsDataReady() > 0) {
      return 0;
    }
    os_time_dly(1);
    timeout_ms -= 10;
  }
  return -2;
}

void QMI8658A_SoftReset(void) {
  I2C_WriteByte(BOARD_IMU_I2C_ADDR7, QMI8658_REG_RESET, QMI8658_SOFT_RESET_VAL);
  os_time_dly(10);
}
