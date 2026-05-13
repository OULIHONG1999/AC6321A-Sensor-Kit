#ifndef __BATTERY_MONITOR_H__
#define __BATTERY_MONITOR_H__

#include "typedef.h"
#include "generic/typedef.h"

/**
 * @brief 电池信息结构
 */
typedef struct {
    u32 voltage_mv;        // 电池电压(mV)，范围 3300-4200
    u8  percentage;        // 电量百分比(0-100%)
    u8  is_low_battery;    // 低电量标志(<20%)
} battery_info_t;

/**
 * @brief 初始化电量监控模块
 * @note 在任务开始时调用一次
 */
void battery_monitor_init(void);

/**
 * @brief 获取电池信息（指针形式，避免拷贝）
 * @return 电池信息结构指针
 */
battery_info_t* battery_monitor_get_info(void);

/**
 * @brief 更新电池数据（内部调用ADC）
 * @note 建议在主循环中定期调用（如每5秒）
 */
void battery_monitor_update(void);

#endif /* __BATTERY_MONITOR_H__ */
