#include "u8g2_port.h"
#include "../../../board/board_pins.h"
#include <string.h>

/**
 * @brief 毫秒延迟
 */
void u8g2_delay_ms(uint32_t ms)
{
    os_time_dly(ms);  // 使用系统延迟
}

/**
 * @brief 微秒延迟（简化实现，实际精度可能不足）
 */
void u8g2_delay_us(uint32_t us)
{
    volatile uint32_t i;
    for (i = 0; i < us * 10; i++) {  // 粗略延迟，根据主频调整系数
        __asm__("nop");
    }
}

/**
 * @brief GPIO 和延迟回调函数
 */
uint8_t u8g2_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg) {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            // I2C 总线已在 board_i2c_bus0_init() 中初始化
            break;
            
        case U8X8_MSG_DELAY_MILLI:
            u8g2_delay_ms(arg_int);
            break;
            
        case U8X8_MSG_DELAY_10MICRO:
            u8g2_delay_us(10);
            break;
            
        case U8X8_MSG_DELAY_100NANO:
            __asm__("nop");
            break;
            
        default:
            return 0;
    }
    
    return 1;
}

/**
 * @brief I2C 字节通信回调
 */
uint8_t u8g2_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    static uint8_t buffer[129];  // 1字节命令 + 128字节数据
    static uint16_t buf_index;
    
    switch (msg) {
        case U8X8_MSG_BYTE_INIT:
            // I2C 已初始化
            break;
            
        case U8X8_MSG_BYTE_START_TRANSFER:
            buf_index = 0;
            break;
            
        case U8X8_MSG_BYTE_SEND:
            // 收集要发送的数据
            {
                uint8_t *data = (uint8_t *)arg_ptr;
                uint16_t len = arg_int;
                
                if (buf_index + len < sizeof(buffer)) {
                    memcpy(&buffer[buf_index], data, len);
                    buf_index += len;
                }
            }
            break;
            
        case U8X8_MSG_BYTE_END_TRANSFER:
            // 发送收集的数据
            if (buf_index > 0) {
                i2c_bus_write_buf(BOARD_OLED_I2C_ADDR7, buffer, buf_index);
            }
            break;
            
        default:
            return 0;
    }
    
    return 1;
}
