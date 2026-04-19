#include "power_en.h"
#include "../../board/board_pins.h"

/// @brief 初始化电源使能引脚
void power_en_init(void) {
  // 初始化电源使能引脚
  gpio_set_direction(BOARD_POWER_EN, 0);
  gpio_set_pull_up(BOARD_POWER_EN, 0);
  gpio_set_pull_down(BOARD_POWER_EN, 1);
  gpio_set_output_value(BOARD_POWER_EN, 0);
}

/// @brief 使能电源
/// @param en 使能状态，0 为关闭，1 为开启
void power_en_enable(u8 en) {
  // 使能引脚输出 en 状态
  printf("power_en_enable: %d\n", en);
  gpio_set_output_value(BOARD_POWER_EN, en);
}