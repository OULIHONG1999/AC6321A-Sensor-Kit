/**
 * @file mw_runtime.c
 * @brief 中间层实现：聚合 OLED / IMU 等驱动，应用层不直接依赖寄存器级 API。
 */
#include "mw_runtime.h"
#include "../drivers/i2c/i2c_bus.h"
#include "../drivers/oled/bmp.h"
#include "../drivers/oled/oled.h"
#include "../drivers/qmi8658/qmi8658a.h"
#include "os/os_api.h"
#include "timer.h"
#include "typedef.h"
#include <math.h>
#include <stdbool.h>

static void int_to_str(int num, char *str, int width) {
  int i = 0;
  int is_negative = 0;
  
  if (num < 0) {
    is_negative = 1;
    num = -num;
  }
  
  if (num == 0) {
    str[i++] = '0';
  }
  
  while (num > 0) {
    str[i++] = (num % 10) + '0';
    num /= 10;
  }
  
  if (is_negative) {
    str[i++] = '-';
  }
  
  while (i < width) {
    str[i++] = ' ';
  }
  
  str[i] = '\0';
  
  // 反转字符串
  int start = 0;
  int end = i - 1;
  while (start < end) {
    char temp = str[start];
    str[start] = str[end];
    str[end] = temp;
    start++;
    end--;
  }
}

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
  OLED_ShowString(8, 16, "QMI8658 IMU Test", 16, 1);
  OLED_ShowString(20, 32, "2026/04/20", 16, 1);
  OLED_ShowString(0, 48, "Sensor Ready!", 16, 1);
  OLED_Refresh();
  os_time_dly(200);

  // 初始化传感器
  if (QMI8658A_Init() < 0) {
    printf("QMI8658A init failed!\n");
    OLED_Clear();
    OLED_ShowString(0, 16, "IMU Init Failed!", 16, 1);
    OLED_Refresh();
    while (1) {
      os_time_dly(100);
    }
  }

  // 清屏准备显示数据
  OLED_Clear();
  OLED_ShowString(0, 0, "T:", 8, 1);
  OLED_ShowString(0, 16, "A:", 8, 1);
  OLED_ShowString(64, 16, "G:", 8, 1);
  OLED_Refresh();

  // 正常模式：读取并显示传感器数据
  QMI8658A_Data_t data;
  float temperature;
  while (1) {
    if (QMI8658A_ReadData(&data) < 0) {
      printf("QMI8658A read data failed\n");
      os_time_dly(10);
      continue;
    }

    QMI8658A_ReadTemperature(&temperature);

    printf("Acc: ax=%d, ay=%d, az=%d\nGyr: gx=%d, gy=%d, gz=%d\nTemp: %d\n",
           data.acc_x, data.acc_y, data.acc_z,
           data.gyr_x, data.gyr_y, data.gyr_z,
           (int)temperature);

    // 将整数转为固定长度字符串
    char acc_x_str[8], acc_y_str[8], acc_z_str[8];
    char gyr_x_str[8], gyr_y_str[8], gyr_z_str[8];
    char temp_str[8];
    
    int_to_str(data.acc_x, acc_x_str, 5);
    int_to_str(data.acc_y, acc_y_str, 5);
    int_to_str(data.acc_z, acc_z_str, 5);
    int_to_str(data.gyr_x, gyr_x_str, 5);
    int_to_str(data.gyr_y, gyr_y_str, 5);
    int_to_str(data.gyr_z, gyr_z_str, 5);
    int_to_str((int)temperature, temp_str, 3);
    
    // 显示温度数据 (最上面)
    OLED_ShowString(16, 0, (u8*)temp_str, 8, 1);
    
    // 显示加速度数据 (左边)
    OLED_ShowString(16, 16, (u8*)acc_x_str, 8, 1);
    OLED_ShowString(16, 24, (u8*)acc_y_str, 8, 1);
    OLED_ShowString(16, 32, (u8*)acc_z_str, 8, 1);
    
    // 显示陀螺仪数据 (右边)
    OLED_ShowString(80, 16, (u8*)gyr_x_str, 8, 1);
    OLED_ShowString(80, 24, (u8*)gyr_y_str, 8, 1);
    OLED_ShowString(80, 32, (u8*)gyr_z_str, 8, 1);
    
    // 刷新显示
    OLED_Refresh();
    os_time_dly(1);
  }
}

void mw_runtime_init(void) {
  os_task_create(mw_task, NULL, 5, 1024, 0, "mw_task");
}
