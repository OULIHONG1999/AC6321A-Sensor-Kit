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

  QMI8658A_Init();

  // 显示欢迎界面
  OLED_ShowString(8, 16, "QMI8658 IMU Test", 16, 1);
  OLED_ShowString(20, 32, "2026/04/20", 16, 1);
  OLED_ShowString(0, 48, "Sensor Ready!", 16, 1);
  OLED_Refresh();
  // 正常模式：读取并显示传感器数据
  QMI8658A_Data_t data;
  while (1) {
    QMI8658A_ReadData(&data);

    printf("Acc: ,ax=%d, ay=%d, az=%d\nGyr: ,gx=%d, gy=%d, gz=%d\n", data.acc_x,
           data.acc_y, data.acc_z, data.gyr_x, data.gyr_y, data.gyr_z);

    OLED_ShowString(0, 0, "Acc:", 16, 1);
    OLED_ShowNum(32, 0, data.acc_x, 16, 1, 1);
    OLED_ShowNum(48, 0, data.acc_y, 16, 1, 1);
    OLED_ShowNum(64, 0, data.acc_z, 16, 1, 1);
    OLED_ShowString(0, 16, "Gyr:", 16, 1);
    OLED_ShowNum(32, 16, data.gyr_x, 16, 1, 1);
    OLED_ShowNum(48, 16, data.gyr_y, 16, 1, 1);
    OLED_ShowNum(64, 16, data.gyr_z, 16, 1, 1);
    OLED_Refresh();
    os_time_dly(1);
  }
}

void mw_runtime_init(void) {
  os_task_create(mw_task, NULL, 5, 1024, 0, "mw_task");
}
