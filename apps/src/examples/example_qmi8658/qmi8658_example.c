/**
 * @file qmi8658_example.c
 * @brief QMI8658六轴IMU传感器示例 - 完整独立实现
 * 
 * 功能：
 * - 实时显示加速度、角速度、温度数据
 * - 支持按键校准传感器
 * - 自动检测QMI8658A/C型号
 */
#include "../../board/example_config.h"

#if ENABLE_EXAMPLE_QMI8658

// ========== 显示方式选择（在示例内独立配置）==========
// 0 = 使用原生OLED驱动（直接调用OLED函数）
// 1 = 使用u8g2图形库（更丰富的图形功能）
#define USE_U8G2_DISPLAY    1

#if USE_U8G2_DISPLAY
    #include "../../lib/u8g2/port/u8g2_port.h"
    static u8g2_t u8g2;
#endif

#include "qmi8658_example.h"
#include "../../drivers/power_en/power_en.h"
#include "../../drivers/i2c/i2c_bus.h"
#include "../../drivers/oled/oled.h"
#include "../../drivers/oled/oled_utils.h"
#include "../../drivers/qmi8658/qmi8658a.h"
#include "os/os_api.h"
#include "system/event.h"
#include "typedef.h"

// ========== 1. 按键事件处理 ==========

/**
 * @brief 按键事件处理函数（由 app_spp_and_le.c 调用）
 * @param key_value 按键值（0=KEY1, 1=KEY2, 2=KEY3, 3=KEY4）
 * @param event_type 事件类型（KEY_EVENT_CLICK短按, KEY_EVENT_LONG长按）
 */
void example_key_handler(u8 key_value, u8 event_type) {
    // KEY3长按：手动校准传感器
    if (event_type == KEY_EVENT_LONG && key_value == 2) {
#if USE_U8G2_DISPLAY
        // u8g2显示方式
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
        u8g2_DrawStr(&u8g2, 10, 30, "Calibrating");   // y=30: 居中偏上
        u8g2_DrawStr(&u8g2, 25, 50, "Gyro...");       // y=50: 居中
        u8g2_SendBuffer(&u8g2);
#else
        // 原生OLED显示方式
        OLED_Clear();
        OLED_ShowString(16, 16, "Calibrating", 16, 1);
        OLED_ShowString(24, 32, "Gyro...", 16, 1);
        OLED_Refresh();
#endif
        
        if (QMI8658_CalibrateGyro() == 0) {
#if USE_U8G2_DISPLAY
            u8g2_DrawStr(&u8g2, 25, 50, "Accel...");   // y=50: 居中
            u8g2_SendBuffer(&u8g2);
#else
            OLED_ShowString(24, 32, "Accel...", 16, 1);
            OLED_Refresh();
#endif
            QMI8658_CalibrateAccel();
            
#if USE_U8G2_DISPLAY
            u8g2_DrawStr(&u8g2, 30, 50, "Done!");      // y=50: 居中
            u8g2_SendBuffer(&u8g2);
#else
            OLED_ShowString(24, 32, "Done!", 16, 1);
            OLED_Refresh();
#endif
            os_time_dly(100);
        }
    }
}



// ========== 2. 示例主任务 ==========

/**
 * @brief QMI8658示例主任务
 * @param p_arg 任务参数（通常为NULL）
 */
static void qmi8658_example_task(void *p_arg) {
    
    // ---- 2.1 硬件初始化 ----
    
    // 电源使能
    power_en_enable(1);
    os_time_dly(10);  // 等待电源稳定
    
    // I2C总线初始化
    board_i2c_bus0_init();
    i2c_bus_scan();  // 扫描I2C设备
    
    // OLED/u8g2初始化
#if USE_U8G2_DISPLAY
    // u8g2初始化（SSD1306 I2C）
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        &u8g2,
        U8G2_R0,
        u8g2_byte_cb,              // 使用项目中的字节回调
        u8g2_gpio_and_delay_cb     // 使用项目中的GPIO和延时回调
    );
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_ClearBuffer(&u8g2);
#else
    // 原生OLED初始化
    OLED_Init();
    OLED_ColorTurn(0);
    OLED_DisplayTurn(0);
    OLED_Contrast(0xFF);
#endif
    
    
    // ---- 2.2 显示欢迎界面 ----
    
#if USE_U8G2_DISPLAY
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);  // 大字体标题
    u8g2_DrawStr(&u8g2, 20, 30, "QMI8658");      // y=30: 居中偏上
    u8g2_DrawStr(&u8g2, 25, 50, "IMU Test");     // y=50: 居中
    u8g2_SetFont(&u8g2, u8g2_font_7x13_tr);      // 小字体日期
    u8g2_DrawStr(&u8g2, 25, 63, "2026/05/10");   // y=63: 底部
    u8g2_SendBuffer(&u8g2);
#else
    OLED_Clear();
    OLED_ShowString(16, 8, "QMI8658", 16, 1);
    OLED_ShowString(16, 24, "IMU Test", 16, 1);
    OLED_ShowString(16, 40, "2026/05/10", 16, 1);
    OLED_Refresh();
#endif
    os_time_dly(200);  // 显示2秒
    
    
    // ---- 2.3 传感器初始化 ----
    
    if (QMI8658_Init() < 0) {
        printf("QMI8658 init failed!\n");
#if USE_U8G2_DISPLAY
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
        u8g2_DrawStr(&u8g2, 10, 30, "IMU Init");      // y=30: 居中偏上
        u8g2_DrawStr(&u8g2, 15, 50, "Failed!");       // y=50: 居中
        u8g2_SendBuffer(&u8g2);
#else
        OLED_Clear();
        OLED_ShowString(8, 16, "IMU Init", 16, 1);
        OLED_ShowString(8, 32, "Failed!", 16, 1);
        OLED_Refresh();
#endif
        while (1) {
            os_time_dly(100);
        }
    }
    
    
    // ---- 2.4 准备显示界面 ----
    
#if USE_U8G2_DISPLAY
    u8g2_ClearBuffer(&u8g2);
#else
    OLED_Clear();
#endif
    
    // 显示设备型号（左上角）
    QMI8658_Type_t dev_type = QMI8658_GetDeviceType();
#if USE_U8G2_DISPLAY
    u8g2_SetFont(&u8g2, u8g2_font_7x13_tr);
    if (dev_type == QMI8658_TYPE_C) {
        u8g2_DrawStr(&u8g2, 0, 12, "QMI8658C");
    } else {
        u8g2_DrawStr(&u8g2, 0, 12, "QMI8658A");
    }
#else
    if (dev_type == QMI8658_TYPE_C) {
        OLED_ShowString(0, 0, "QMI8658C", 8, 1);
    } else {
        OLED_ShowString(0, 0, "QMI8658A", 8, 1);
    }
#endif
    
    // 显示校准状态
    QMI8658_Calibration_t calib;
    QMI8658_GetCalibration(&calib);
#if USE_U8G2_DISPLAY
    if (calib.calibrated) {
        u8g2_DrawStr(&u8g2, 90, 14, "CAL");
    } else {
        u8g2_DrawStr(&u8g2, 85, 14, "UNCAL");
    }
#else
    if (calib.calibrated) {
        OLED_ShowString(80, 0, "CAL", 8, 1);
    } else {
        OLED_ShowString(80, 0, "UNCAL", 8, 1);
    }
#endif
    
    // 注意：具体数据标签在主循环中绘制
#if USE_U8G2_DISPLAY
    u8g2_SendBuffer(&u8g2);
#else
    OLED_ShowString(0, 16, "A:", 8, 1);
    OLED_ShowString(64, 16, "G:", 8, 1);
    OLED_ShowString(0, 40, "T:", 8, 1);
    OLED_Refresh();
#endif
    
    
    // ---- 2.5 主循环 ----
    
    QMI8658_Data_t data;
    float temperature;
    uint32_t tick_count = 0;
    
    while (1) {
        tick_count++;
        
        // 每60秒自动校准一次（如果未校准）
        if (tick_count % 60000 == 0 && !calib.calibrated) {
#if USE_U8G2_DISPLAY
            u8g2_ClearBuffer(&u8g2);
            u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
            u8g2_DrawStr(&u8g2, 10, 30, "Auto Calib");   // y=30: 居中偏上
            u8g2_DrawStr(&u8g2, 25, 50, "Wait...");      // y=50: 居中
            u8g2_SendBuffer(&u8g2);
#else
            OLED_Clear();
            OLED_ShowString(24, 16, "Auto Calib", 16, 1);
            OLED_ShowString(24, 32, "Wait...", 16, 1);
            OLED_Refresh();
#endif
            
            if (QMI8658_CalibrateGyro() == 0) {
                QMI8658_CalibrateAccel();
                QMI8658_GetCalibration(&calib);
                
                // 重新显示界面
#if USE_U8G2_DISPLAY
                u8g2_ClearBuffer(&u8g2);
                u8g2_SetFont(&u8g2, u8g2_font_7x13_tr);
                if (dev_type == QMI8658_TYPE_C) {
                    u8g2_DrawStr(&u8g2, 0, 14, "QMI8658C");
                } else {
                    u8g2_DrawStr(&u8g2, 0, 14, "QMI8658A");
                }
                u8g2_DrawStr(&u8g2, 90, 14, "CAL");
                u8g2_SendBuffer(&u8g2);
#else
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
#endif
            }
        }
        
        // 读取传感器数据
        if (QMI8658_ReadData(&data) < 0) {
            printf("QMI8658 read data failed\n");
            os_time_dly(10);
            continue;
        }
        
        // 读取温度
        QMI8658_ReadTemperature(&temperature);
        int16_t raw_temp = (int16_t)((temperature - 25.0f) * 256.0f);
        
        // 格式化数据
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
        
        // 显示数据
#if USE_U8G2_DISPLAY
        // === 清除缓冲区，避免残影（重要！）===
        u8g2_ClearBuffer(&u8g2);
        
        // === 128x64屏幕美观布局 ===
        // 第1部分：标题栏 (y=12) - 大字体
        u8g2_SetFont(&u8g2, u8g2_font_ncenB10_tr);  // 10像素高
        if (dev_type == QMI8658_TYPE_C) {
            u8g2_DrawStr(&u8g2, 2, 12, "QMI8658C");
        } else {
            u8g2_DrawStr(&u8g2, 2, 12, "QMI8658A");
        }
        
        // 校准状态指示器
        if (calib.calibrated) {
            u8g2_SetDrawColor(&u8g2, 1);
            u8g2_DrawBox(&u8g2, 100, 4, 6, 6);  // 绿色方块表示已校准
        } else {
            u8g2_SetDrawColor(&u8g2, 1);
            u8g2_DrawFrame(&u8g2, 100, 4, 6, 6);  // 空心框表示未校准
        }
        u8g2_SetDrawColor(&u8g2, 1);  // 恢复绘制颜色
        
        // 分隔线
        u8g2_DrawHLine(&u8g2, 0, 16, 128);
        
        // 第2部分：主数据区 (y=28, 40, 52) - 中等字体
        u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);  // 6×10像素
        
        // 第1行：加速度X / 角速度X
        u8g2_DrawStr(&u8g2, 2, 28, "AX:");
        u8g2_DrawStr(&u8g2, 18, 28, acc_x_str);
        u8g2_DrawStr(&u8g2, 66, 28, "GX:");
        u8g2_DrawStr(&u8g2, 82, 28, gyr_x_str);
        
        // 第2行：加速度Y / 角速度Y
        u8g2_DrawStr(&u8g2, 2, 40, "AY:");
        u8g2_DrawStr(&u8g2, 18, 40, acc_y_str);
        u8g2_DrawStr(&u8g2, 66, 40, "GY:");
        u8g2_DrawStr(&u8g2, 82, 40, gyr_y_str);
        
        // 第3行：加速度Z / 角速度Z
        u8g2_DrawStr(&u8g2, 2, 52, "AZ:");
        u8g2_DrawStr(&u8g2, 18, 52, acc_z_str);
        u8g2_DrawStr(&u8g2, 66, 52, "GZ:");
        u8g2_DrawStr(&u8g2, 82, 52, gyr_z_str);
        
        // 第3部分：状态栏 - 温度显示（右下角）
        u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);  // 最小字体
        u8g2_DrawStr(&u8g2, 2, 62, "T:");
        u8g2_DrawStr(&u8g2, 12, 62, temp_str);
        
        // 发送缓冲区到屏幕
        u8g2_SendBuffer(&u8g2);
#else
        // 原生OLED显示方式
        OLED_ShowString(16, 16, (u8 *)acc_x_str, 8, 1);
        OLED_ShowString(16, 24, (u8 *)acc_y_str, 8, 1);
        OLED_ShowString(16, 32, (u8 *)acc_z_str, 8, 1);
        
        OLED_ShowString(80, 16, (u8 *)gyr_x_str, 8, 1);
        OLED_ShowString(80, 24, (u8 *)gyr_y_str, 8, 1);
        OLED_ShowString(80, 32, (u8 *)gyr_z_str, 8, 1);
        
        OLED_ShowString(16, 40, (u8 *)temp_str, 8, 1);
        
        OLED_Refresh();
#endif
        os_time_dly(1);  // 约1000Hz刷新率
    }
}


// ========== 3. 启动函数 ==========

/**
 * @brief 启动QMI8658示例
 * @note 此函数由 mw_runtime_init() 调用
 */
void qmi8658_example_start(void) {
    // 创建OS任务
    // 参数：任务函数, 参数, 优先级, 栈大小, CPU, 任务名
    os_task_create(qmi8658_example_task, NULL, 5, 1024, 0, "qmi8658_ex");
}

#endif /* ENABLE_EXAMPLE_QMI8658 */
