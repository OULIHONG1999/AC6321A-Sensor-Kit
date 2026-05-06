/**
 * @file mw_runtime.c
 * @brief 中间层实现：聚合 OLED / IMU 等驱动，应用层不直接依赖寄存器级 API。
 */
#include "mw_runtime.h"
#include "../drivers/power_en/power_en.h"
#include "../drivers/i2c/i2c_bus.h"
#include "../drivers/oled/bmp.h"
#include "../drivers/oled/oled.h"
#include "../drivers/oled/oled_utils.h"
#include "../drivers/qmi8658/qmi8658_reg.h"
#include "../drivers/qmi8658/qmi8658a.h"
#include "../lib/u8g2/csrc/u8g2.h"
#include "../lib/u8g2/port/u8g2_port.h"
#include "os/os_api.h"
#include "timer.h"
#include "typedef.h"
#include <math.h>
// #include <stdbool.h>
#include <string.h>

// #include <stdio.h>

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

    OLED_ShowString(16, 16, (u8 *)acc_x_str, 8, 1);
    OLED_ShowString(16, 24, (u8 *)acc_y_str, 8, 1);
    OLED_ShowString(16, 32, (u8 *)acc_z_str, 8, 1);

    OLED_ShowString(80, 16, (u8 *)gyr_x_str, 8, 1);
    OLED_ShowString(80, 24, (u8 *)gyr_y_str, 8, 1);
    OLED_ShowString(80, 32, (u8 *)gyr_z_str, 8, 1);

    OLED_ShowString(16, 40, (u8 *)temp_str, 8, 1);

    OLED_Refresh();
    os_time_dly(1);
  }
}

// 最简单的 u8x8 测试 - 只需要底层驱动
static u8x8_t u8x8;

// 声明 SSD1306 驱动回调函数（在 u8x8_d_ssd1306_128x64_noname.c 中定义）
extern uint8_t u8x8_d_ssd1306_128x64_noname(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

// u8g2 对象
static u8g2_t u8g2;

/* ========== 布局常量 (128 x 64) ========== */
#define TITLE_H     9       // 顶部反色标题栏高度
#define GAUGE_CX    28      // 仪表圆心 X
#define GAUGE_CY    26      // 仪表圆心 Y
#define GAUGE_R     13      // 仪表半径
#define CARD_X      54      // 右侧卡片起始 X
#define CARD_W      70      // 卡片宽度
#define CARD_H      20      // 卡片高度
#define CARD1_Y     10      // CPU 卡片 Y
#define CARD2_Y     32      // TEMP 卡片 Y
#define BAR_X       20      // 进度条起始 X (MEM 标签占 x=0~18)
#define BAR_Y       54      // 进度条 Y
#define BAR_W       106     // 进度条宽度
#define BAR_H       8       // 进度条高度

// ========== 自定义绘图工具 ==========

// 粗线
static void drawThickLine(u8g2_uint_t x1, u8g2_uint_t y1,
                          u8g2_uint_t x2, u8g2_uint_t y2, int t) {
    for (int i = -t / 2; i <= t / 2; i++)
        u8g2_DrawLine(&u8g2, x1, y1 + i, x2, y2 + i);
}

// 带标签的卡片（数值基线自动贴近卡片底部）
static void drawCard(u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t h,
                     const char *label, const char *value) {
    u8g2_DrawRFrame(&u8g2, x, y, w, h, 3);

    // 标签（小字）
    u8g2_SetFont(&u8g2, u8g2_font_5x7_tf);
    u8g2_DrawStr(&u8g2, x + 4, y + 8, label);

    // 数值（大字）—— 基线 = y + h - 1，随卡片高度自适应
    u8g2_SetFont(&u8g2, u8g2_font_8x13B_tf);
    u8g2_DrawStr(&u8g2, x + 4, y + h - 1, value);
}

// 圆形仪表（210° ~ 330° 弧形，映射 0~100%）
static void drawGauge(u8g2_uint_t cx, u8g2_uint_t cy,
                      u8g2_uint_t r, int percent) {
    // 外圆
    u8g2_DrawCircle(&u8g2, cx, cy, r, 2);

    // 刻度线（12 个刻度）
    for (int i = 0; i < 12; i++) {
        float angle = (210.0f + i * 240.0f / 11.0f) * (float)M_PI / 180.0f;
        int x1 = cx + (r - 2) * cosf(angle);
        int y1 = cy + (r - 2) * sinf(angle);
        int x2 = cx + (r - 5) * cosf(angle);
        int y2 = cy + (r - 5) * sinf(angle);
        u8g2_DrawLine(&u8g2, x1, y1, x2, y2);
    }

    // 指针
    float na = (210.0f + percent * 240.0f / 100.0f) * (float)M_PI / 180.0f;
    int nx = cx + (r - 7) * cosf(na);
    int ny = cy + (r - 7) * sinf(na);
    drawThickLine(cx, cy, nx, ny, 2);
    u8g2_DrawDisc(&u8g2, cx, cy, 2, 0);

    // 百分比文字（圆的正下方）
    char buf[5];
    sprintf(buf, "%d%%", percent);
    u8g2_SetFont(&u8g2, u8g2_font_5x7_tf);
    u8g2_uint_t tw = u8g2_GetStrWidth(&u8g2, buf);
    u8g2_DrawStr(&u8g2, cx - tw / 2, cy + r + 7, buf);
}

// 进度条
static void drawProgressBar(u8g2_uint_t x, u8g2_uint_t y,
                            u8g2_uint_t w, u8g2_uint_t h, int percent) {
    u8g2_DrawRFrame(&u8g2, x, y, w, h, h / 2);
    int fillW = (long)(w - 4) * percent / 100;
    if (fillW > 0)
        u8g2_DrawRBox(&u8g2, x + 2, y + 2, fillW, h - 4, (h - 4) / 2);
}

// ========== 仪表盘测试任务 ==========

void u8g2_test(void *p_arg) {
    printf("U8g2 dashboard test started\n");

    // 初始化 u8g2 - 使用 SSD1306 128x64 I2C (全缓冲区模式)
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        &u8g2,
        U8G2_R0,
        u8g2_byte_cb,
        u8g2_gpio_and_delay_cb);

    // 初始化显示
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);

    printf("U8g2 initialized\n");

    // 清屏
    u8g2_ClearBuffer(&u8g2);
    u8g2_SendBuffer(&u8g2);

    int percent = 0;
    while (1) {
        percent = (percent + 1) % 101;

        u8g2_ClearBuffer(&u8g2);
        u8g2_SetDrawColor(&u8g2, 1);

        // ---- 顶部标题栏（反色）----
        u8g2_DrawBox(&u8g2, 0, 0, 128, TITLE_H);
        u8g2_SetDrawColor(&u8g2, 0);
        u8g2_SetFont(&u8g2, u8g2_font_5x7_tf);
        u8g2_DrawStr(&u8g2, 2, 7, "DASHBOARD");
        u8g2_SetDrawColor(&u8g2, 1);

        // ---- 左侧：圆形仪表 ----
        drawGauge(GAUGE_CX, GAUGE_CY, GAUGE_R, percent);

        // ---- 右侧：数据卡片 ----
        char buf[8];

        sprintf(buf, "%d%%", percent);
        drawCard(CARD_X, CARD1_Y, CARD_W, CARD_H, "CPU", buf);

        sprintf(buf, "%d", 45 + percent / 3);
        drawCard(CARD_X, CARD2_Y, CARD_W, CARD_H, "TEMP", buf);

        // ---- 底部：内存进度条 ----
        u8g2_SetFont(&u8g2, u8g2_font_5x7_tf);
        u8g2_DrawStr(&u8g2, 2, BAR_Y + 7, "MEM");
        drawProgressBar(BAR_X, BAR_Y, BAR_W, BAR_H, percent);

        u8g2_SendBuffer(&u8g2);

        // 延迟 50ms
        os_time_dly(5);
    }
}

void mw_runtime_init(void) {
    os_task_create(u8g2_test, NULL, 5, 1024, 0, "u8g2_test");
}