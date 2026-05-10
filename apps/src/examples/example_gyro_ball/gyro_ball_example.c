/**
 * @file gyro_ball_example.c
 * @brief 陀螺仪控制小球示例实现
 * 
 * 功能：
 * - 使用QMI8658陀螺仪X/Y轴角速度控制小球移动
 * - 平滑的物理运动效果
 * - 简洁的UI布局，无元素重叠
 * - 支持按键调整灵敏度、阻尼、半径
 * 
 * 使用方式：
 * 1. 在 example_config.h 中定义 ENABLE_EXAMPLE_GYRO_BALL = 1
 * 2. 调用 gyro_ball_example_start() 启动示例
 * 3. 在按键事件中调用 example_key_handler()
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

// UI布局区域
#define TITLE_HEIGHT        14      // 标题栏高度
#define PARAMS_HEIGHT       10      // 参数栏高度
#define PLAY_AREA_TOP       TITLE_HEIGHT    // 游戏区顶部
#define PLAY_AREA_BOTTOM    (SCREEN_HEIGHT - PARAMS_HEIGHT)  // 游戏区底部
#define PLAY_AREA_HEIGHT    (PLAY_AREA_BOTTOM - PLAY_AREA_TOP)

// 小球参数范围
#define SENSITIVITY_MIN     20.0f
#define SENSITIVITY_MAX     200.0f    // 大幅提高最大值
#define SENSITIVITY_DEFAULT 80.0f     // 大幅提高默认值
#define SENSITIVITY_STEP    10.0f     // 增大步进

#define DAMPING_MIN         0.01f
#define DAMPING_MAX         0.3f
#define DAMPING_DEFAULT     0.05f
#define DAMPING_STEP        0.01f

#define BALL_RADIUS_MIN     2
#define BALL_RADIUS_MAX     6
#define BALL_RADIUS_DEFAULT 3
#define BALL_RADIUS_STEP    1

// 物理仿真参数
#define MAX_VELOCITY        5.0f    // 降低最大速度
#define DT                  0.02f   // 固定时间步长 (用于物理积分，非实际时间)

// 按键值定义（对应KEY_1~KEY_4）
// 注意：SDK的key_value从1开始，不是0
#define KEY_1               1
#define KEY_2               2
#define KEY_3               3
#define KEY_4               4

// ============================================================================
// 数据结构定义
// ============================================================================

/**
 * @brief 小球参数结构
 */
typedef struct {
    float sensitivity;      // 灵敏度
    float move_speed;       // 移动速度（平滑系数）
    uint8_t radius;         // 小球半径
} ball_params_t;

/**
 * @brief 小球状态结构
 */
typedef struct {
    float x;                // X坐标（浮点数，用于平滑运动）
    float y;                // Y坐标
    float vx;               // X速度
    float vy;               // Y速度
} ball_state_t;

// ============================================================================
// 全局变量定义
// ============================================================================

ball_params_t g_ball_params = {
    .sensitivity = SENSITIVITY_DEFAULT,
    .move_speed = 0.10f,      // 降低摩擦力（原来是0.15）
    .radius = BALL_RADIUS_DEFAULT
};

ball_state_t g_ball_state = {
    .x = SCREEN_WIDTH / 2,
    .y = (PLAY_AREA_TOP + PLAY_AREA_BOTTOM) / 2,
    .vx = 0,
    .vy = 0
};

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 限制数值范围
 */
static float clamp_float(float value, float min_val, float max_val)
{
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

/**
 * @brief 边界碰撞处理（弹性碰撞）
 */
static void handle_boundary_collision(ball_state_t *ball, uint8_t radius)
{
    float min_x = radius;
    float max_x = SCREEN_WIDTH - 1 - radius;
    float min_y = PLAY_AREA_TOP + radius;
    float max_y = PLAY_AREA_BOTTOM - 1 - radius;
    
    // X轴边界
    if (ball->x < min_x) {
        ball->x = min_x;
        ball->vx = -ball->vx * 0.7f;  // 弹性反弹
    } else if (ball->x > max_x) {
        ball->x = max_x;
        ball->vx = -ball->vx * 0.7f;
    }
    
    // Y轴边界
    if (ball->y < min_y) {
        ball->y = min_y;
        ball->vy = -ball->vy * 0.7f;
    } else if (ball->y > max_y) {
        ball->y = max_y;
        ball->vy = -ball->vy * 0.7f;
    }
}

/**
 * @brief 浮点数转字符串（保留1位小数）
 */
static void float_to_str_1dec(float value, char *str)
{
    int int_part = (int)value;
    int dec_part = (int)((value - int_part) * 10);
    if (dec_part < 0) dec_part = -dec_part;
    
    str[0] = (int_part / 10) + '0';
    str[1] = (int_part % 10) + '0';
    str[2] = '.';
    str[3] = dec_part + '0';
    str[4] = '\0';
}

// ============================================================================
// 渲染函数
// ============================================================================

/**
 * @brief 绘制标题栏
 */
static void draw_title_bar(u8g2_t *u8g2_ptr, QMI8658_Data_t *data)
{
    char str[16];
    
    // 标题文字
    u8g2_SetFont(u8g2_ptr, u8g2_font_5x7_tr);
    u8g2_DrawStr(u8g2_ptr, 2, 8, "GYRO BALL");
    
    // 显示传感器数据（调试用）
    // sensor_data.acc_x/y/z 是RAW值，需要转换为g
    // QMI8658配置：±4g量程，8192 LSB/g
    float acc_x_g = (float)data->acc_x / 8192.0f;  // 转换为g
    float acc_y_g = (float)data->acc_y / 8192.0f;
    float acc_z_g = (float)data->acc_z / 8192.0f;
    
    // 显示g值（更直观）
    int acc_x_display = (int)(acc_x_g * 10);  // 保留1位小数
    int acc_y_display = (int)(acc_y_g * 10);
    int acc_z_display = (int)(acc_z_g * 10);
    
    // 格式化X值: "X:-2.5" 或 "X:3.8"
    char x_str[8];
    int idx = 0;
    x_str[idx++] = 'X';
    x_str[idx++] = ':';
    
    if (acc_x_display < 0) {
        acc_x_display = -acc_x_display;
        x_str[idx++] = '-';
    }
    
    // 整数部分（可能多位数）
    int int_part = acc_x_display / 10;
    int dec_part = acc_x_display % 10;
    
    // 确保小数部分在0-9范围内
    if (dec_part < 0) dec_part = -dec_part;
    if (dec_part > 9) dec_part = 9;
    
    if (int_part >= 10) {
        x_str[idx++] = (int_part / 10) + '0';
        x_str[idx++] = (int_part % 10) + '0';
    } else {
        x_str[idx++] = int_part + '0';
    }
    
    x_str[idx++] = '.';
    x_str[idx++] = dec_part + '0';
    x_str[idx] = '\0';
    
    u8g2_DrawStr(u8g2_ptr, 55, 8, x_str);
    
    // 格式化Y值
    char y_str[8];
    idx = 0;
    y_str[idx++] = 'Y';
    y_str[idx++] = ':';
    
    if (acc_y_display < 0) {
        acc_y_display = -acc_y_display;
        y_str[idx++] = '-';
    }
    
    int_part = acc_y_display / 10;
    dec_part = acc_y_display % 10;
    
    // 确保小数部分在0-9范围内
    if (dec_part < 0) dec_part = -dec_part;
    if (dec_part > 9) dec_part = 9;
    
    if (int_part >= 10) {
        y_str[idx++] = (int_part / 10) + '0';
        y_str[idx++] = (int_part % 10) + '0';
    } else {
        y_str[idx++] = int_part + '0';
    }
    
    y_str[idx++] = '.';
    y_str[idx++] = dec_part + '0';
    y_str[idx] = '\0';
    
    u8g2_DrawStr(u8g2_ptr, 90, 8, y_str);
    
    // 分隔线
    u8g2_DrawHLine(u8g2_ptr, 0, TITLE_HEIGHT - 1, SCREEN_WIDTH);
}

/**
 * @brief 绘制小球
 */
static void draw_ball(u8g2_t *u8g2_ptr)
{
    u8g2_DrawDisc(u8g2_ptr, (int)g_ball_state.x, (int)g_ball_state.y, 
                  g_ball_params.radius, U8G2_DRAW_ALL);
}

/**
 * @brief 绘制参数栏
 */
static void draw_params_bar(u8g2_t *u8g2_ptr)
{
    char str[8];
    
    // 分隔线
    u8g2_DrawHLine(u8g2_ptr, 0, SCREEN_HEIGHT - PARAMS_HEIGHT, SCREEN_WIDTH);
    
    // 设置小字体
    u8g2_SetFont(u8g2_ptr, u8g2_font_5x7_tr);
    
    // 灵敏度
    float_to_str_1dec(g_ball_params.sensitivity, str);
    u8g2_DrawStr(u8g2_ptr, 2, SCREEN_HEIGHT - 2, "S:");
    u8g2_DrawStr(u8g2_ptr, 14, SCREEN_HEIGHT - 2, str);
    
    // 摩擦力
    int friction_val = (int)(g_ball_params.move_speed * 100);
    str[0] = (friction_val / 10) + '0';
    str[1] = '.';
    str[2] = (friction_val % 10) + '0';
    str[3] = '\0';
    u8g2_DrawStr(u8g2_ptr, 40, SCREEN_HEIGHT - 2, "F:");
    u8g2_DrawStr(u8g2_ptr, 52, SCREEN_HEIGHT - 2, str);
    
    // 半径
    str[0] = (g_ball_params.radius / 10) + '0';
    str[1] = (g_ball_params.radius % 10) + '0';
    str[2] = '\0';
    u8g2_DrawStr(u8g2_ptr, 78, SCREEN_HEIGHT - 2, "R:");
    u8g2_DrawStr(u8g2_ptr, 88, SCREEN_HEIGHT - 2, str);
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
    // 打印事件类型名称
    const char* event_names[] = {"CLICK", "LONG", "HOLD", "UP", "DOUBLE"};
    const char* event_name = (event_type < 5) ? event_names[event_type] : "UNKNOWN";
    printf("[GYRO_BALL] Key=%d, Event=%s(%d)\n", key_value, event_name, event_type);
    
    switch (key_value) {
    case KEY_1:  // 灵敏度
        if (event_type == KEY_EVENT_CLICK) {
            g_ball_params.sensitivity += SENSITIVITY_STEP;
            g_ball_params.sensitivity = clamp_float(g_ball_params.sensitivity, 
                                                     SENSITIVITY_MIN, SENSITIVITY_MAX);
            printf("[GYRO_BALL] Sens: %d\n", (int)(g_ball_params.sensitivity * 10));
        } else if (event_type == KEY_EVENT_DOUBLE_CLICK) {
            g_ball_params.sensitivity -= SENSITIVITY_STEP;
            g_ball_params.sensitivity = clamp_float(g_ball_params.sensitivity, 
                                                     SENSITIVITY_MIN, SENSITIVITY_MAX);
            printf("[GYRO_BALL] Sens: %d\n", (int)(g_ball_params.sensitivity * 10));
        } else if (event_type == KEY_EVENT_LONG) {
            g_ball_params.sensitivity = SENSITIVITY_DEFAULT;
            printf("[GYRO_BALL] Sens reset\n");
        }
        break;
        
    case KEY_2:  // 摩擦力
        if (event_type == KEY_EVENT_CLICK) {
            g_ball_params.move_speed += 0.05f;
            if (g_ball_params.move_speed > 0.5f)
                g_ball_params.move_speed = 0.5f;
            printf("[GYRO_BALL] Friction: %d\n", (int)(g_ball_params.move_speed * 100));
        } else if (event_type == KEY_EVENT_DOUBLE_CLICK) {
            g_ball_params.move_speed -= 0.05f;
            if (g_ball_params.move_speed < 0.05f)
                g_ball_params.move_speed = 0.05f;
            printf("[GYRO_BALL] Friction: %d\n", (int)(g_ball_params.move_speed * 100));
        } else if (event_type == KEY_EVENT_LONG) {
            g_ball_params.move_speed = 0.15f;
            printf("[GYRO_BALL] Friction reset\n");
        }
        break;
        
    case KEY_3:  // 半径
        if (event_type == KEY_EVENT_CLICK) {
            g_ball_params.radius += BALL_RADIUS_STEP;
            if (g_ball_params.radius > BALL_RADIUS_MAX)
                g_ball_params.radius = BALL_RADIUS_MAX;
            printf("[GYRO_BALL] Radius: %d\n", g_ball_params.radius);
        } else if (event_type == KEY_EVENT_DOUBLE_CLICK) {
            if (g_ball_params.radius > BALL_RADIUS_MIN)
                g_ball_params.radius -= BALL_RADIUS_STEP;
            printf("[GYRO_BALL] Radius: %d\n", g_ball_params.radius);
        } else if (event_type == KEY_EVENT_LONG) {
            g_ball_params.radius = BALL_RADIUS_DEFAULT;
            printf("[GYRO_BALL] Radius reset\n");
        }
        break;
        
    case KEY_4:  // 重置
        if (event_type == KEY_EVENT_LONG) {
            // 长按开始时重置
            g_ball_state.x = SCREEN_WIDTH / 2;
            g_ball_state.y = (PLAY_AREA_TOP + PLAY_AREA_BOTTOM) / 2;
            g_ball_state.vx = 0;
            g_ball_state.vy = 0;
            printf("[GYRO_BALL] Reset\n");
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
static void gyro_ball_example_task(void *p_arg)
{
    QMI8658_Data_t sensor_data;
    
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
    
    // 清屏避免花屏
    u8g2_ClearBuffer(&u8g2);
    u8g2_SendBuffer(&u8g2);
    
    // 传感器初始化
    if (QMI8658_Init() < 0) {
        printf("QMI8658 init failed\n");
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_7x13_tr);
        u8g2_DrawStr(&u8g2, 10, 30, "Init Failed");
        u8g2_SendBuffer(&u8g2);
        while (1) os_time_dly(10);
    }
    
    // 陀螺仪校准
    QMI8658_CalibrateGyro();
    
    // 显示欢迎界面
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_7x13_tr);
    u8g2_DrawStr(&u8g2, 20, 25, "GYRO BALL");
    u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);
    u8g2_DrawStr(&u8g2, 30, 45, "Demo v1.0");
    u8g2_SendBuffer(&u8g2);
    os_time_dly(150);  // 显示1.5秒
    
    // 初始化小球位置
    g_ball_state.x = SCREEN_WIDTH / 2;
    g_ball_state.y = (PLAY_AREA_TOP + PLAY_AREA_BOTTOM) / 2;
    g_ball_state.vx = 0;
    g_ball_state.vy = 0;
    
    printf("Gyro Ball Example Started\n");
    
    // 调试计数
    uint32_t debug_count = 0;
    
    // ---- 主循环 ----
    while (1) {
        
        // 读取传感器数据
        if (QMI8658_ReadData(&sensor_data) == 0) {
            // 平衡球物理模型：倾斜产生重力分量，推动小球滚动
            // QMI8658配置：±4g量程，8192 LSB/g
            
            // 转换为g值
            float acc_x_g = (float)sensor_data.acc_x / 8192.0f;
            float acc_y_g = (float)sensor_data.acc_y / 8192.0f;
            
            // 低通滤波：平滑数据
            static float filtered_acc_x = 0;
            static float filtered_acc_y = 0;
            float alpha = 0.5f;  // 滤波系数
            
            filtered_acc_x = alpha * acc_x_g + (1 - alpha) * filtered_acc_x;
            filtered_acc_y = alpha * acc_y_g + (1 - alpha) * filtered_acc_y;
            
            // 死区过滤：微小倾斜不产生力（阈值0.05g）
            if (filtered_acc_x > -0.05f && filtered_acc_x < 0.05f) filtered_acc_x = 0;
            if (filtered_acc_y > -0.05f && filtered_acc_y < 0.05f) filtered_acc_y = 0;
            
            // 计算加速度（g值 * 灵敏度）
            // 灵敏度单位：像素/(s^2·g)，默认0.3
            // 倾斜10°时：acc_x ≈ 0.17g，加速度 = 0.17 × 0.3 = 0.05 像素/s^2
            float ax = filtered_acc_x * g_ball_params.sensitivity;
            float ay = filtered_acc_y * g_ball_params.sensitivity;
            
            // 更新速度（v = v + a * dt）
            g_ball_state.vx += ax * DT;
            g_ball_state.vy += ay * DT;
            
            // 应用摩擦力/阻尼（模拟滚动阻力）
            float friction = 1.0f - g_ball_params.move_speed;  // move_speed现在是摩擦系数
            g_ball_state.vx *= friction;
            g_ball_state.vy *= friction;
            
            // 限制最大速度
            g_ball_state.vx = clamp_float(g_ball_state.vx, -MAX_VELOCITY, MAX_VELOCITY);
            g_ball_state.vy = clamp_float(g_ball_state.vy, -MAX_VELOCITY, MAX_VELOCITY);
            
            // 更新位置（p = p + v * dt）
            g_ball_state.x += g_ball_state.vx;
            g_ball_state.y += g_ball_state.vy;
            
            // 边界碰撞检测（弹性反弹）
            handle_boundary_collision(&g_ball_state, g_ball_params.radius);
        }
        
        // 渲染显示（每帧完整重绘，避免残影）
        u8g2_ClearBuffer(&u8g2);
        
        draw_title_bar(&u8g2, &sensor_data);
        draw_ball(&u8g2);
        draw_params_bar(&u8g2);
        
        u8g2_SendBuffer(&u8g2);
        
        // 每50帧输出一次调试信息
        debug_count++;
        if (debug_count % 50 == 0) {
            int pos_x = (int)g_ball_state.x;
            int pos_y = (int)g_ball_state.y;
            printf("[GYRO_BALL] Pos:(%d,%d)\n", pos_x, pos_y);
        }
        
        // 注意：os_time_dly() 精度有限（通常10ms粒度）
        // 这里使用较小延迟让系统调度，实际刷新率由传感器采样决定
        os_time_dly(1);
    }
}

// ============================================================================
// 公共接口
// ============================================================================

void gyro_ball_example_start(void)
{
    // 创建OS任务
    // 参数：任务函数, 参数, 优先级, 栈大小, CPU, 任务名
    os_task_create(gyro_ball_example_task, NULL, 10, 1024, 0, "gyro_ball");
}

#endif /* ENABLE_EXAMPLE_GYRO_BALL */
