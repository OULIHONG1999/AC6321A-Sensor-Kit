/**
 * @file oled_config.h
 * @brief 本板 SSD1306 I2C 屏几何参数（与 doc 中景园 1.3 例程逻辑对应，分辨率为 128×64）。
 */
#ifndef OLED_CONFIG_H
#define OLED_CONFIG_H

#define OLED_GRAM_COLUMNS 128
#define OLED_PAGE_NUM 8
#define OLED_PIXEL_HEIGHT (OLED_PAGE_NUM * 8)
#define OLED_VISIBLE_WIDTH 128
#define OLED_COLUMN_ADDR_OFFSET 0x02u

#endif /* OLED_CONFIG_H */
