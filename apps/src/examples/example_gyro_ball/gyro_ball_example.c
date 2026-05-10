/**
 * @file gyro_ball_example.c
 * @brief 陀螺仪控制小球示例实现
 * 
 * 功能：
 * - 使用QMI8658陀螺仪X/Y轴角速度控制小球移动
 * - 实时显示物理参数和运动轨迹
 * - 支持按键调整灵敏度、阻尼、半径
 * 
 * 使用方式：
 * 1. 在 example_config.h 中定义 ENABLE_EXAMPLE_GYRO_BALL = 1
 * 2. 调用 gyro_ball_example_start() 启动示例
 * 3. 在按键事件中调用 gyro_ball_key_handler()
 * 
 * @note 禁止包含 <stdio.h>，杰理SDK使用自定义printf实现
 */

#include "../../board/example_config.h"

#if ENABLE_EXAMPLE_GYRO_BALL

#include "../../lib/u8g2/port/u8g2_port.h"
static u8g2_t u8g2;

#include "gyro_ball_example.h"
#include "../../drivers/power_en/power_en.h"
#include "../../drivers/i2c/i2c_bus.h"
#include "../../drivers/qmi8658/qmi8658a.h"
#include "os/os_api.h"
#include "typedef.h"
#include "system/event.h"

// ============================================================================
// 宏定义
// ============================================================================

// 屏幕尺寸
#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       64
#define SCREEN_CENTER_X     (SCREEN_WIDTH / 2)
#define SCREEN_CENTER_Y     (SCREEN_HEIGHT / 2)

// 游戏区域边界
#define PLAY_AREA_X_MIN     0
#define PLAY_AREA_X_MAX     (SCREEN_WIDTH - 1)
#define PLAY_AREA_Y_MIN     17   // 标题栏下方
#define PLAY_AREA_Y_MAX     55   // 参数栏上方

// 小球参数范围
#define SENSITIVITY_MIN     1.0f
#define SENSITIVITY_MAX     50.0f
#define SENSITIVITY_DEFAULT 10.0f
#define SENSITIVITY_STEP    1.0f

#define DAMPING_MIN         0.1f
#define DAMPING_MAX         2.0f
#define DAMPING_DEFAULT     0.5f
#define DAMPING_STEP        0.1f

#define BALL_RADIUS_MIN     2
#define BALL_RADIUS_MAX     8
#define BALL_RADIUS_DEFAULT 4
#define BALL_RADIUS_STEP    1

// 轨迹配置
#define MAX_TRAIL_POINTS    50
#define TRAIL_UPDATE_INTERVAL 100  // ms

// 按键值定义（对应KEY_1~KEY_4）
#define KEY_1               0
#define KEY_2               1
#define KEY_3               2
#define KEY_4               3

// ============================================================================
// 数据结构定义
// ============================================================================

/**
 * @brief 小球参数结构
 */
typedef struct {
    float sensitivity;      // 灵敏度
    float damping;          // 阻尼系数
    uint8_t radius;         // 小球半径
    uint8_t show_trail;     // 是否显示轨迹
} ball_params_t;

/**
 * @brief 小球状态结构
 */
typedef struct {
    float x;                // X坐标
    float y;                // Y坐标
    float vx;               // X速度
    float vy;               // Y速度
} ball_state_t;

/**
 * @brief 轨迹点结构
 */
typedef struct {
    float x;                // X坐标
    float y;                // Y坐标
    uint8_t alpha;          // 透明度 (0-255)
} trail_point_t;

// ============================================================================
// 全局变量定义
// ============================================================================

ball_params_t g_ball_params = {
    .sensitivity = SENSITIVITY_DEFAULT,
    .damping = DAMPING_DEFAULT,
    .radius = BALL_RADIUS_DEFAULT,
    .show_trail = 1
};

ball_state_t g_ball_state = {
    .x = SCREEN_CENTER_X,
    .y = SCREEN_CENTER_Y,
    .vx = 0,
    .vy = 0
};

trail_point_t g_trail[MAX_TRAIL_POINTS];
uint8_t g_trail_count = 0;

// ============================================================================
// 内部变量
// ============================================================================

static uint32_t last_trail_update_time = 0;

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 浮点数转字符串（保留1位小数）
 * 
 * @param value 浮点数值
 * @param str 输出字符串缓冲区（至少5字节）
 */
static void float_to_str_1dec(float value, char *str)
{
    int int_part = (int)value;
    int dec_part = (int)((value - int_part) * 10);
    if (dec_part < 0) dec_part = -dec_part;
    
    // 手动格式化，避免使用 sprintf
    str[0] = (int_part / 10) + '0';
    str[1] = (int_part % 10) + '0';
    str[2] = '.';
    str[3] = dec_part + '0';
    str[4] = '\0';
}

/**
 * @brief 边界碰撞处理
 * 
 * @param pos 位置指针
 * @param vel 速度指针
 * @param min_pos 最小边界
 * @param max_pos 最大边界
 * @param radius 小球半径
 */
static void handle_boundary_collision(float *pos, float *vel, 
                                       float min_pos, float max_pos, 
                                       uint8_t radius)
{
    float min_limit = min_pos + radius;
    float max_limit = max_pos - radius;
    
    // 左边界碰撞
    if (*pos < min_limit) {
        *pos = min_limit;
        *vel = -*vel * 0.5f;  // 反弹并衰减
    }
    // 右边界碰撞
    else if (*pos > max_limit) {
        *pos = max_limit;
        *vel = -*vel * 0.5f;
    }
}

/**
 * @brief 更新轨迹点
 */
static void update_trail(void)
{
    if (!g_ball_params.show_trail) {
        g_trail_count = 0;
        return;
    }
    
    // 所有轨迹点透明度递减
    for (int i = 0; i < g_trail_count; i++) {
        if (g_trail[i].alpha > 20) {
            g_trail[i].alpha -= 20;
        } else {
            g_trail[i].alpha = 0;
        }
    }
    
    // 移除完全透明的点
    int write_idx = 0;
    for (int i = 0; i < g_trail_count; i++) {
        if (g_trail[i].alpha > 0) {
            if (write_idx != i) {
                g_trail[write_idx] = g_trail[i];
            }
            write_idx++;
        }
    }
    g_trail_count = write_idx;
    
    // 添加新轨迹点
    if (g_trail_count < MAX_TRAIL_POINTS) {
        g_trail[g_trail_count].x = g_ball_state.x;
        g_trail[g_trail_count].y = g_ball_state.y;
        g_trail[g_trail_count].alpha = 200;
        g_trail_count++;
    }
}

// ============================================================================
// 渲染函数
// ============================================================================

/**
 * @brief 绘制标题栏
 */
static void draw_title_bar(u8g2_t *u8g2)
{
    // 标题文字
    u8g2_SetFont(u8g2, u8g2_font_ncenB10_tr);
    u8g2_DrawStr(u8g2, 2, 12, "GYRO BALL");
    
    // 轨迹状态指示器
    if (g_ball_params.show_trail) {
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawDisc(u8g2, 110, 9, 3, U8G2_DRAW_ALL);  // 实心圆表示开启
    } else {
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawCircle(u8g2, 110, 9, 3, U8G2_DRAW_ALL);  // 空心圆表示关闭
    }
    u8g2_SetDrawColor(u8g2, 1);  // 恢复颜色
    
    // 分隔线
    u8g2_DrawHLine(u8g2, 0, 16, 128);
}

/**
 * @brief 绘制游戏区域边界
 */
static void draw_play_area(u8g2_t *u8g2)
{
    // 绘制虚线边框
    u8g2_SetDrawColor(u8g2, 1);
    
    // 上边界
    for (int x = PLAY_AREA_X_MIN; x <= PLAY_AREA_X_MAX; x += 4) {
        u8g2_DrawPixel(u8g2, x, PLAY_AREA_Y_MIN);
    }
    // 下边界
    for (int x = PLAY_AREA_X_MIN; x <= PLAY_AREA_X_MAX; x += 4) {
        u8g2_DrawPixel(u8g2, x, PLAY_AREA_Y_MAX);
    }
    // 左边界
    for (int y = PLAY_AREA_Y_MIN; y <= PLAY_AREA_Y_MAX; y += 4) {
        u8g2_DrawPixel(u8g2, PLAY_AREA_X_MIN, y);
    }
    // 右边界
    for (int y = PLAY_AREA_Y_MIN; y <= PLAY_AREA_Y_MAX; y += 4) {
        u8g2_DrawPixel(u8g2, PLAY_AREA_X_MAX, y);
    }
    
    u8g2_SetDrawColor(u8g2, 1);  // 恢复颜色
}

/**
 * @brief 绘制轨迹
 */
static void draw_trail(u8g2_t *u8g2)
{
    if (!g_ball_params.show_trail || g_trail_count == 0) {
        return;
    }
    
    // 从旧到新绘制轨迹点
    for (int i = 0; i < g_trail_count; i++) {
        uint8_t alpha = g_trail[i].alpha;
        if (alpha > 128) {
            u8g2_SetDrawColor(u8g2, 1);
            u8g2_DrawPixel(u8g2, (int)g_trail[i].x, (int)g_trail[i].y);
        } else if (alpha > 64) {
            // 较淡的点，可以跳过以减少渲染负担
        }
    }
    u8g2_SetDrawColor(u8g2, 1);
}

/**
 * @brief 绘制小球
 */
static void draw_ball(u8g2_t *u8g2)
{
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawDisc(u8g2, (int)g_ball_state.x, (int)g_ball_state.y, 
                  g_ball_params.radius, U8G2_DRAW_ALL);
    u8g2_SetDrawColor(u8g2, 1);
}

/**
 * @brief 绘制参数栏
 */
static void draw_params_bar(u8g2_t *u8g2)
{
    char str[8];
    
    // 分隔线
    u8g2_DrawHLine(u8g2, 0, 56, 128);
    
    // 设置小字体
    u8g2_SetFont(u8g2, u8g2_font_5x7_tr);
    u8g2_SetDrawColor(u8g2, 1);
    
    // 灵敏度
    float_to_str_1dec(g_ball_params.sensitivity, str);
    u8g2_DrawStr(u8g2, 2, 62, "S:");
    u8g2_DrawStr(u8g2, 12, 62, str);
    
    // 阻尼
    float_to_str_1dec(g_ball_params.damping, str);
    u8g2_DrawStr(u8g2, 32, 62, "D:");
    u8g2_DrawStr(u8g2, 42, 62, str);
    
    // 半径
    str[0] = (g_ball_params.radius / 10) + '0';
    str[1] = (g_ball_params.radius % 10) + '0';
    str[2] = '\0';
    u8g2_DrawStr(u8g2, 64, 62, "R:");
    u8g2_DrawStr(u8g2, 74, 62, str);
    
    // 轨迹状态
    u8g2_DrawStr(u8g2, 90, 62, "T:[");
    if (g_ball_params.show_trail) {
        u8g2_DrawStr(u8g2, 102, 62, "ON]");
    } else {
        u8g2_DrawStr(u8g2, 102, 62, "OFF");
    }
    
    u8g2_SetDrawColor(u8g2, 1);
}

// ============================================================================
// 按键事件处理（必须实现）
// ============================================================================

/**
 * @brief 按键事件处理函数（由 app_spp_and_le.c 调用）
 * @param key_value 按键值（0=KEY1, 1=KEY2, 2=KEY3, 3=KEY4）
 * @param event_type 事件类型（KEY_EVENT_CLICK短按, KEY_EVENT_DOUBLE_CLICK双击, KEY_EVENT_LONG长按）
 * 
 * @note 此函数必须是全局函数（不能是static），因为会被外部调用
 */
void example_key_handler(u8 key_value, u8 event_type)
{
    switch (key_value) {
    case KEY_1:
        if (event_type == KEY_EVENT_CLICK) {
            g_ball_params.sensitivity += SENSITIVITY_STEP;
            if (g_ball_params.sensitivity > SENSITIVITY_MAX)
                g_ball_params.sensitivity = SENSITIVITY_MAX;
        } else if (event_type == KEY_EVENT_DOUBLE_CLICK) {
            g_ball_params.sensitivity -= SENSITIVITY_STEP;
            if (g_ball_params.sensitivity < SENSITIVITY_MIN)
                g_ball_params.sensitivity = SENSITIVITY_MIN;
        } else if (event_type == KEY_EVENT_LONG) {
            g_ball_params.sensitivity = SENSITIVITY_DEFAULT;
        }
        break;
        
    case KEY_2:
        if (event_type == KEY_EVENT_CLICK) {
            g_ball_params.damping += DAMPING_STEP;
            if (g_ball_params.damping > DAMPING_MAX)
                g_ball_params.damping = DAMPING_MAX;
        } else if (event_type == KEY_EVENT_DOUBLE_CLICK) {
            g_ball_params.damping -= DAMPING_STEP;
            if (g_ball_params.damping < DAMPING_MIN)
                g_ball_params.damping = DAMPING_MIN;
        } else if (event_type == KEY_EVENT_LONG) {
            g_ball_params.damping = DAMPING_DEFAULT;
        }
        break;
        
    case KEY_3:
        if (event_type == KEY_EVENT_CLICK) {
            g_ball_params.radius += BALL_RADIUS_STEP;
            if (g_ball_params.radius > BALL_RADIUS_MAX)
                g_ball_params.radius = BALL_RADIUS_MAX;
        } else if (event_type == KEY_EVENT_DOUBLE_CLICK) {
            if (g_ball_params.radius > BALL_RADIUS_MIN)
                g_ball_params.radius -= BALL_RADIUS_STEP;
            else
                g_ball_params.radius = BALL_RADIUS_MIN;
        } else if (event_type == KEY_EVENT_LONG) {
            g_ball_params.radius = BALL_RADIUS_DEFAULT;
        }
        break;
        
    case KEY_4:
        if (event_type == KEY_EVENT_CLICK) {
            g_ball_params.show_trail = !g_ball_params.show_trail;
            if (!g_ball_params.show_trail) {
                g_trail_count = 0;
            }
        } else if (event_type == KEY_EVENT_DOUBLE_CLICK) {
            g_trail_count = 0;
        } else if (event_type == KEY_EVENT_LONG) {
            g_ball_state.x = SCREEN_CENTER_X;
            g_ball_state.y = SCREEN_CENTER_Y;
            g_ball_state.vx = 0;
            g_ball_state.vy = 0;
            g_trail_count = 0;
        }
        break;
        
    default:
        break;
    }
}

// ============================================================================
// 主循环
// ============================================================================

/**
 * @brief 示例主任务
 * @param p_arg 任务参数（通常为NULL）
 * 
 * @note 此任务包含完整的初始化流程和主循环
 */
static void gyro_ball_task(void *p_arg)
{
    // ---- 硬件初始化 ----
    
    // 电源使能
    power_en_enable(1);
    os_time_dly(10);  // 等待电源稳定
    
    // I2C总线初始化
    board_i2c_bus0_init();
    
    // u8g2显示屏初始化
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, 
        u8g2_byte_cb, u8g2_gpio_and_delay_cb);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    
    // 传感器初始化
    if (QMI8658_Init() < 0) {
        printf("QMI8658 init failed\n");
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
        u8g2_DrawStr(&u8g2, 10, 30, "Init Failed");
        u8g2_SendBuffer(&u8g2);
        while (1) os_time_dly(100);
    }
    
    // 陀螺仪校准
    QMI8658_CalibrateGyro();
    
    // 显示欢迎界面
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
    u8g2_DrawStr(&u8g2, 20, 30, "GYRO BALL");
    u8g2_SetFont(&u8g2, u8g2_font_7x13_tr);
    u8g2_DrawStr(&u8g2, 35, 50, "Demo");
    u8g2_SendBuffer(&u8g2);
    os_time_dly(200);  // 显示2秒
    
    // 初始化小球位置
    g_ball_state.x = SCREEN_CENTER_X;
    g_ball_state.y = SCREEN_CENTER_Y;
    g_ball_state.vx = 0;
    g_ball_state.vy = 0;
    g_trail_count = 0;
    
    printf("Gyro Ball Example Started\n");
    
    while (1) {
        current_time = os_time_get();
        
        // 读取陀螺仪数据
        if (QMI8658_ReadData(&sensor_data) == 0) {
            // 更新速度（使用角速度）
            g_ball_state.vx += sensor_data.gyr_x * g_ball_params.sensitivity;
            g_ball_state.vy += sensor_data.gyr_y * g_ball_params.sensitivity;
            
            // 应用阻尼
            g_ball_state.vx *= (1.0f - g_ball_params.damping);
            g_ball_state.vy *= (1.0f - g_ball_params.damping);
            
            // 更新位置
            g_ball_state.x += g_ball_state.vx;
            g_ball_state.y += g_ball_state.vy;
            
            // 边界碰撞检测
            handle_boundary_collision(&g_ball_state.x, &g_ball_state.vx,
                                      PLAY_AREA_X_MIN, PLAY_AREA_X_MAX,
                                      g_ball_params.radius);
            handle_boundary_collision(&g_ball_state.y, &g_ball_state.vy,
                                      PLAY_AREA_Y_MIN, PLAY_AREA_Y_MAX,
                                      g_ball_params.radius);
        }
        
        // 更新轨迹（每100ms）
        if (current_time - last_trail_update_time >= TRAIL_UPDATE_INTERVAL) {
            update_trail();
            last_trail_update_time = current_time;
        }
        
        // 渲染显示
        u8g2_ClearBuffer(u8g2);
        
        draw_title_bar(u8g2);
        draw_play_area(u8g2);
        draw_trail(u8g2);
        draw_ball(u8g2);
        draw_params_bar(u8g2);
        
        u8g2_SendBuffer(u8g2);
        
        // 控制刷新率 ~20 FPS
        os_time_dly(50);
    }
}

// ============================================================================
// 公共接口
// ============================================================================

void gyro_ball_example_start(void)
{
    // 创建OS任务
    // 参数：任务函数, 参数, 优先级, 栈大小, 队列大小, 任务名
    os_task_create(gyro_ball_task, NULL, 10, 1024, 0, "gyro_ball");
}

#endif /* ENABLE_EXAMPLE_GYRO_BALL */
