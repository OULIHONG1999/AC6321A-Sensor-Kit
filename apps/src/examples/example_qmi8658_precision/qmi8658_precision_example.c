/**
 * @file qmi8658_precision_example.c
 * @brief QMI8658精确测量示例 - 模块化设计
 * 
 * 功能模块：
 * - 数据显示模块（RAW值/物理量）
 * - 水平仪功能（姿态可视化）
 * - 峰值测试工具（冲击力度测量）
 * - 敲击检测验证（Tap事件识别）
 * - 统计信息分析（均值、标准差）
 * - 温度监控（实时温度显示）
 */
#include "../../board/example_config.h"

#if ENABLE_EXAMPLE_QMI8658_PRECISION

// ========== 1. 头文件包含 ==========
#include "qmi8658_precision_example.h"
#include "../../drivers/power_en/power_en.h"
#include "../../drivers/i2c/i2c_bus.h"
#include "../../board/board_pins.h"
#include "../../lib/u8g2/port/u8g2_port.h"
#include "../../drivers/qmi8658/qmi8658a.h"
#include "../../drivers/qmi8658/qmi8658_reg.h"
#include "../../drivers/oled/oled_utils.h"  // 添加scaled_int_to_str函数
#include "battery_monitor.h"  // 电量监控模块
#include "os/os_api.h"
#include "system/event.h"
#include "typedef.h"
#include <math.h>

static u8g2_t u8g2;

// ========== 2. 模块配置宏 ==========
#define ENABLE_MODULE_DATA_DISPLAY  1   // 数据显示模块
#define ENABLE_MODULE_LEVEL         1   // 水平仪模块
#define ENABLE_MODULE_PEAK_TEST     1   // 峰值测试模块
#define ENABLE_MODULE_TAP_DETECT    0   // 敲击检测模块（需要完整寄存器配置，暂时禁用）
#define ENABLE_MODULE_STATS         1   // 统计信息模块
#define ENABLE_MODULE_TEMP_MONITOR  1   // 温度监控模块

// ========== 2.5. 传感器量程配置（智能映射）==========
// 配置量程（与初始化代码保持一致）
#define ACC_RANGE_CONFIG    QMI8658_CTRL2_ACC_RANGE_4G    // 加速度计量程
#define GYR_RANGE_CONFIG    QMI8658_CTRL3_GYR_RANGE_2000DPS  // 陀螺仪量程

// 根据配置自动选择比例因子（与驱动保持一致）
#if (ACC_RANGE_CONFIG == QMI8658_CTRL2_ACC_RANGE_2G)
    #define ACC_SCALE_FACTOR    16384.0f   // ±2g: 16384 LSB/g
#elif (ACC_RANGE_CONFIG == QMI8658_CTRL2_ACC_RANGE_4G)
    #define ACC_SCALE_FACTOR    8192.0f    // ±4g: 8192 LSB/g
#elif (ACC_RANGE_CONFIG == QMI8658_CTRL2_ACC_RANGE_8G)
    #define ACC_SCALE_FACTOR    4096.0f    // ±8g: 4096 LSB/g
#elif (ACC_RANGE_CONFIG == QMI8658_CTRL2_ACC_RANGE_16G)
    #define ACC_SCALE_FACTOR    2048.0f    // ±16g: 2048 LSB/g
#else
    #error "Unsupported accelerometer range!"
#endif

#if (GYR_RANGE_CONFIG == QMI8658_CTRL3_GYR_RANGE_250DPS)
    #define GYR_SCALE_FACTOR    131.0f     // ±250dps
#elif (GYR_RANGE_CONFIG == QMI8658_CTRL3_GYR_RANGE_500DPS)
    #define GYR_SCALE_FACTOR    65.5f      // ±500dps
#elif (GYR_RANGE_CONFIG == QMI8658_CTRL3_GYR_RANGE_1000DPS)
    #define GYR_SCALE_FACTOR    32.8f      // ±1000dps
#elif (GYR_RANGE_CONFIG == QMI8658_CTRL3_GYR_RANGE_2000DPS)
    #define GYR_SCALE_FACTOR    16.4f      // ±2000dps
#else
    #error "Unsupported gyroscope range!"
#endif

#define TEMP_SCALE_FACTOR   256.0f    // 温度

// ========== 3. 数据结构定义 ==========

/**
 * @brief 显示模式枚举
 */
typedef enum {
    MODE_RAW = 0,           // RAW原始值
    MODE_PHYSICAL,          // 物理量(g, dps)
    MODE_ACCEL_ONLY,        // 仅加速度
    MODE_GYRO_ONLY,         // 仅角速度
#if ENABLE_MODULE_LEVEL
    MODE_LEVEL,             // 水平仪
#endif
#if ENABLE_MODULE_PEAK_TEST
    MODE_PEAK_TEST,         // 峰值测试
#endif
#if ENABLE_MODULE_TAP_DETECT
    MODE_TAP_DETECT,        // 敲击检测
#endif
#if ENABLE_MODULE_STATS
    MODE_STATS,             // 统计信息
#endif
#if ENABLE_MODULE_TEMP_MONITOR
    MODE_TEMP,              // 温度监控
#endif
    MODE_COUNT              // 模式总数
} display_mode_t;

/**
 * @brief 峰值数据结构
 */
#if ENABLE_MODULE_PEAK_TEST
typedef struct {
    float acc_peak_x, acc_peak_y, acc_peak_z;  // 加速度峰值(g)
    float gyr_peak_x, gyr_peak_y, gyr_peak_z;  // 角速度峰值(dps)
    uint32_t peak_timestamp;                    // 峰值时间戳
} peak_data_t;
#endif

/**
 * @brief 统计数据结构
 */
#if ENABLE_MODULE_STATS
typedef struct {
    float acc_x_sum, acc_y_sum, acc_z_sum;
    float acc_x_sq_sum, acc_y_sq_sum, acc_z_sq_sum;
    float gyr_x_sum, gyr_y_sum, gyr_z_sum;
    float gyr_x_sq_sum, gyr_y_sq_sum, gyr_z_sq_sum;
    uint32_t sample_count;
} stats_data_t;
#endif

// ========== 4. 全局变量 ==========
static display_mode_t g_current_mode = MODE_RAW;
static uint8_t g_frozen = 0;  // 显示冻结标志
static battery_info_t* g_battery_info = NULL;  // 电量信息指针

#if ENABLE_MODULE_PEAK_TEST
static peak_data_t g_peak = {0};
#endif

#if ENABLE_MODULE_STATS
static stats_data_t g_stats = {0};
#endif

#if ENABLE_MODULE_TAP_DETECT
static uint8_t g_tap_threshold = 50;  // Tap阈值默认值（范围1-255，值越小越灵敏）
static uint8_t g_single_tap_count = 0;
static uint8_t g_double_tap_count = 0;
#endif

// ========== 5. 按键事件处理 ==========

/**
 * @brief 按键事件处理函数（由 app_spp_and_le.c 调用）
 * @param key_value 按键值（0=KEY1, 1=KEY2, 2=KEY3, 3=KEY4）
 * @param event_type 事件类型（KEY_EVENT_CLICK短按, KEY_EVENT_LONG长按）
 */
void example_key_handler(u8 key_value, u8 event_type) {
    if (event_type != KEY_EVENT_CLICK) return;
    
    switch (key_value) {
        case 1:  // KEY1: 切换显示模式
            g_current_mode = (g_current_mode + 1) % MODE_COUNT;
            printf("[MODE] Switch to mode %d\n", g_current_mode);
            break;
            
        case 2:  // KEY2: 校准传感器
            printf("[CAL] Starting calibration...\n");
            if (QMI8658_CalibrateGyro() == 0) {
                QMI8658_CalibrateAccel();
                printf("[CAL] Calibration done\n");
            }
            break;
            
        case 3:  // KEY3: 功能键（根据当前模式）
#if ENABLE_MODULE_PEAK_TEST
            if (g_current_mode == MODE_PEAK_TEST) {
                memset(&g_peak, 0, sizeof(peak_data_t));
                printf("[PEAK] Reset peak data\n");
            }
#endif
#if ENABLE_MODULE_TAP_DETECT
            if (g_current_mode == MODE_TAP_DETECT) {
                g_tap_threshold = (g_tap_threshold + 10) % 200;
                printf("[TAP] Threshold: %d\n", g_tap_threshold);
            }
#endif
            break;
            
        case 4:  // KEY4: 冻结显示
            g_frozen = !g_frozen;
            printf("[DISPLAY] %s\n", g_frozen ? "Frozen" : "Active");
            break;
    }
}

// ========== 6. 辅助函数 ==========

/**
 * @brief 限制数值范围
 */
static int clamp(int value, int min_val, int max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

/**
 * @brief 绘制顶部电量状态栏
 * @note 占用 y=0~12 区域，内容区从 y=14 开始
 */
static void draw_battery_status_bar(void) {
    if (!g_battery_info) return;
    
    char str[16];
    
    // === 绘制状态栏背景框 ===
    u8g2_DrawFrame(&u8g2, 0, 0, 128, 12);  // 外边框
    
    // === 左侧：电池图标 + 百分比 ===
    u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);
    u8g2_DrawStr(&u8g2, 2, 9, "BAT:");
    
    // 绘制电池外框（12x6像素）
    u8g2_DrawFrame(&u8g2, 26, 3, 12, 6);
    
    // 绘制电池正极（右侧小突起）
    u8g2_DrawBox(&u8g2, 38, 4, 2, 4);
    
    // 根据电量填充电池内部
    uint8_t fill_width = (g_battery_info->percentage * 10) / 100;
    if (fill_width > 0) {
        u8g2_DrawBox(&u8g2, 27, 4, fill_width, 4);
    }
    
    // 显示百分比
    sprintf(str, "%d%%", g_battery_info->percentage);
    u8g2_DrawStr(&u8g2, 42, 9, str);
    
    // === 右侧：电压值 ===
    sprintf(str, "%dmV", g_battery_info->voltage_mv);
    u8g2_DrawStr(&u8g2, 88, 9, str);
    
    // === 低电量警告（红色闪烁效果用实心框表示）===
    if (g_battery_info->is_low_battery) {
        static uint8_t blink_count = 0;
        if (++blink_count % 10 == 0) {  // 每10帧闪烁一次（更快）
            u8g2_SetDrawColor(&u8g2, 0);  // 黑色
            u8g2_DrawBox(&u8g2, 1, 1, 126, 10);  // 填充背景
            u8g2_SetDrawColor(&u8g2, 1);  // 恢复白色
            u8g2_DrawStr(&u8g2, 40, 9, "LOW!");  // 显示警告
        }
    }
    
    // === 分隔线（状态栏和内容区之间）===
    u8g2_DrawHLine(&u8g2, 0, 12, 128);
}

/**
 * @brief 加速度原始值转换为g
 * @param raw 原始数据（int16）
 * @return 加速度值（单位：g）
 */
static float convert_acc_to_g(int16_t raw) {
    return (float)raw / ACC_SCALE_FACTOR;
}

/**
 * @brief 陀螺仪原始值转换为dps
 * @param raw 原始数据（int16）
 * @return 角速度值（单位：dps）
 */
static float convert_gyro_to_dps(int16_t raw) {
    return (float)raw / GYR_SCALE_FACTOR;
}

/**
 * @brief 温度原始值转换为摄氏度
 * @param raw 原始数据（int16）
 * @return 温度值（单位：°C）
 */
static float convert_temp_to_c(int16_t raw) {
    return 25.0f + (float)raw / TEMP_SCALE_FACTOR;
}

#if ENABLE_MODULE_STATS
/**
 * @brief 计算均值和标准差
 */
static void calculate_stddev(float sum, float sq_sum, uint32_t count,
                              float *mean, float *stddev) {
    if (count == 0) {
        *mean = 0;
        *stddev = 0;
        return;
    }
    *mean = sum / count;
    float variance = (sq_sum / count) - (*mean * *mean);
    *stddev = sqrtf(variance > 0 ? variance : 0);
}
#endif

// ========== 7. 模块实现 ==========

#if ENABLE_MODULE_DATA_DISPLAY
/**
 * @brief 数据显示模式（左右分栏布局）
 */
static void display_data_mode(display_mode_t mode, 
                               QMI8658_Data_t *raw_data,
                               QMI8658_Physical_t *phys_data) {
    char str[32];
    
    printf("[DISPLAY_FUNC] mode=%d, RAW_ACC=%d/%d/%d\n", 
           mode, raw_data->acc_x, raw_data->acc_y, raw_data->acc_z);
    
    u8g2_ClearBuffer(&u8g2);
    
    // 绘制电量状态栏
    draw_battery_status_bar();
    
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
    
    // 仅在左右分栏模式绘制分隔线（Y起点从 0 改为 14，长度从 64 改为 50）
    if (mode == MODE_RAW || mode == MODE_PHYSICAL) {
        u8g2_DrawVLine(&u8g2, 63, 14, 50);
    }
    
    switch (mode) {
        case MODE_RAW:
            printf("[RAW_MODE] Drawing RAW data\n");
            // 标题（Y=14+4=18）
            u8g2_DrawStr(&u8g2, 0, 22, "[RAW]");
            
            // 左侧：加速度原始值（所有Y坐标+12）
            u8g2_DrawStr(&u8g2, 2, 36, "AX:");
            sprintf(str, "%d", -raw_data->acc_x);
            u8g2_DrawStr(&u8g2, 20, 36, str);
            
            u8g2_DrawStr(&u8g2, 2, 48, "AY:");
            sprintf(str, "%d", -raw_data->acc_y);
            u8g2_DrawStr(&u8g2, 20, 48, str);
            
            u8g2_DrawStr(&u8g2, 2, 60, "AZ:");
            sprintf(str, "%d", -raw_data->acc_z);
            u8g2_DrawStr(&u8g2, 20, 60, str);
            
            // 右侧：陀螺仪原始值（所有Y坐标+12）
            u8g2_DrawStr(&u8g2, 66, 36, "GX:");
            sprintf(str, "%d", -raw_data->gyr_x);
            u8g2_DrawStr(&u8g2, 84, 36, str);
            
            u8g2_DrawStr(&u8g2, 66, 48, "GY:");
            sprintf(str, "%d", -raw_data->gyr_y);
            u8g2_DrawStr(&u8g2, 84, 48, str);
            
            u8g2_DrawStr(&u8g2, 66, 60, "GZ:");
            sprintf(str, "%d", -raw_data->gyr_z);
            u8g2_DrawStr(&u8g2, 84, 60, str);
            break;
            
        case MODE_PHYSICAL:
            printf("[PHYSICAL_MODE] Drawing PHYSICAL data\n");
            // 标题（Y=22）
            u8g2_DrawStr(&u8g2, 0, 22, "[PHYSICAL]");
            
            // 左侧：加速度(g)（所有Y坐标+12）
            u8g2_DrawStr(&u8g2, 2, 36, "X:");
            scaled_int_to_str(-raw_data->acc_x, str, 5, (int)ACC_SCALE_FACTOR);
            u8g2_DrawStr(&u8g2, 12, 36, str);
            
            u8g2_DrawStr(&u8g2, 2, 48, "Y:");
            scaled_int_to_str(-raw_data->acc_y, str, 5, (int)ACC_SCALE_FACTOR);
            u8g2_DrawStr(&u8g2, 12, 48, str);
            
            u8g2_DrawStr(&u8g2, 2, 60, "Z:");
            scaled_int_to_str(-raw_data->acc_z, str, 5, (int)ACC_SCALE_FACTOR);
            u8g2_DrawStr(&u8g2, 12, 60, str);
            
            // 右侧：陀螺仪(dps)（所有Y坐标+12）
            u8g2_DrawStr(&u8g2, 66, 36, "X:");
            scaled_int_to_str(-raw_data->gyr_x, str, 5, (int)GYR_SCALE_FACTOR);
            u8g2_DrawStr(&u8g2, 76, 36, str);
            
            u8g2_DrawStr(&u8g2, 66, 48, "Y:");
            scaled_int_to_str(-raw_data->gyr_y, str, 5, (int)GYR_SCALE_FACTOR);
            u8g2_DrawStr(&u8g2, 76, 48, str);
            
            u8g2_DrawStr(&u8g2, 66, 60, "Z:");
            scaled_int_to_str(-raw_data->gyr_z, str, 5, (int)GYR_SCALE_FACTOR);
            u8g2_DrawStr(&u8g2, 76, 60, str);
            break;
            
        case MODE_ACCEL_ONLY:
            u8g2_DrawStr(&u8g2, 0, 22, "[ACCEL ONLY]");
            
            // 左侧：标签（Y从36/48/60）
            u8g2_DrawStr(&u8g2, 5, 40, "X:");
            u8g2_DrawStr(&u8g2, 5, 52, "Y:");
            u8g2_DrawStr(&u8g2, 5, 64, "Z:");
            
            // 右侧：数值
            scaled_int_to_str(-raw_data->acc_x, str, 5, (int)ACC_SCALE_FACTOR);
            u8g2_DrawStr(&u8g2, 25, 40, str);
            
            scaled_int_to_str(-raw_data->acc_y, str, 5, (int)ACC_SCALE_FACTOR);
            u8g2_DrawStr(&u8g2, 25, 52, str);
            
            scaled_int_to_str(-raw_data->acc_z, str, 5, (int)ACC_SCALE_FACTOR);
            u8g2_DrawStr(&u8g2, 25, 64, str);
            break;
            
        case MODE_GYRO_ONLY:
            u8g2_DrawStr(&u8g2, 0, 22, "[GYRO ONLY]");
            
            // 左侧：标签
            u8g2_DrawStr(&u8g2, 5, 40, "X:");
            u8g2_DrawStr(&u8g2, 5, 52, "Y:");
            u8g2_DrawStr(&u8g2, 5, 64, "Z:");
            
            // 右侧：数值
            scaled_int_to_str(-raw_data->gyr_x, str, 5, (int)GYR_SCALE_FACTOR);
            u8g2_DrawStr(&u8g2, 25, 40, str);
            
            scaled_int_to_str(-raw_data->gyr_y, str, 5, (int)GYR_SCALE_FACTOR);
            u8g2_DrawStr(&u8g2, 25, 52, str);
            
            scaled_int_to_str(-raw_data->gyr_z, str, 5, (int)GYR_SCALE_FACTOR);
            u8g2_DrawStr(&u8g2, 25, 64, str);
            break;
    }
    
    u8g2_SendBuffer(&u8g2);
}
#endif

#if ENABLE_MODULE_LEVEL
/**
 * @brief 水平仪模式（左数据右平衡球布局）
 */
static void level_mode_task(QMI8658_Data_t *data) {
    // 直接使用原始数据，考虑背靠背安装反转坐标
    int16_t acc_x = -data->acc_x;  // 反转X
    int16_t acc_y = -data->acc_y;  // 反转Y
    int16_t acc_z = -data->acc_z;  // 反转Z
    
    // 计算俯仰角和横滚角
    float acc_x_g = (float)acc_x / ACC_SCALE_FACTOR;
    float acc_y_g = (float)acc_y / ACC_SCALE_FACTOR;
    float acc_z_g = (float)acc_z / ACC_SCALE_FACTOR;
    
    // Pitch: 前后倾斜（绕X轴），使用Y和Z分量
    float pitch = atan2f(acc_y_g, sqrtf(acc_x_g*acc_x_g + acc_z_g*acc_z_g)) * 180.0f / 3.14159f;
    // Roll: 左右倾斜（绕Y轴），使用X和Z分量  
    float roll = atan2f(-acc_x_g, acc_z_g) * 180.0f / 3.14159f;
    
    // 检查角度是否有效（防止NaN或无穷大）
    if (pitch != pitch || roll != roll) {  // NaN检测
        pitch = 0.0f;
        roll = 0.0f;
    }
    
    // 计算小球位置（映射到右侧64x64区域）
    // 右侧区域中心：(96, 32)，范围±32像素
    int ball_x = 96 + (int)(roll * 1.5f);      // 灵敏度1.5
    int ball_y = 32 - (int)(pitch * 1.5f);     // 反转Y轴方向
    
    // 限制在64x64区域内
    ball_x = clamp(ball_x, 64, 127);   // 右侧区域左右边界
    ball_y = clamp(ball_y, 0, 63);     // 上下边界
    
    u8g2_ClearBuffer(&u8g2);
    
    // 绘制电量状态栏
    draw_battery_status_bar();
    
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
    
    // === 左侧数据显示区（0-63像素，Y从14开始）===
    
    // 标题（Y=22）
    u8g2_DrawStr(&u8g2, 0, 22, "LEVEL");
    
    // Pitch角度（Y=36）
    char str[32];
    int pitch_int = (int)(pitch * 10);
    int pitch_abs = (pitch_int < 0) ? -pitch_int : pitch_int;
    sprintf(str, "P:%d.%d", pitch_int/10, pitch_abs%10);
    u8g2_DrawStr(&u8g2, 0, 36, str);
    
    // Roll角度（Y=48）
    int roll_int = (int)(roll * 10);
    int roll_abs = (roll_int < 0) ? -roll_int : roll_int;
    sprintf(str, "R:%d.%d", roll_int/10, roll_abs%10);
    u8g2_DrawStr(&u8g2, 0, 48, str);
    
    // Z轴加速度（Y=60）
    int acc_z_fixed = (int)(acc_z_g * 100);
    int acc_z_abs = (acc_z_fixed < 0) ? -acc_z_fixed : acc_z_fixed;
    sprintf(str, "Z:%d.%dg", acc_z_fixed/100, acc_z_abs%100);
    u8g2_DrawStr(&u8g2, 0, 60, str);
    
    // === 右侧平衡球显示区（64-127像素，Y从14开始）===
    
    // 绘制64x50边界框（高度从54改为50，Y起点从10改为14）
    u8g2_DrawFrame(&u8g2, 64, 14, 64, 50);
    
    // 绘制十字准星（中心点96,39）
    u8g2_DrawHLine(&u8g2, 64, 39, 64);   // 水平线
    u8g2_DrawVLine(&u8g2, 96, 14, 50);   // 垂直线
    
    // 绘制小球（ball_y需要+14偏移）
    u8g2_DrawDisc(&u8g2, ball_x, ball_y + 14, 4, U8G2_DRAW_ALL);
    
    u8g2_SendBuffer(&u8g2);
}
#endif

#if ENABLE_MODULE_PEAK_TEST
/**
 * @brief 更新峰值数据
 */
static void update_peak_data(QMI8658_Physical_t *phys) {
    if (fabsf(phys->acc_x_g) > fabsf(g_peak.acc_peak_x)) {
        g_peak.acc_peak_x = phys->acc_x_g;
        g_peak.peak_timestamp = os_time_get();
    }
    if (fabsf(phys->acc_y_g) > fabsf(g_peak.acc_peak_y)) {
        g_peak.acc_peak_y = phys->acc_y_g;
        g_peak.peak_timestamp = os_time_get();
    }
    if (fabsf(phys->acc_z_g) > fabsf(g_peak.acc_peak_z)) {
        g_peak.acc_peak_z = phys->acc_z_g;
        g_peak.peak_timestamp = os_time_get();
    }
}

/**
 * @brief 峰值测试模式
 */
static void peak_test_mode(QMI8658_Physical_t *phys) {
    update_peak_data(phys);
    
    u8g2_ClearBuffer(&u8g2);
    
    // 绘制电量状态栏
    draw_battery_status_bar();
    
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
    
    // 标题（Y=22）
    u8g2_DrawStr(&u8g2, 0, 22, "[PEAK TEST]");
    
    char str[32];
    // 所有Y坐标+12
    sprintf(str, "Cur AX:%d.%02dg", (int)(phys->acc_x_g * 100), (int)(fabsf(phys->acc_x_g) * 100) % 100);
    u8g2_DrawStr(&u8g2, 0, 34, str);
    
    sprintf(str, "Pk  AX:%d.%02dg*", (int)(g_peak.acc_peak_x * 100), (int)(fabsf(g_peak.acc_peak_x) * 100) % 100);
    u8g2_DrawStr(&u8g2, 0, 46, str);
    
    sprintf(str, "Cur AY:%d.%02dg", (int)(phys->acc_y_g * 100), (int)(fabsf(phys->acc_y_g) * 100) % 100);
    u8g2_DrawStr(&u8g2, 0, 58, str);
    
    u8g2_SendBuffer(&u8g2);
}
#endif

#if ENABLE_MODULE_TAP_DETECT
/**
 * @brief 配置Tap检测参数
 * @note QMI8658的Tap检测需要在初始化时配置阈值和时序参数
 */
static void configure_tap_detection(void) {
    // Step 1: 配置Tap阈值和时序参数
    // CTRL8寄存器布局：
    // bit7-6: 保留
    // bit5:   TAP_EN (1=使能)
    // bit4:   PEDO_EN (0=禁用计步器)
    // bit3-2: 保留
    // bit1-0: WOM_TH[1:0] (运动唤醒阈值)
    
    // 首先禁用所有运动检测功能
    i2c_bus_write_buf(BOARD_IMU_I2C_ADDR7, 
        (uint8_t[]){QMI8658_REG_CTRL8, 0x00}, 2);
    os_time_dly(5);
    
    // Step 2: 发送Tap配置命令到CTRL9
    // 这个命令会让传感器加载Tap配置参数
    i2c_bus_write_buf(BOARD_IMU_I2C_ADDR7, 
        (uint8_t[]){QMI8658_REG_CTRL9, QMI8658_CMD_CFG_TAP}, 2);
    os_time_dly(10);
    
    // Step 3: 使能Tap检测
    // 设置CTRL8的bit5为1，使能Tap检测
    i2c_bus_write_buf(BOARD_IMU_I2C_ADDR7, 
        (uint8_t[]){QMI8658_REG_CTRL8, QMI8658_CTRL8_TAP_EN}, 2);
    os_time_dly(10);
    
    printf("[TAP] Detection configured and enabled\n");
}

/**
 * @brief 检查Tap事件
 */
static void check_tap_event(void) {
    uint8_t tap_status = i2c_bus_read_reg8(BOARD_IMU_I2C_ADDR7, QMI8658_REG_TAP_STATUS);
    
    if (tap_status & 0x01) {
        g_single_tap_count++;
        printf("[TAP] Single Tap detected! (Count: %d)\n", g_single_tap_count);
    }
    if (tap_status & 0x02) {
        g_double_tap_count++;
        printf("[TAP] Double Tap detected! (Count: %d)\n", g_double_tap_count);
    }
}

/**
 * @brief 敲击检测模式
 */
static void tap_detect_mode(void) {
    check_tap_event();
    
    u8g2_ClearBuffer(&u8g2);
    
    // 绘制电量状态栏
    draw_battery_status_bar();
    
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
    
    // 标题（Y=22）
    u8g2_DrawStr(&u8g2, 0, 22, "[TAP DETECT]");
    
    char str[32];
    // 所有Y坐标+12
    sprintf(str, "Thresh:%d", g_tap_threshold);
    u8g2_DrawStr(&u8g2, 0, 36, str);
    
    sprintf(str, "Single:%d", g_single_tap_count);
    u8g2_DrawStr(&u8g2, 0, 48, str);
    
    sprintf(str, "Double:%d", g_double_tap_count);
    u8g2_DrawStr(&u8g2, 0, 60, str);
    
    u8g2_SendBuffer(&u8g2);
}
#endif

#if ENABLE_MODULE_STATS
/**
 * @brief 更新统计数据
 */
static void update_stats(QMI8658_Physical_t *phys) {
    g_stats.sample_count++;
    
    g_stats.acc_x_sum += phys->acc_x_g;
    g_stats.acc_x_sq_sum += phys->acc_x_g * phys->acc_x_g;
    g_stats.acc_y_sum += phys->acc_y_g;
    g_stats.acc_y_sq_sum += phys->acc_y_g * phys->acc_y_g;
    g_stats.acc_z_sum += phys->acc_z_g;
    g_stats.acc_z_sq_sum += phys->acc_z_g * phys->acc_z_g;
    
    g_stats.gyr_x_sum += phys->gyr_x_dps;
    g_stats.gyr_x_sq_sum += phys->gyr_x_dps * phys->gyr_x_dps;
    g_stats.gyr_y_sum += phys->gyr_y_dps;
    g_stats.gyr_y_sq_sum += phys->gyr_y_dps * phys->gyr_y_dps;
    g_stats.gyr_z_sum += phys->gyr_z_dps;
    g_stats.gyr_z_sq_sum += phys->gyr_z_dps * phys->gyr_z_dps;
}

/**
 * @brief 统计信息模式（显示加速度和陀螺仪的均值与标准差）
 */
static void stats_mode(QMI8658_Physical_t *phys) {
    update_stats(phys);
    
    float mean_x, stddev_x;
    float mean_y, stddev_y;
    float mean_z, stddev_z;
    
    calculate_stddev(g_stats.acc_x_sum, g_stats.acc_x_sq_sum, 
                     g_stats.sample_count, &mean_x, &stddev_x);
    calculate_stddev(g_stats.acc_y_sum, g_stats.acc_y_sq_sum, 
                     g_stats.sample_count, &mean_y, &stddev_y);
    calculate_stddev(g_stats.acc_z_sum, g_stats.acc_z_sq_sum, 
                     g_stats.sample_count, &mean_z, &stddev_z);
    
    u8g2_ClearBuffer(&u8g2);
    
    // 绘制电量状态栏
    draw_battery_status_bar();
    
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
    
    char str[32];
    sprintf(str, "[STATS] N=%lu", g_stats.sample_count);
    u8g2_DrawStr(&u8g2, 0, 22, str);
    
    // X轴统计（Y=34）
    int mean_x_fixed = (int)(mean_x * 100);
    int std_x_fixed = (int)(stddev_x * 100);
    sprintf(str, "AX:%d.%02d S:%d.%02d", 
            mean_x_fixed/100, (mean_x_fixed<0?-mean_x_fixed:mean_x_fixed)%100,
            std_x_fixed/100, (std_x_fixed<0?-std_x_fixed:std_x_fixed)%100);
    u8g2_DrawStr(&u8g2, 0, 34, str);
    
    // Y轴统计（Y=46）
    int mean_y_fixed = (int)(mean_y * 100);
    int std_y_fixed = (int)(stddev_y * 100);
    sprintf(str, "AY:%d.%02d S:%d.%02d", 
            mean_y_fixed/100, (mean_y_fixed<0?-mean_y_fixed:mean_y_fixed)%100,
            std_y_fixed/100, (std_y_fixed<0?-std_y_fixed:std_y_fixed)%100);
    u8g2_DrawStr(&u8g2, 0, 46, str);
    
    // Z轴统计（Y=58）
    int mean_z_fixed = (int)(mean_z * 100);
    int std_z_fixed = (int)(stddev_z * 100);
    sprintf(str, "AZ:%d.%02d S:%d.%02d", 
            mean_z_fixed/100, (mean_z_fixed<0?-mean_z_fixed:mean_z_fixed)%100,
            std_z_fixed/100, (std_z_fixed<0?-std_z_fixed:std_z_fixed)%100);
    u8g2_DrawStr(&u8g2, 0, 58, str);
    
    u8g2_SendBuffer(&u8g2);
}
#endif

#if ENABLE_MODULE_TEMP_MONITOR
/**
 * @brief 温度监控模式（显示芯片内部温度）
 * @note QMI8658温度传感器测量的是芯片结温，需要校准才能反映真实温度
 */
static void temp_monitor_mode(void) {
    // 使用驱动函数读取温度
    float temperature = 0.0f;
    QMI8658_ReadTemperature(&temperature);
    
    // 温度校准：减去偏移量使读数接近实际温度
    // 校准方法：在已知环境温度下（如25°C），记录显示值，计算偏移
    // 例如：环境25°C时显示52°C，则偏移=52-25=27°C
    #define TEMP_CALIBRATION_OFFSET  22.0f  // 校准偏移量（可根据实际情况调整）
    float calibrated_temp = temperature - TEMP_CALIBRATION_OFFSET;
    
    u8g2_ClearBuffer(&u8g2);
    
    // 绘制电量状态栏
    draw_battery_status_bar();
    
    // === 温度显示 - 居中美观布局 ===
    
    // 绘制外框
    u8g2_DrawFrame(&u8g2, 10, 20, 108, 40);
    
    // 标题（居中）
    u8g2_SetFont(&u8g2, u8g2_font_7x13_tr);
    u8g2_DrawStr(&u8g2, 48, 35, "TEMP");
    
    // 温度值（大字体，居中）
    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
    char str[32];
    int temp_fixed = (int)(calibrated_temp * 100);
    int temp_abs = (temp_fixed < 0) ? -temp_fixed : temp_fixed;
    sprintf(str, "%d.%02d", temp_fixed/100, temp_abs%100);
    
    // 计算字符串宽度并居中
    uint16_t str_width = u8g2_GetStrWidth(&u8g2, str);
    u8g2_DrawStr(&u8g2, (128 - str_width) / 2, 52, str);
    
    // 单位符号
    u8g2_SetFont(&u8g2, u8g2_font_7x13_tr);
    u8g2_DrawStr(&u8g2, 95, 52, "°C");
    
    u8g2_SendBuffer(&u8g2);
}
#endif

// ========== 8. 初始化流程 ==========

/**
 * @brief QMI8658精确测量示例初始化
 */
static int qmi8658_precision_init(void) {
    int ret;
    uint8_t whoami;
    
    // Step 1: 读取WHOAMI验证连接
    ret = i2c_bus_read_reg8(BOARD_IMU_I2C_ADDR7, QMI8658_REG_WHO_AM_I);
    if (ret < 0) {
        printf("[QMI8658] ERROR: read WHOAMI failed\n");
        return -1;
    }
    whoami = (uint8_t)ret;
    printf("[QMI8658] WHOAMI: 0x%02X\n", whoami);
    
    // Step 2: 软复位
    i2c_bus_write_buf(BOARD_IMU_I2C_ADDR7, 
                      (uint8_t[]){QMI8658_REG_RESET, QMI8658_SOFT_RESET_VAL}, 2);
    os_time_dly(5);  // ✅ 进一步优化：从10降到5（50ms足够）
    
    // Step 3: 使能地址自动递增
    i2c_bus_write_buf(BOARD_IMU_I2C_ADDR7, 
                      (uint8_t[]){QMI8658_REG_CTRL1, QMI8658_CTRL1_ADDR_AI_EN}, 2);
    
    // Step 4: 配置加速度计（使用配置宏）
    uint8_t ctrl2 = ACC_RANGE_CONFIG | QMI8658_CTRL2_ACC_ODR_512HZ;
    i2c_bus_write_buf(BOARD_IMU_I2C_ADDR7, 
                      (uint8_t[]){QMI8658_REG_CTRL2, ctrl2}, 2);
    
    // Step 5: 配置陀螺仪（使用配置宏）
    uint8_t ctrl3 = GYR_RANGE_CONFIG | QMI8658_CTRL3_GYR_ODR_512HZ;
    i2c_bus_write_buf(BOARD_IMU_I2C_ADDR7, 
                      (uint8_t[]){QMI8658_REG_CTRL3, ctrl3}, 2);
    
    // Step 6: 配置低通滤波器：50Hz
    uint8_t ctrl5 = QMI8658_CTRL5_ACC_LPF_50HZ | QMI8658_CTRL5_GYR_LPF_50HZ;
    i2c_bus_write_buf(BOARD_IMU_I2C_ADDR7, 
                      (uint8_t[]){QMI8658_REG_CTRL5, ctrl5}, 2);
    
    // Step 7: 使能传感器（ACC + GYR + TEMP）
    uint8_t ctrl7 = QMI8658_CTRL7_ACC_EN | QMI8658_CTRL7_GYR_EN | QMI8658_CTRL7_TEMP_EN;
    i2c_bus_write_buf(BOARD_IMU_I2C_ADDR7, 
                      (uint8_t[]){QMI8658_REG_CTRL7, ctrl7}, 2);
    
    // Step 8: 【可选】配置Tap检测
#if ENABLE_MODULE_TAP_DETECT
    configure_tap_detection();
#endif
    
    // Step 9: 等待传感器稳定
    os_time_dly(10);  // ✅ 进一步优化：从20降到10（100ms足够）
    
    // Step 10: 验证数据就绪
    ret = i2c_bus_read_reg8(BOARD_IMU_I2C_ADDR7, QMI8658_REG_STATUSINT);
    if (ret >= 0) {
        printf("[QMI8658] Status: 0x%02X\n", (uint8_t)ret);
    }
    
    printf("[QMI8658] Init OK\n");
    return 0;
}

// ========== 9. 示例主任务 ==========

/**
 * @brief QMI8658精确测量示例主任务
 */
static void qmi8658_precision_example_task(void *p_arg) {
    
    // ---- Step 1: 基础硬件初始化 ----
    
    // 电源使能
    power_en_enable(1);
    os_time_dly(10);  // 等待电源稳定
    
    // 初始化电量监控
    battery_monitor_init();
    g_battery_info = battery_monitor_get_info();
    
    // I2C总线初始化
    board_i2c_bus0_init();
    
    // u8g2显示屏初始化
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, 
        u8g2_byte_cb, u8g2_gpio_and_delay_cb);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    
    // ✅ 立即亮屏：显示启动画面（让用户立刻看到反馈）
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
    u8g2_DrawStr(&u8g2, 10, 25, "QMI8658");
    u8g2_DrawStr(&u8g2, 5, 45, "Precision");
    u8g2_SendBuffer(&u8g2);
    // ⚡ 不延迟，立即继续初始化
    
    // ---- Step 2: 显示加载中提示 ----
    
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
    u8g2_DrawStr(&u8g2, 0, 12, "Loading...");
    u8g2_DrawStr(&u8g2, 0, 26, "QMI8658 IMU");
    u8g2_DrawStr(&u8g2, 0, 40, "6-Axis Sensor");
    u8g2_DrawStr(&u8g2, 0, 54, "Accel + Gyro");
    u8g2_SendBuffer(&u8g2);
    // ⚡ 不延迟，立即开始传感器初始化
    
    // ---- Step 3: 初始化QMI8658传感器 ----
    
    // ✅ 优化：跳过I2C扫描以加快启动（已知QMI8658地址为0x6A）
    // i2c_bus_scan();  // 开发阶段可取消注释用于调试
    
    if (qmi8658_precision_init() < 0) {
        printf("QMI8658 precision init failed!\n");
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
        u8g2_DrawStr(&u8g2, 10, 30, "Init Failed");
        u8g2_SendBuffer(&u8g2);
        while (1) os_time_dly(100);
    }
    
    // ✅ 初始化成功：立即显示Ready（无延迟）
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_7x13_tr);
    u8g2_DrawStr(&u8g2, 15, 25, "QMI8658");
    u8g2_DrawStr(&u8g2, 20, 40, "Ready!");
    u8g2_SendBuffer(&u8g2);
    os_time_dly(20);  // ✅ 仅保留短暂显示（0.2秒）
    
    // ---- Step 4: 进入主循环 ----
    
    QMI8658_Data_t raw_data;
    QMI8658_Physical_t phys_data;
    
    while (1) {
        // 更新电量数据（内部有5秒间隔控制）
        battery_monitor_update();
        
        if (g_frozen) {
            os_time_dly(10);
            continue;
        }
        
        // 读取传感器数据
        if (QMI8658_ReadData(&raw_data) == 0) {
            // 转换为物理量（考虑背靠背安装，反转X、Y、Z轴）
            phys_data.acc_x_g = -convert_acc_to_g(raw_data.acc_x);  // 反转X
            phys_data.acc_y_g = -convert_acc_to_g(raw_data.acc_y);  // 反转Y
            phys_data.acc_z_g = -convert_acc_to_g(raw_data.acc_z);  // 反转Z
            phys_data.gyr_x_dps = -convert_gyro_to_dps(raw_data.gyr_x);  // 反转X
            phys_data.gyr_y_dps = -convert_gyro_to_dps(raw_data.gyr_y);  // 反转Y
            phys_data.gyr_z_dps = -convert_gyro_to_dps(raw_data.gyr_z);  // 反转Z
            
            // 读取温度
            uint8_t temp_buf[2];
            i2c_bus_read_buf(BOARD_IMU_I2C_ADDR7, QMI8658_REG_TEMP_L, temp_buf, 2);
            int16_t raw_temp = (int16_t)((temp_buf[1] << 8) | temp_buf[0]);
            phys_data.temp_c = convert_temp_to_c(raw_temp);
            
            // 调试日志：每100次打印一次数据
            static uint32_t debug_count = 0;
            if (++debug_count % 100 == 0) {
                // 将浮点数转换为定点数进行格式化（避免嵌入式系统sprintf浮点问题）
                int acc_x_fixed = (int)(phys_data.acc_x_g * 100);
                int acc_y_fixed = (int)(phys_data.acc_y_g * 100);
                int acc_z_fixed = (int)(phys_data.acc_z_g * 100);
                int gyr_x_fixed = (int)(phys_data.gyr_x_dps * 10);
                int gyr_y_fixed = (int)(phys_data.gyr_y_dps * 10);
                int gyr_z_fixed = (int)(phys_data.gyr_z_dps * 10);
                
                printf("[DEBUG] Mode=%d, RAW_ACC=%d/%d/%d, RAW_GYR=%d/%d/%d\n",
                       g_current_mode,
                       raw_data.acc_x, raw_data.acc_y, raw_data.acc_z,
                       raw_data.gyr_x, raw_data.gyr_y, raw_data.gyr_z);
                printf("[DEBUG] PHYSICAL: ACC=%d.%02d/%d.%02d/%d.%02d, GYR=%d.%d/%d.%d/%d.%d\n",
                       acc_x_fixed/100, (acc_x_fixed<0?-acc_x_fixed:acc_x_fixed)%100,
                       acc_y_fixed/100, (acc_y_fixed<0?-acc_y_fixed:acc_y_fixed)%100,
                       acc_z_fixed/100, (acc_z_fixed<0?-acc_z_fixed:acc_z_fixed)%100,
                       gyr_x_fixed/10, (gyr_x_fixed<0?-gyr_x_fixed:gyr_x_fixed)%10,
                       gyr_y_fixed/10, (gyr_y_fixed<0?-gyr_y_fixed:gyr_y_fixed)%10,
                       gyr_z_fixed/10, (gyr_z_fixed<0?-gyr_z_fixed:gyr_z_fixed)%10);
                
                // 静止检测：如果设备水平静止，Z轴应接近±1g
                if (debug_count % 500 == 0) {
                    int calc_z_fixed = (int)((float)raw_data.acc_z / ACC_SCALE_FACTOR * 100);
                    int after_invert_fixed = (int)(-(float)raw_data.acc_z / ACC_SCALE_FACTOR * 100);
                    printf("[Z-AXIS CHECK] RAW_Z=%d, SCALE=%d, CALC_Z=%d.%02dg\n", 
                           raw_data.acc_z, (int)ACC_SCALE_FACTOR,
                           calc_z_fixed/100, (calc_z_fixed<0?-calc_z_fixed:calc_z_fixed)%100);
                    printf("[Z-AXIS CHECK] After invert: %d.%02dg (should be +1g when screen up)\n",
                           after_invert_fixed/100, (after_invert_fixed<0?-after_invert_fixed:after_invert_fixed)%100);
                }
            }
            
            // 根据当前模式显示
            switch (g_current_mode) {
#if ENABLE_MODULE_DATA_DISPLAY
                case MODE_RAW:
                case MODE_PHYSICAL:
                case MODE_ACCEL_ONLY:
                case MODE_GYRO_ONLY:
                    printf("[DISPLAY] Entering data display mode %d\n", g_current_mode);
                    display_data_mode(g_current_mode, &raw_data, &phys_data);
                    break;
#endif
                    
#if ENABLE_MODULE_LEVEL
                case MODE_LEVEL:
                    level_mode_task(&raw_data);  // 使用原始数据
                    break;
#endif
                    
#if ENABLE_MODULE_PEAK_TEST
                case MODE_PEAK_TEST:
                    peak_test_mode(&phys_data);
                    break;
#endif
                    
#if ENABLE_MODULE_TAP_DETECT
                case MODE_TAP_DETECT:
                    tap_detect_mode();
                    break;
#endif
                    
#if ENABLE_MODULE_STATS
                case MODE_STATS:
                    stats_mode(&phys_data);
                    break;
#endif
                    
#if ENABLE_MODULE_TEMP_MONITOR
                case MODE_TEMP:
                    temp_monitor_mode();
                    break;
#endif
                    
                default:
                    break;
            }
        }
        
        os_time_dly(1);  // 5ms延迟，最大化响应速度
    }
}

// ========== 10. 启动函数 ==========

/**
 * @brief 启动QMI8658精确测量示例
 */
void qmi8658_precision_example_start(void) {
    os_task_create(qmi8658_precision_example_task, NULL, 10, 2048, 0, "qmi8658_prec");
}

#endif /* ENABLE_EXAMPLE_QMI8658_PRECISION */
