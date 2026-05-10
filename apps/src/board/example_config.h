/**
 * @file example_config.h
 * @brief 示例选择配置（只能开启一个示例）
 * 
 * 使用说明：
 * 1. 将需要启用的示例宏设为 1
 * 2. 其他示例宏必须为 0
 * 3. 每次只能启用一个示例
 * 
 * 修改此文件后，需要重新编译项目
 */
#ifndef EXAMPLE_CONFIG_H
#define EXAMPLE_CONFIG_H

/* ========== 示例选择（只能开启一个）========== */

// QMI8658 IMU 传感器示例
#define ENABLE_EXAMPLE_QMI8658          1

// u8g2 图形库仪表盘演示
#define ENABLE_EXAMPLE_U8G2_DASHBOARD   0

// BME280 温湿度气压传感器示例（待添加）
#define ENABLE_EXAMPLE_BME280           0

// VL53L0X 激光测距传感器示例（待添加）
#define ENABLE_EXAMPLE_VL53L0X          0

// ... 后续添加更多示例

/* ========== 配置检查（编译时检查是否只启用了一个示例）========== */
#define ENABLED_EXAMPLE_COUNT ( \
    ENABLE_EXAMPLE_QMI8658 + \
    ENABLE_EXAMPLE_U8G2_DASHBOARD + \
    ENABLE_EXAMPLE_BME280 + \
    ENABLE_EXAMPLE_VL53L0X \
)

#if ENABLED_EXAMPLE_COUNT == 0
    #error "请至少启用一个示例！修改 board/example_config.h"
#elif ENABLED_EXAMPLE_COUNT > 1
    #error "只能启用一个示例！当前启用了多个，请修改 board/example_config.h"
#endif

#endif /* EXAMPLE_CONFIG_H */
