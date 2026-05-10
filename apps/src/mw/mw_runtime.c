/**
 * @file mw_runtime.c
 * @brief 中间层实现：根据配置选择并启动对应的示例
 * 
 * 注意：此文件只负责示例选择，不包含任何业务逻辑
 */
#include "mw_runtime.h"
#include "../board/example_config.h"

#if ENABLE_EXAMPLE_QMI8658
    #include "../examples/example_qmi8658/qmi8658_example.h"
#elif ENABLE_EXAMPLE_U8G2_DASHBOARD
    #include "../examples/example_u8g2_dashboard/u8g2_dashboard.h"
#elif ENABLE_EXAMPLE_GYRO_BALL
    #include "../examples/example_gyro_ball/gyro_ball_example.h"
#elif ENABLE_EXAMPLE_QMI8658_PRECISION
    #include "../examples/example_qmi8658_precision/qmi8658_precision_example.h"
#endif

/**
 * @brief 中间层初始化（根据配置启动对应示例）
 */
void mw_runtime_init(void) {
#if ENABLE_EXAMPLE_QMI8658
    qmi8658_example_start();
    
#elif ENABLE_EXAMPLE_U8G2_DASHBOARD
    u8g2_dashboard_start();
    
#elif ENABLE_EXAMPLE_GYRO_BALL
    gyro_ball_example_start();
    
#elif ENABLE_EXAMPLE_QMI8658_PRECISION
    qmi8658_precision_example_start();
    
#else
    #error "未启用任何示例！请检查 board/example_config.h"
#endif
}