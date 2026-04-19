/**
 * @file user_main.c
 * @brief SDK 约定的用户入口：只做转发，业务在 app/ 中实现，便于与示例工程对齐。
 */
#include "app/app_sensor.h"

void user_main(void)
{
    app_sensor_boot();
}
