/**
 * @file app_sensor.c
 * @brief 应用层：演示串口横幅 + 通过中间层完成显示子系统初始化。
 */
#include "app_sensor.h"
#include "../mw/mw_runtime.h"
#include "system/includes.h"

void app_sensor_boot(void) {
  printf(
      "--------------------------------------------------------------------\n");
  printf(
      "|  sensor board app — layered: app -> mw -> drivers -> hal/board |\n");
  printf(
      "--------------------------------------------------------------------\n");

  mw_runtime_init();
}
