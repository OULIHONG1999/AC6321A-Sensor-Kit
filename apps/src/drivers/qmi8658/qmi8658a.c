#include "qmi8658a.h"
#include "../../board/board_pins.h"
#include "../i2c/i2c_bus.h"
#include "qmi8658_reg.h"
#include "os/os_api.h"

static QMI8658_Type_t g_device_type = QMI8658_TYPE_UNKNOWN;
static QMI8658_Calibration_t g_calibration = {0};
static uint8_t g_fifo_buffer[256];

static int qmi8658_read_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data) {
  int ret = i2c_bus_read_reg8(dev_addr, reg_addr);
  if (ret < 0) {
    return ret;
  }
  *data = (uint8_t)ret;
  return 0;
}

static int qmi8658_write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data) {
  uint8_t buf[2] = {reg_addr, data};
  return i2c_bus_write_buf(dev_addr, buf, 2);
}

static int qmi8658_read_regs(uint8_t dev_addr, uint8_t reg_addr, uint8_t *buf, uint8_t len) {
  return i2c_bus_read_buf(dev_addr, reg_addr, buf, len);
}

static int qmi8658_write_reg_bits(uint8_t reg, uint8_t mask, uint8_t value) {
  uint8_t current;
  int ret = qmi8658_read_reg(BOARD_IMU_I2C_ADDR7, reg, &current);
  if (ret < 0) {
    return ret;
  }
  uint8_t new_val = (current & ~mask) | (value & mask);
  return qmi8658_write_reg(BOARD_IMU_I2C_ADDR7, reg, new_val);
}

int  QMI8658_Init(void) {
  int ret;
  
  // 读取WHOAMI寄存器，确认设备连接
  uint8_t whoami;
  ret = qmi8658_read_reg(BOARD_IMU_I2C_ADDR7, QMI8658_REG_WHO_AM_I, &whoami);
  if (ret < 0) {
    printf("[QMI8658] ERROR: read WHOAMI failed: %d\n", ret);
    return ret;
  }
  printf("[QMI8658] WHOAMI: 0x%02X\n", whoami);

  // 读取REVISION_ID寄存器（硬件版本号）
  uint8_t revision_id;
  ret = qmi8658_read_reg(BOARD_IMU_I2C_ADDR7, QMI8658_REG_REVISION_ID, &revision_id);
  if (ret < 0) {
    printf("[QMI8658] ERROR: read REVISION_ID failed: %d\n", ret);
  } else {
    printf("[QMI8658] REVISION_ID: 0x%02X (Hardware version)\n", revision_id);
  }

  // 检测是否为QMI8658C（通过读取AttitudeEngine四元数寄存器）
  // C系列支持姿态引擎，四元数寄存器(0x49-0x4A)会有有效数据
  // A系列不支持，读取会返回0或无效值
  uint8_t ae_buf[4];
  ret = qmi8658_read_regs(BOARD_IMU_I2C_ADDR7, QMI8658_REG_DQW_L, ae_buf, 4);
  if (ret == 0 && (ae_buf[0] != 0 || ae_buf[1] != 0 || ae_buf[2] != 0 || ae_buf[3] != 0)) {
    g_device_type = QMI8658_TYPE_C;
    printf("[QMI8658] Device: QMI8658C (AttitudeEngine detected)\n");
  } else {
    g_device_type = QMI8658_TYPE_A;
    printf("[QMI8658] Device: QMI8658A\n");
  }

  // 使能地址自动递增
  ret = qmi8658_write_reg(BOARD_IMU_I2C_ADDR7, QMI8658_REG_CTRL1, QMI8658_CTRL1_ADDR_AI_EN);
  if (ret < 0) {
    printf("[QMI8658] ERROR: write CTRL1 failed: %d\n", ret);
    return ret;
  }

  // 加速度设置 ±4G, 512Hz
  ret = qmi8658_write_reg_bits(QMI8658_REG_CTRL2, QMI8658_CTRL2_ACC_RANGE_Msk, QMI8658_CTRL2_ACC_RANGE_4G);
  if (ret < 0) {
    printf("[QMI8658] ERROR: write CTRL2 ACC_RANGE failed: %d\n", ret);
    return ret;
  }
  ret = qmi8658_write_reg_bits(QMI8658_REG_CTRL2, QMI8658_CTRL2_ACC_ODR_Msk, QMI8658_CTRL2_ACC_ODR_512HZ);
  if (ret < 0) {
    printf("[QMI8658] ERROR: write CTRL2 ACC_ODR failed: %d\n", ret);
    return ret;
  }

  // 陀螺仪设置 ±2000dps, 512Hz
  ret = qmi8658_write_reg_bits(QMI8658_REG_CTRL3, QMI8658_CTRL3_GYR_RANGE_Msk, QMI8658_CTRL3_GYR_RANGE_2000DPS);
  if (ret < 0) {
    printf("[QMI8658] ERROR: write CTRL3 GYR_RANGE failed: %d\n", ret);
    return ret;
  }
  ret = qmi8658_write_reg_bits(QMI8658_REG_CTRL3, QMI8658_CTRL3_GYR_ODR_Msk, QMI8658_CTRL3_GYR_ODR_512HZ);
  if (ret < 0) {
    printf("[QMI8658] ERROR: write CTRL3 GYR_ODR failed: %d\n", ret);
    return ret;
  }

  // 滤波器配置（启动低通滤波）
  ret = qmi8658_write_reg(BOARD_IMU_I2C_ADDR7, QMI8658_REG_CTRL5, 0x00);
  if (ret < 0) {
    printf("[QMI8658] ERROR: write CTRL5 failed: %d\n", ret);
    return ret;
  }

  // 使能传感器（加速度计 + 陀螺仪 + 温度）
  ret = qmi8658_write_reg_bits(QMI8658_REG_CTRL7, QMI8658_CTRL7_ACC_EN_Msk, QMI8658_CTRL7_ACC_EN);
  if (ret < 0) {
    printf("[QMI8658] ERROR: write CTRL7 ACC_EN failed: %d\n", ret);
    return ret;
  }
  ret = qmi8658_write_reg_bits(QMI8658_REG_CTRL7, QMI8658_CTRL7_GYR_EN_Msk, QMI8658_CTRL7_GYR_EN);
  if (ret < 0) {
    printf("[QMI8658] ERROR: write CTRL7 GYR_EN failed: %d\n", ret);
    return ret;
  }
  ret = qmi8658_write_reg_bits(QMI8658_REG_CTRL7, QMI8658_CTRL7_TEMP_EN_Msk, QMI8658_CTRL7_TEMP_EN);
  if (ret < 0) {
    printf("[QMI8658] ERROR: write CTRL7 TEMP_EN failed: %d\n", ret);
    return ret;
  }

  QMI8658_DEBUG_PRINT("Init OK\n");
  return 0;
}

QMI8658_Type_t QMI8658_GetDeviceType(void) {
  return g_device_type;
}

int  QMI8658_ReadData(QMI8658_Data_t *data) {
  uint8_t buf[12];
  int ret;

  if (data == NULL) {
    return -1;
  }

  ret = qmi8658_read_regs(BOARD_IMU_I2C_ADDR7, QMI8658_REG_AX_L, buf, 12);
  if (ret < 0) {
    printf("[QMI8658] ERROR: read data failed: %d\n", ret);
    return ret;
  }

  data->acc_x = (int16_t)((buf[1] << 8) | buf[0]);
  data->acc_y = (int16_t)((buf[3] << 8) | buf[2]);
  data->acc_z = (int16_t)((buf[5] << 8) | buf[4]);
  data->gyr_x = (int16_t)((buf[7] << 8) | buf[6]);
  data->gyr_y = (int16_t)((buf[9] << 8) | buf[8]);
  data->gyr_z = (int16_t)((buf[11] << 8) | buf[10]);

  if (g_calibration.calibrated) {
    data->acc_x -= g_calibration.acc_offset_x;
    data->acc_y -= g_calibration.acc_offset_y;
    data->acc_z -= g_calibration.acc_offset_z;
    data->gyr_x -= g_calibration.gyr_offset_x;
    data->gyr_y -= g_calibration.gyr_offset_y;
    data->gyr_z -= g_calibration.gyr_offset_z;
  }

  QMI8658_DEBUG_PRINT("Data: A(%d,%d,%d) G(%d,%d,%d)\n",
    data->acc_x, data->acc_y, data->acc_z,
    data->gyr_x, data->gyr_y, data->gyr_z);

  return 0;
}

int  QMI8658_ReadTemperature(float *temperature) {
  uint8_t buf[2];
  int ret;
  int16_t raw_temp;

  if (temperature == NULL) {
    return -1;
  }

  ret = qmi8658_read_regs(BOARD_IMU_I2C_ADDR7, QMI8658_REG_TEMP_L, buf, 2);
  if (ret < 0) {
    printf("[QMI8658] ERROR: read temperature failed: %d\n", ret);
    return ret;
  }

  raw_temp = (int16_t)((buf[1] << 8) | buf[0]);
  *temperature = 25.0f + (raw_temp / 256.0f);

  QMI8658_DEBUG_PRINT("Temp: %.2f\n", *temperature);

  return 0;
}

float QMI8658_ConvertAccToG(int16_t raw) {
  return (float)raw / QMI8658_ACC_SCALE_4G;
}

float QMI8658_ConvertGyroToDPS(int16_t raw) {
  return (float)raw / QMI8658_GYR_SCALE_2000DPS;
}

float QMI8658_ConvertTempToC(int16_t raw) {
  return 25.0f + (float)raw / QMI8658_TEMP_SCALE;
}

int QMI8658_ReadPhysical(QMI8658_Physical_t *physical) {
  QMI8658_Data_t raw_data;
  uint8_t temp_buf[2];
  int16_t raw_temp;
  int ret;

  if (physical == NULL) {
    return -1;
  }

  ret = QMI8658_ReadData(&raw_data);
  if (ret < 0) {
    return ret;
  }

  physical->acc_x_g = QMI8658_ConvertAccToG(raw_data.acc_x);
  physical->acc_y_g = QMI8658_ConvertAccToG(raw_data.acc_y);
  physical->acc_z_g = QMI8658_ConvertAccToG(raw_data.acc_z);
  physical->gyr_x_dps = QMI8658_ConvertGyroToDPS(raw_data.gyr_x);
  physical->gyr_y_dps = QMI8658_ConvertGyroToDPS(raw_data.gyr_y);
  physical->gyr_z_dps = QMI8658_ConvertGyroToDPS(raw_data.gyr_z);

  ret = qmi8658_read_regs(BOARD_IMU_I2C_ADDR7, QMI8658_REG_TEMP_L, temp_buf, 2);
  if (ret < 0) {
    printf("[QMI8658] ERROR: read temperature failed: %d\n", ret);
    physical->temp_c = 0.0f;
  } else {
    raw_temp = (int16_t)((temp_buf[1] << 8) | temp_buf[0]);
    physical->temp_c = QMI8658_ConvertTempToC(raw_temp);
  }

  QMI8658_DEBUG_PRINT("Physical: A(%d,%d,%d)g G(%d,%d,%d)dps T(%d)C\n",
    (int)(physical->acc_x_g * 100), (int)(physical->acc_y_g * 100), (int)(physical->acc_z_g * 100),
    (int)(physical->gyr_x_dps * 100), (int)(physical->gyr_y_dps * 100), (int)(physical->gyr_z_dps * 100),
    (int)(physical->temp_c * 100));

  return 0;
}

int  QMI8658_IsDataReady(void) {
  uint8_t status;
  int ret;

  ret = qmi8658_read_reg(BOARD_IMU_I2C_ADDR7, QMI8658_REG_STATUSINT, &status);
  if (ret < 0) {
    return ret;
  }

  return (status & (QMI8658_STATUSINT_DRDY_ACC_Msk | QMI8658_STATUSINT_DRDY_GYR_Msk)) != 0;
}

int  QMI8658_WaitForDataReady(int timeout_ms) {
  while (timeout_ms > 0) {
    if (QMI8658_IsDataReady() > 0) {
      return 0;
    }
    os_time_dly(1);
    timeout_ms -= 10;
  }
  return -2;
}

void QMI8658_SoftReset(void) {
  qmi8658_write_reg(BOARD_IMU_I2C_ADDR7, QMI8658_REG_RESET, QMI8658_SOFT_RESET_VAL);
  os_time_dly(10);
}

int QMI8658_CalibrateGyro(void) {
  int ret;
  int32_t gyr_x_sum = 0, gyr_y_sum = 0, gyr_z_sum = 0;
  QMI8658_Data_t data;
  const int samples = 100;

  QMI8658_DEBUG_PRINT("Gyro calibration started...\n");
  
  for (int i = 0; i < samples; i++) {
    ret = QMI8658_ReadData(&data);
    if (ret < 0) {
      QMI8658_DEBUG_PRINT("Gyro calibration failed: read data error\n");
      return ret;
    }
    gyr_x_sum += data.gyr_x;
    gyr_y_sum += data.gyr_y;
    gyr_z_sum += data.gyr_z;
    os_time_dly(10);
  }
  
  g_calibration.gyr_offset_x = (int16_t)(gyr_x_sum / samples);
  g_calibration.gyr_offset_y = (int16_t)(gyr_y_sum / samples);
  g_calibration.gyr_offset_z = (int16_t)(gyr_z_sum / samples);
  g_calibration.calibrated = 1;
  
  QMI8658_DEBUG_PRINT("Gyro calibration complete: (%d,%d,%d)\n",
    g_calibration.gyr_offset_x,
    g_calibration.gyr_offset_y,
    g_calibration.gyr_offset_z);
  
  return 0;
}

int QMI8658_CalibrateAccel(void) {
  int ret;
  int32_t acc_x_sum = 0, acc_y_sum = 0, acc_z_sum = 0;
  QMI8658_Data_t data;
  const int samples = 100;
  
  QMI8658_DEBUG_PRINT("Accel calibration started...\n");
  
  for (int i = 0; i < samples; i++) {
    ret = QMI8658_ReadData(&data);
    if (ret < 0) {
      QMI8658_DEBUG_PRINT("Accel calibration failed: read data error\n");
      return ret;
    }
    acc_x_sum += data.acc_x;
    acc_y_sum += data.acc_y;
    acc_z_sum += data.acc_z;
    os_time_dly(10);
  }
  
  g_calibration.acc_offset_x = (int16_t)(acc_x_sum / samples);
  g_calibration.acc_offset_y = (int16_t)(acc_y_sum / samples);
  g_calibration.acc_offset_z = (int16_t)((acc_z_sum / samples) - QMI8658_ACC_SCALE_4G);
  g_calibration.calibrated = 1;
  
  QMI8658_DEBUG_PRINT("Accel calibration complete: (%d,%d,%d)\n",
    g_calibration.acc_offset_x,
    g_calibration.acc_offset_y,
    g_calibration.acc_offset_z);
  
  return 0;
}

int QMI8658_ApplyCalibration(void) {
  int ret;
  uint8_t cal_data[8];
  
  if (!g_calibration.calibrated) {
    QMI8658_DEBUG_PRINT("No calibration data available\n");
    return -1;
  }
  
  cal_data[0] = (uint8_t)(g_calibration.gyr_offset_x & 0xFF);
  cal_data[1] = (uint8_t)((g_calibration.gyr_offset_x >> 8) & 0xFF);
  cal_data[2] = (uint8_t)(g_calibration.gyr_offset_y & 0xFF);
  cal_data[3] = (uint8_t)((g_calibration.gyr_offset_y >> 8) & 0xFF);
  cal_data[4] = (uint8_t)(g_calibration.gyr_offset_z & 0xFF);
  cal_data[5] = (uint8_t)((g_calibration.gyr_offset_z >> 8) & 0xFF);
  cal_data[6] = (uint8_t)(g_calibration.acc_offset_x & 0xFF);
  cal_data[7] = (uint8_t)((g_calibration.acc_offset_x >> 8) & 0xFF);
  
  ret = i2c_bus_write_buf(BOARD_IMU_I2C_ADDR7, (uint8_t[]){QMI8658_REG_CAL1_L}, 2);
  if (ret < 0) {
    QMI8658_DEBUG_PRINT("Write gyro offset command failed\n");
    return ret;
  }
  
  ret = qmi8658_write_reg(BOARD_IMU_I2C_ADDR7, QMI8658_REG_CTRL9, QMI8658_CMD_GYR_OFFSET);
  if (ret < 0) {
    QMI8658_DEBUG_PRINT("Write gyro offset command failed\n");
    return ret;
  }
  
  QMI8658_DEBUG_PRINT("Calibration applied\n");
  return 0;
}

int QMI8658_GetCalibration(QMI8658_Calibration_t *calib) {
  if (calib == NULL) {
    return -1;
  }
  *calib = g_calibration;
  return 0;
}

int QMI8658_SetCalibration(QMI8658_Calibration_t *calib) {
  if (calib == NULL) {
    return -1;
  }
  g_calibration = *calib;
  return 0;
}

int QMI8658_FifoInit(void) {
  int ret;
  
  ret = qmi8658_write_reg(BOARD_IMU_I2C_ADDR7, QMI8658_REG_FIFO_WTM_TH, 8);
  if (ret < 0) {
    QMI8658_DEBUG_PRINT("FIFO init failed: write WTM\n");
    return ret;
  }
  
  uint8_t fifo_config = QMI8658_FIFO_CTRL_EN | QMI8658_FIFO_CTRL_ACC | QMI8658_FIFO_CTRL_GYR;
  ret = qmi8658_write_reg(BOARD_IMU_I2C_ADDR7, QMI8658_REG_FIFO_CTRL, fifo_config);
  if (ret < 0) {
    QMI8658_DEBUG_PRINT("FIFO init failed: write CTRL\n");
    return ret;
  }
  
  QMI8658_DEBUG_PRINT("FIFO initialized\n");
  return 0;
}

int QMI8658_FifoRead(QMI8658_Data_t *data, uint8_t max_count, uint8_t *actual_count) {
  uint8_t status;
  uint8_t fifo_count;
  int ret;
  
  if (data == NULL || actual_count == NULL) {
    return -1;
  }
  
  ret = qmi8658_read_reg(BOARD_IMU_I2C_ADDR7, QMI8658_REG_FIFO_STATUS, &status);
  if (ret < 0) {
    QMI8658_DEBUG_PRINT("FIFO read failed: read status\n");
    return ret;
  }
  
  fifo_count = (status >> 4) & 0x0F;
  if (fifo_count == 0) {
    *actual_count = 0;
    return 0;
  }
  
  if (fifo_count > max_count) {
    fifo_count = max_count;
  }
  
  uint8_t frame_size = 12;
  ret = qmi8658_read_regs(BOARD_IMU_I2C_ADDR7, QMI8658_REG_FIFO_DATA, g_fifo_buffer, frame_size * fifo_count);
  if (ret < 0) {
    QMI8658_DEBUG_PRINT("FIFO read failed: read data\n");
    return ret;
  }
  
  for (uint8_t i = 0; i < fifo_count; i++) {
    uint8_t offset = i * frame_size;
    data[i].acc_x = (int16_t)((g_fifo_buffer[offset + 1] << 8) | g_fifo_buffer[offset + 0]);
    data[i].acc_y = (int16_t)((g_fifo_buffer[offset + 3] << 8) | g_fifo_buffer[offset + 2]);
    data[i].acc_z = (int16_t)((g_fifo_buffer[offset + 5] << 8) | g_fifo_buffer[offset + 4]);
    data[i].gyr_x = (int16_t)((g_fifo_buffer[offset + 7] << 8) | g_fifo_buffer[offset + 6]);
    data[i].gyr_y = (int16_t)((g_fifo_buffer[offset + 9] << 8) | g_fifo_buffer[offset + 8]);
    data[i].gyr_z = (int16_t)((g_fifo_buffer[offset + 11] << 8) | g_fifo_buffer[offset + 10]);
    
    if (g_calibration.calibrated) {
      data[i].acc_x -= g_calibration.acc_offset_x;
      data[i].acc_y -= g_calibration.acc_offset_y;
      data[i].acc_z -= g_calibration.acc_offset_z;
      data[i].gyr_x -= g_calibration.gyr_offset_x;
      data[i].gyr_y -= g_calibration.gyr_offset_y;
      data[i].gyr_z -= g_calibration.gyr_offset_z;
    }
  }
  
  *actual_count = fifo_count;
  
  QMI8658_DEBUG_PRINT("FIFO read: %d frames\n", fifo_count);
  return 0;
}

int QMI8658_FifoGetStatus(uint8_t *status) {
  if (status == NULL) {
    return -1;
  }
  return qmi8658_read_reg(BOARD_IMU_I2C_ADDR7, QMI8658_REG_FIFO_STATUS, status);
}

int QMI8658_FifoGetCount(uint8_t *count) {
  uint8_t status;
  int ret;
  
  if (count == NULL) {
    return -1;
  }
  
  ret = qmi8658_read_reg(BOARD_IMU_I2C_ADDR7, QMI8658_REG_FIFO_STATUS, &status);
  if (ret < 0) {
    return ret;
  }
  
  *count = (status >> 4) & 0x0F;
  return 0;
}

int QMI8658_FifoReset(void) {
  int ret;
  ret = qmi8658_write_reg(BOARD_IMU_I2C_ADDR7, QMI8658_REG_CTRL9, QMI8658_CMD_RST_FIFO);
  if (ret < 0) {
    QMI8658_DEBUG_PRINT("FIFO reset failed\n");
    return ret;
  }
  QMI8658_DEBUG_PRINT("FIFO reset\n");
  return 0;
}
