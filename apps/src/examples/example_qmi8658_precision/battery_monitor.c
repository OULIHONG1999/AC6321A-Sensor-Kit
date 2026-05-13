#include "battery_monitor.h"
#include "generic/typedef.h"
#include "asm/adc_api.h"  // ADC API
#include "os/os_api.h"

static battery_info_t g_battery_info = {0};
static u32 g_last_update_time = 0;

#define BATTERY_UPDATE_INTERVAL  2000  // 2秒更新一次（提高响应速度）

/**
 * @brief 电压转百分比（线性模型）
 * @param voltage_mv 电压值(mV)
 * @return 百分比(0-100)
 *
 * 锂电池放电曲线简化：
 * 4200mV = 100%
 * 3300mV = 0%
 * 中间线性映射
 */
static u8 voltage_to_percentage(u32 voltage_mv) {
    if (voltage_mv >= 4200) return 100;
    if (voltage_mv <= 3300) return 0;

    // 线性映射：(voltage - 3300) / (4200 - 3300) * 100
    return (u8)((voltage_mv - 3300) * 100 / 900);
}

void battery_monitor_init(void) {
    // 首次立即更新
    battery_monitor_update();
}

battery_info_t* battery_monitor_get_info(void) {
    return &g_battery_info;
}

void battery_monitor_update(void) {
    u32 current_time = os_time_get();

    // 检查是否需要更新（避免频繁读取ADC）
    if (current_time - g_last_update_time < BATTERY_UPDATE_INTERVAL) {
        return;
    }

    // 读取电池电压（注意：AD_CH_VBAT是1/4分压，需*4）
    u32 voltage_mv = adc_get_voltage(AD_CH_VBAT) * 4;

    // 合理性检查（防止异常值）
    if (voltage_mv < 2000 || voltage_mv > 5000) {
        printf("[BATTERY] Invalid voltage: %dmV\n", voltage_mv);
        return;
    }

    // 更新数据
    g_battery_info.voltage_mv = voltage_mv;
    g_battery_info.percentage = voltage_to_percentage(voltage_mv);
    g_battery_info.is_low_battery = (g_battery_info.percentage < 20) ? 1 : 0;

    g_last_update_time = current_time;

    // 低电量警告
    if (g_battery_info.is_low_battery) {
        printf("⚠️ LOW BATTERY: %d%% (%dmV)\n",
               g_battery_info.percentage,
               g_battery_info.voltage_mv);
    }
}
