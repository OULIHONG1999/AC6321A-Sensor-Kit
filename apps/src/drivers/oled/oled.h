#ifndef __OLED_H
#define __OLED_H

#include "stdlib.h"
#include "system/includes.h"
#include "../../board/board_pins.h"
#include "oled_config.h"
#include "../../hal/hal_gpio.h"
#define delay_ms os_time_dly

#define IIC_SDA BOARD_OLED_I2C_SDA
#define IIC_SCL BOARD_OLED_I2C_SCL
#define IIC_RES BOARD_OLED_I2C_RES

#define OLED_SCL_Clr() hal_gpio_write(IIC_SCL, 0)
#define OLED_SCL_Set() hal_gpio_write(IIC_SCL, 1)
#define OLED_SDA_Clr() hal_gpio_write(IIC_SDA, 0)
#define OLED_SDA_Set() hal_gpio_write(IIC_SDA, 1)
#define OLED_RES_Clr() hal_gpio_write(IIC_RES, 0)
#define OLED_RES_Set() hal_gpio_write(IIC_RES, 1)

#define OLED_CMD 0
#define OLED_DATA 1

#include <stdint.h>

typedef struct {
	u8 cmd_code;
	u8 param_num;
	u8 params[4];
} CmdParam_t;

void OLED_ColorTurn(u8 i);
void OLED_DisplayTurn(u8 i);
void OLED_WR_Byte(u8 dat, u8 mode);
void OLED_DisPlay_On(void);
void OLED_DisPlay_Off(void);
void OLED_Refresh(void);
void OLED_Clear(void);
void OLED_Fill(u8 fill);
void OLED_Set_Pos(u8 x, u8 y_page);
void OLED_Contrast(u8 contrast);
void OLED_DrawPoint(u8 x, u8 y, u8 t);
void OLED_ClearPoint(u8 x, u8 y);
void OLED_DrawLine(u8 x1, u8 y1, u8 x2, u8 y2, u8 mode);
void OLED_DrawRect(u8 x1, u8 y1, u8 x2, u8 y2, u8 mode);
void OLED_DrawCircle(u8 x, u8 y, u8 r);
void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 size1, u8 mode);
void OLED_ShowChar6x8(u8 x, u8 y, u8 chr, u8 mode);
void OLED_ShowString(u8 x, u8 y, u8 *chr, u8 size1, u8 mode);
void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 size1, u8 mode);
void OLED_ShowChinese(u8 x, u8 y, u8 num, u8 size1, u8 mode);
void OLED_ScrollDisplay(u8 num, u8 space, u8 mode);
void OLED_GramShiftLeftOneColumn(void);
void OLED_ShowPicture(u8 x, u8 y, u8 sizex, u8 sizey, u8 BMP[], u8 mode);
#define OLED_DrawBMP OLED_ShowPicture
u32 OLED_Pow(u8 m, u8 n);
void OEL_SendCmdParam(CmdParam_t *cmd_param);
void OLED_Init(void);

#endif
