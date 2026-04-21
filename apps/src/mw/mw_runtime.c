/**
 * @file mw_runtime.c
 * @brief 中间层实现：聚合 OLED / IMU 等驱动，应用层不直接依赖寄存器级 API。
 */
#include "mw_runtime.h"
#include "../drivers/i2c/i2c_bus.h"
#include "../drivers/oled/bmp.h"
#include "../drivers/oled/oled.h"
#include "../drivers/oled/oled_utils.h"
#include "../drivers/qmi8658/qmi8658a.h"
#include "../drivers/qmi8658/qmi8658_reg.h"
#include "os/os_api.h"
#include "timer.h"
#include "typedef.h"
#include <math.h>
#include <stdbool.h>

void init_oled() {
  // 初始化 OLED
  OLED_Init();
  OLED_ColorTurn(0);
  OLED_DisplayTurn(0);
  OLED_Contrast(0xFF);
}

void mw_task(void *p_arg) {
  power_en_enable(1);
  os_time_dly(10);

  // 初始化 I2C 总线
  board_i2c_bus0_init();
  i2c_bus_scan();
  init_oled();

  // 清屏并显示欢迎界面
  OLED_Clear();
  OLED_ShowString(16, 8, "QMI8658", 16, 1);
  OLED_ShowString(16, 24, "IMU Test", 16, 1);
  OLED_ShowString(16, 40, "2026/04/22", 16, 1);
  OLED_Refresh();
  os_time_dly(200);

  // 初始化传感器
  if (QMI8658_Init() < 0) {
    printf("QMI8658 init failed!\n");
    OLED_Clear();
    OLED_ShowString(8, 16, "IMU Init", 16, 1);
    OLED_ShowString(8, 32, "Failed!", 16, 1);
    OLED_Refresh();
    while (1) {
      os_time_dly(100);
    }
  }

  // 清屏准备显示数据
  OLED_Clear();
  
  // 显示设备型号（左上角）
  QMI8658_Type_t dev_type = QMI8658_GetDeviceType();
  if (dev_type == QMI8658_TYPE_C) {
    OLED_ShowString(0, 0, "QMI8658C", 8, 1);
  } else {
    OLED_ShowString(0, 0, "QMI8658A", 8, 1);
  }
  
  // 显示校准状态
  QMI8658_Calibration_t calib;
  QMI8658_GetCalibration(&calib);
  if (calib.calibrated) {
    OLED_ShowString(80, 0, "CAL", 8, 1);
  } else {
    OLED_ShowString(80, 0, "UNCAL", 8, 1);
  }
  
  // 显示标签
  OLED_ShowString(0, 16, "A:", 8, 1);
  OLED_ShowString(64, 16, "G:", 8, 1);
  OLED_ShowString(0, 40, "T:", 8, 1);
  OLED_Refresh();

  // 正常模式：读取并显示传感器数据
  QMI8658_Data_t data;
  float temperature;
  uint32_t tick_count = 0;
  while (1) {
    tick_count++;
    
    // 每60秒自动校准一次（如果未校准）
    if (tick_count % 60000 == 0 && !calib.calibrated) {
      OLED_Clear();
      OLED_ShowString(24, 16, "Calibrating", 16, 1);
      OLED_ShowString(24, 32, "Gyro...", 16, 1);
      OLED_Refresh();
      
      if (QMI8658_CalibrateGyro() == 0) {
        OLED_ShowString(24, 32, "Accel...", 16, 1);
        OLED_Refresh();
        QMI8658_CalibrateAccel();
        QMI8658_GetCalibration(&calib);
        
        // 重新显示界面
        OLED_Clear();
        if (dev_type == QMI8658_TYPE_C) {
          OLED_ShowString(0, 0, "QMI8658C", 8, 1);
        } else {
          OLED_ShowString(0, 0, "QMI8658A", 8, 1);
        }
        OLED_ShowString(80, 0, "CAL", 8, 1);
        OLED_ShowString(0, 16, "A:", 8, 1);
        OLED_ShowString(64, 16, "G:", 8, 1);
        OLED_ShowString(0, 40, "T:", 8, 1);
        OLED_Refresh();
      }
    }
    
    if (QMI8658_ReadData(&data) < 0) {
      printf("QMI8658 read data failed\n");
      os_time_dly(10);
      continue;
    }
    
    QMI8658_ReadTemperature(&temperature);
    int16_t raw_temp = (int16_t)((temperature - 25.0f) * 256.0f);

    char acc_x_str[8], acc_y_str[8], acc_z_str[8];
    char gyr_x_str[8], gyr_y_str[8], gyr_z_str[8];
    char temp_str[8];
    
    scaled_int_to_str(data.acc_x, acc_x_str, 7, 8192);
    scaled_int_to_str(data.acc_y, acc_y_str, 7, 8192);
    scaled_int_to_str(data.acc_z, acc_z_str, 7, 8192);
    scaled_int_to_str(data.gyr_x, gyr_x_str, 7, 16);
    scaled_int_to_str(data.gyr_y, gyr_y_str, 7, 16);
    scaled_int_to_str(data.gyr_z, gyr_z_str, 7, 16);
    scaled_int_to_str(raw_temp, temp_str, 7, 256);
    
    OLED_ShowString(16, 16, (u8*)acc_x_str, 8, 1);
    OLED_ShowString(16, 24, (u8*)acc_y_str, 8, 1);
    OLED_ShowString(16, 32, (u8*)acc_z_str, 8, 1);
    
    OLED_ShowString(80, 16, (u8*)gyr_x_str, 8, 1);
    OLED_ShowString(80, 24, (u8*)gyr_y_str, 8, 1);
    OLED_ShowString(80, 32, (u8 *)gyr_z_str, 8, 1);

    OLED_ShowString(16, 40, (u8*)temp_str, 8, 1);
    
    OLED_Refresh();
    os_time_dly(1);
  }
}

void mw_runtime_init(void) {
  os_task_create(mw_task, NULL, 5, 1024, 0, "mw_task");
}
