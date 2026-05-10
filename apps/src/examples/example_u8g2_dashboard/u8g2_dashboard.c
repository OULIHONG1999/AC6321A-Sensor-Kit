/**
 * @file u8g2_dashboard.c
 * @brief u8g2图形库仪表盘演示 - 完整独立实现
 * 
 * 功能：
 * - 圆形仪表盘绘制（0-100%）
 * - 数据卡片显示（CPU、温度）
 * - 进度条动画（内存使用）
 * - 反色标题栏
 */
#include "../../board/example_config.h"

#if ENABLE_EXAMPLE_U8G2_DASHBOARD

#include "u8g2_dashboard.h"
#include "../../drivers/power_en/power_en.h"
#include "../../drivers/i2c/i2c_bus.h"
#include "../../lib/u8g2/csrc/u8g2.h"
#include "../../lib/u8g2/port/u8g2_port.h"
#include "os/os_api.h"
#include "system/event.h"
#include "typedef.h"
#include <math.h>
// #include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ========== 1. 按键事件处理 ==========

/**
 * @brief 按键事件处理函数（由 app_spp_and_le.c 调用）
 * @param key_value 按键值
 * @param event_type 事件类型
 * @note 此示例不需要按键功能，实现为空
 */
void example_key_handler(u8 key_value, u8 event_type) {
    // 此示例不需要按键功能
    (void)key_value;
    (void)event_type;
}


// ========== 2. 布局常量 ==========

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


// ========== 3. 自定义绘图工具 ==========

// u8g2 对象
static u8g2_t u8g2;

// 粗线绘制
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


// ========== 4. 示例主任务 ==========

/**
 * @brief u8g2仪表盘示例主任务
 * @param p_arg 任务参数（通常为NULL）
 */
static void u8g2_dashboard_task(void *p_arg) {
    printf("U8g2 dashboard test started\n");

    // ---- 4.1 硬件初始化 ----
    
    // 电源使能
    power_en_enable(1);
    os_time_dly(10);
    
    // I2C总线初始化
    board_i2c_bus0_init();

    // u8g2初始化 - 使用 SSD1306 128x64 I2C (全缓冲区模式)
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


    // ---- 4.2 主循环 ----
    
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


// ========== 5. 启动函数 ==========

/**
 * @brief 启动u8g2仪表盘示例
 * @note 此函数由 mw_runtime_init() 调用
 */
void u8g2_dashboard_start(void) {
    // 创建OS任务
    // 参数：任务函数, 参数, 优先级, 栈大小, CPU, 任务名
    os_task_create(u8g2_dashboard_task, NULL, 5, 1024, 0, "u8g2_dash");
}

#endif /* ENABLE_EXAMPLE_U8G2_DASHBOARD */
