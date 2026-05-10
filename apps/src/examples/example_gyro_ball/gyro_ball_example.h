#ifndef __GYRO_BALL_EXAMPLE_H__
#define __GYRO_BALL_EXAMPLE_H__

#include "u8g2_api.h"
#include "typedef.h"

/**
 * @file gyro_ball_example.h
 * @brief 陀螺仪控制小球示例头文件
 * 
 * 功能：
 * - 使用陀螺仪X/Y轴角速度控制小球移动
 * - 支持按键调整灵敏度、阻尼、半径等参数
 * - 显示运动轨迹效果
 */

// ============================================================================
// 配置宏
// ============================================================================

#define ENABLE_EXAMPLE_GYRO_BALL    1

// ============================================================================
// 参数范围定义
// ============================================================================

#define SENSITIVITY_MIN         1.0f
#define SENSITIVITY_MAX         50.0f
#define SENSITIVITY_DEFAULT     10.0f
#define SENSITIVITY_STEP        1.0f

#define DAMPING_MIN             0.1f
#define DAMPING_MAX             2.0f
#define DAMPING_DEFAULT         0.5f
#define DAMPING_STEP            0.1f

#define BALL_RADIUS_MIN         2
#define BALL_RADIUS_MAX         8
#define BALL_RADIUS_DEFAULT     4
#define BALL_RADIUS_STEP        1

// ============================================================================
// 屏幕边界定义
// ============================================================================

#define PLAY_AREA_X_MIN         4
#define PLAY_AREA_X_MAX         124
#define PLAY_AREA_Y_MIN         20
#define PLAY_AREA_Y_MAX         52

// 屏幕中心点
#define SCREEN_CENTER_X         64
#define SCREEN_CENTER_Y         36

// ============================================================================
// 轨迹配置
// ============================================================================

#define MAX_TRAIL_POINTS        50
#define TRAIL_UPDATE_INTERVAL   100  // 轨迹更新间隔 (ms)

// ============================================================================
// 按键事件类型
// ============================================================================

#define KEY_EVENT_CLICK         1   // 单击
#define KEY_EVENT_DOUBLE_CLICK  2   // 双击
#define KEY_EVENT_LONG          3   // 长按

// ============================================================================
// 数据结构定义
// ============================================================================

/**
 * @brief 小球参数结构体
 */
typedef struct {
    float sensitivity;     ///< 灵敏度 (1.0 ~ 50.0)
    float damping;         ///< 阻尼系数 (0.1 ~ 2.0)
    uint8_t radius;        ///< 小球半径 (2 ~ 8 像素)
    uint8_t show_trail;    ///< 显示轨迹 (0=关闭, 1=开启)
} ball_params_t;

/**
 * @brief 小球状态结构体
 */
typedef struct {
    float x;               ///< 当前位置 X
    float y;               ///< 当前位置 Y
    float vx;              ///< 当前速度 X
    float vy;              ///< 当前速度 Y
} ball_state_t;

/**
 * @brief 轨迹点结构体
 */
typedef struct {
    float x;               ///< 位置 X
    float y;               ///< 位置 Y
    uint8_t alpha;         ///< 透明度 (0-255)
} trail_point_t;

// ============================================================================
// 全局变量声明
// ============================================================================

extern ball_params_t g_ball_params;
extern ball_state_t g_ball_state;
extern trail_point_t g_trail[MAX_TRAIL_POINTS];
extern uint8_t g_trail_count;

// ============================================================================
// 函数声明
// ============================================================================

/**
 * @brief 启动陀螺仪小球示例
 * 
 * 功能：
 * - 初始化参数和状态
 * - 启动显示循环
 * - 读取陀螺仪数据并更新小球位置
 */
void gyro_ball_example_start(void);

/**
 * @brief 按键事件处理函数（必须是全局函数）
 * 
 * @param key_value 按键值 (KEY_1, KEY_2, KEY_3, KEY_4)
 * @param event_type 事件类型 (KEY_EVENT_CLICK, KEY_EVENT_DOUBLE_CLICK, KEY_EVENT_LONG)
 * 
 * 按键功能：
 * - KEY1: 单击+灵敏度, 双击-灵敏度, 长按重置灵敏度
 * - KEY2: 单击+阻尼, 双击-阻尼, 长按重置阻尼
 * - KEY3: 单击+半径, 双击-半径, 长按重置半径
 * - KEY4: 单击切换轨迹, 双击清除轨迹, 长按重置小球
 */
void gyro_ball_key_handler(u8 key_value, u8 event_type);

#endif /* __GYRO_BALL_EXAMPLE_H__ */
