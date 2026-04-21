/**
 * @file oled.c
 * @brief SSD1306 I2C 驱动（72×40）；总线由上层 board_i2c_bus0_init，本文件仅发屏命令与数据。
 */
#include "oled.h"
#include "stdlib.h"
#include "oledfont.h"
#include "../i2c/i2c_bus.h"

u8 OLED_GRAM[OLED_GRAM_COLUMNS][OLED_PAGE_NUM];

#define OLED_COL_ADDR_HIGH (0x10 | ((OLED_COLUMN_ADDR_OFFSET >> 4) & 0x0F))
#define OLED_COL_ADDR_LOW (OLED_COLUMN_ADDR_OFFSET & 0x0F)

void OLED_ColorTurn(u8 i)
{
	if (i == 0) {
		OLED_WR_Byte(0xA6, OLED_CMD);
	}
	if (i == 1) {
		OLED_WR_Byte(0xA7, OLED_CMD);
	}
}

void OLED_DisplayTurn(u8 i)
{
	if (i == 0) {
		OLED_WR_Byte(0xC8, OLED_CMD);
		OLED_WR_Byte(0xA1, OLED_CMD);
	}
	if (i == 1) {
		OLED_WR_Byte(0xC0, OLED_CMD);
		OLED_WR_Byte(0xA0, OLED_CMD);
	}
}

void OLED_WR_Byte(u8 dat, u8 mode)
{
	u8 buf[2];
	buf[0] = mode ? 0x40 : 0x00;
	buf[1] = dat;
	(void)i2c_bus_write_buf(BOARD_OLED_I2C_ADDR7, buf, 2);
}


void OEL_SendCmdParam(CmdParam_t *cmd_param)
{
	u8 i;

	if (cmd_param == NULL) {
		return;
	}
	OLED_WR_Byte(cmd_param->cmd_code, OLED_CMD);
	if (cmd_param->param_num > 0) {
		for (i = 0; i < cmd_param->param_num && i < 4; i++) {
			OLED_WR_Byte(cmd_param->params[i], OLED_CMD);
		}
	}
}

void OLED_DisPlay_On(void)
{
	OLED_WR_Byte(0x8D, OLED_CMD);
	OLED_WR_Byte(0x14, OLED_CMD);
	OLED_WR_Byte(0xAF, OLED_CMD);
}

void OLED_DisPlay_Off(void)
{
	OLED_WR_Byte(0x8D, OLED_CMD);
	OLED_WR_Byte(0x10, OLED_CMD);
	OLED_WR_Byte(0xAE, OLED_CMD);
}


void OLED_Refresh(void)
{
	u8 i, n;
	for (i = 0; i < OLED_PAGE_NUM; i++) {
		OLED_WR_Byte(0xb0 + i, OLED_CMD); //设置页面起始地址
		OLED_WR_Byte(OLED_COL_ADDR_LOW, OLED_CMD);   //设置低列起始地址
		OLED_WR_Byte(OLED_COL_ADDR_HIGH, OLED_CMD);   //设置高列起始地址
		{
			u8 buf[1 + OLED_GRAM_COLUMNS];
			buf[0] = 0x40;
			for (n = 0; n < OLED_GRAM_COLUMNS; n++) {
				buf[1 + n] = OLED_GRAM[n][i];
			}
			(void)i2c_bus_write_buf(BOARD_OLED_I2C_ADDR7, buf, sizeof(buf));
		}
	}
}

void OLED_Fill(u8 fill)
{
	u8 i, n;
	for (i = 0; i < OLED_PAGE_NUM; i++) {
		for (n = 0; n < OLED_GRAM_COLUMNS; n++) {
			OLED_GRAM[n][i] = fill;
		}
	}
	OLED_Refresh();
}

void OLED_Set_Pos(u8 x, u8 y_page)
{
	if (y_page >= OLED_PAGE_NUM || x >= OLED_GRAM_COLUMNS) {
		return;
	}
	OLED_WR_Byte((u8)(0xb0 + y_page), OLED_CMD);
	OLED_WR_Byte((u8)(0x00 | (x & 0x0Fu)), OLED_CMD);
	OLED_WR_Byte((u8)(0x10 | ((x >> 4) & 0x0Fu)), OLED_CMD);
}

void OLED_Contrast(u8 contrast)
{
	OLED_WR_Byte(0x81, OLED_CMD);
	OLED_WR_Byte(contrast, OLED_CMD);
}

void OLED_Clear(void)
{
	u8 i, n;
	for (i = 0; i < OLED_PAGE_NUM; i++) {
		for (n = 0; n < OLED_GRAM_COLUMNS; n++) {
			OLED_GRAM[n][i] = 0;
		}
	}
	OLED_Refresh();
}

void OLED_DrawPoint(u8 x, u8 y, u8 t)
{
	u8 i, m, n;

	if (x >= OLED_GRAM_COLUMNS || y >= OLED_PIXEL_HEIGHT) {
		return;
	}
	i = (u8)(y / 8);
	m = (u8)(y % 8);
	n = (u8)(1 << m);
	if (t) {
		OLED_GRAM[x][i] |= n;
	} else {
		OLED_GRAM[x][i] = (u8)(~OLED_GRAM[x][i]);
		OLED_GRAM[x][i] |= n;
		OLED_GRAM[x][i] = (u8)(~OLED_GRAM[x][i]);
	}
}

void OLED_ClearPoint(u8 x, u8 y)
{
	u8 i, m, n;

	if (x >= OLED_GRAM_COLUMNS || y >= OLED_PIXEL_HEIGHT) {
		return;
	}
	i = (u8)(y / 8);
	m = (u8)(y % 8);
	n = (u8)(1 << m);
	OLED_GRAM[x][i] = (u8)(OLED_GRAM[x][i] & (u8)(~n));
}

void OLED_DrawLine(u8 x1, u8 y1, u8 x2, u8 y2, u8 mode)
{
	u16 t;
	int xerr = 0, yerr = 0, delta_x, delta_y, distance;
	int incx, incy, uRow, uCol;

	delta_x = x2 - x1;
	delta_y = y2 - y1;
	uRow = x1;
	uCol = y1;
	if (delta_x > 0) {
		incx = 1;
	} else if (delta_x == 0) {
		incx = 0;
	} else {
		incx = -1;
		delta_x = -delta_x;
	}
	if (delta_y > 0) {
		incy = 1;
	} else if (delta_y == 0) {
		incy = 0;
	} else {
		incy = -1;
		delta_y = -delta_y;
	}
	if (delta_x > delta_y) {
		distance = delta_x;
	} else {
		distance = delta_y;
	}
	for (t = 0; t < (u16)(distance + 1); t++) {
		OLED_DrawPoint((u8)uRow, (u8)uCol, mode);
		xerr += delta_x;
		yerr += delta_y;
		if (xerr > distance) {
			xerr -= distance;
			uRow += incx;
		}
		if (yerr > distance) {
			yerr -= distance;
			uCol += incy;
		}
	}
}

void OLED_DrawRect(u8 x1, u8 y1, u8 x2, u8 y2, u8 mode)
{
	OLED_DrawLine(x1, y1, x2, y1, mode);
	OLED_DrawLine(x2, y1, x2, y2, mode);
	OLED_DrawLine(x2, y2, x1, y2, mode);
	OLED_DrawLine(x1, y2, x1, y1, mode);
}

void OLED_DrawCircle(u8 x, u8 y, u8 r)
{
	int a, b, num;
	a = 0;
	b = r;
	while (2 * b * b >= r * r) {
		OLED_DrawPoint((u8)(x + a), (u8)(y - b), 1);
		OLED_DrawPoint((u8)(x - a), (u8)(y - b), 1);
		OLED_DrawPoint((u8)(x - a), (u8)(y + b), 1);
		OLED_DrawPoint((u8)(x + a), (u8)(y + b), 1);
		OLED_DrawPoint((u8)(x + b), (u8)(y + a), 1);
		OLED_DrawPoint((u8)(x + b), (u8)(y - a), 1);
		OLED_DrawPoint((u8)(x - b), (u8)(y - a), 1);
		OLED_DrawPoint((u8)(x - b), (u8)(y + a), 1);
		a++;
		num = (a * a + b * b) - r * r;
		if (num > 0) {
			b--;
			a--;
		}
	}
}

void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 size1, u8 mode)
{
	u8 i, m, temp, size2, chr1;
	u8 x0 = x, y0 = y;

	if (size1 == 8) {
		size2 = 6;
	} else {
		size2 = (u8)((size1 / 8 + ((size1 % 8) ? 1 : 0)) * (size1 / 2));
	}
	chr1 = (u8)(chr - ' ');
	for (i = 0; i < size2; i++) {
		if (size1 == 8) {
			temp = asc2_0806[chr1][i];
		} else if (size1 == 12) {
			temp = asc2_1206[chr1][i];
		} else if (size1 == 16) {
			temp = asc2_1608[chr1][i];
		} else if (size1 == 24) {
			temp = asc2_2412[chr1][i];
		} else {
			return;
		}
		for (m = 0; m < 8; m++) {
			if (temp & 0x01) {
				OLED_DrawPoint(x, y, mode);
			} else {
				OLED_DrawPoint(x, y, !mode);
			}
			temp >>= 1;
			y++;
		}
		x++;
		if ((size1 != 8) && ((x - x0) == size1 / 2)) {
			x = x0;
			y0 = (u8)(y0 + 8);
		}
		y = y0;
	}
}

void OLED_ShowChar6x8(u8 x, u8 y, u8 chr, u8 mode)
{
	OLED_ShowChar(x, y, chr, 8, mode);
}

void OLED_ShowString(u8 x, u8 y, u8 *chr, u8 size1, u8 mode)
{
	while ((*chr >= ' ') && (*chr <= '~')) {
		OLED_ShowChar(x, y, *chr, size1, mode);
		if (size1 == 8) {
			x += 6;
		} else {
			x += size1 / 2;
		}
		chr++;
	}
}

u32 OLED_Pow(u8 m, u8 n)
{
	u32 result = 1;
	while (n--) {
		result *= m;
	}
	return result;
}

void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 size1, u8 mode)
{
	u8 t, temp, m = 0;

	if (size1 == 8) {
		m = 2;
	}
	for (t = 0; t < len; t++) {
		temp = (u8)((num / OLED_Pow(10, len - t - 1)) % 10);
		OLED_ShowChar((u8)(x + (size1 / 2 + m) * t), y, (u8)(temp + '0'), size1, mode);
	}
}

void OLED_ShowChinese(u8 x, u8 y, u8 num, u8 size1, u8 mode)
{
	u8 m, temp;
	u8 x0 = x, y0 = y;
	u16 i, size3 = (u16)((size1 / 8 + ((size1 % 8) ? 1 : 0)) * size1);

	for (i = 0; i < size3; i++) {
		if (size1 == 16) {
			temp = Hzk1[num][i];
		} else if (size1 == 24) {
			temp = Hzk2[num][i];
		} else if (size1 == 32) {
			temp = Hzk3[num][i];
		} else if (size1 == 64) {
			temp = Hzk4[num][i];
		} else {
			return;
		}
		for (m = 0; m < 8; m++) {
			if (temp & 0x01) {
				OLED_DrawPoint(x, y, mode);
			} else {
				OLED_DrawPoint(x, y, !mode);
			}
			temp >>= 1;
			y++;
		}
		x++;
		if ((x - x0) == size1) {
			x = x0;
			y0 = (u8)(y0 + 8);
		}
		y = y0;
	}
}

void OLED_ScrollDisplay(u8 num, u8 space, u8 mode)
{
	u8 i, n, t = 0, m = 0, r;

	while (1) {
		if (m == 0) {
			OLED_ShowChinese(OLED_VISIBLE_WIDTH, 24, t, 16, mode); //写入一个汉字到OLED_GRAM[][]数组
			t++;
		}
		if (t == num) {
			for (r = 0; r < 16 * space; r++)      //显示间隔
			 {
				for (i = 1; i < OLED_GRAM_COLUMNS; i++)
					{
						for (n = 0; n < OLED_PAGE_NUM; n++)
						{
							OLED_GRAM[i-1][n] = OLED_GRAM[i][n];
						}
					}
			    OLED_Refresh();
			 }
		    t=0;
		  }
		m++;
		if(m==16){m=0;}
		for(i=1;i<OLED_GRAM_COLUMNS;i++)   //实现滚动
		{
			for(n=0;n<OLED_PAGE_NUM;n++)
			{
				OLED_GRAM[i-1][n]=OLED_GRAM[i][n];
			}
		}
		OLED_Refresh();
	}
}

void OLED_GramShiftLeftOneColumn(void)
{
	u8 i, n;

	for (n = 0; n < OLED_PAGE_NUM; n++) {
		for (i = 1; i < OLED_GRAM_COLUMNS; i++) {
			OLED_GRAM[i - 1][n] = OLED_GRAM[i][n];
		}
		OLED_GRAM[OLED_GRAM_COLUMNS - 1][n] = 0;
	}
	OLED_Refresh();
}

void OLED_ShowPicture(u8 x, u8 y, u8 sizex, u8 sizey, u8 BMP[], u8 mode)
{
	u16 j = 0;
	u8 i, n, temp, m;
	u8 x0 = x, y0 = y;
	u16 total_bytes = (u16)(sizex * ((sizey / 8) + ((sizey % 8) ? 1 : 0)));

	sizey = (u8)(sizey / 8 + ((sizey % 8) ? 1 : 0));
	for (n = 0; n < sizey; n++) {
		for (i = 0; i < sizex; i++) {
			if (j < total_bytes) {
				temp = BMP[j];
				j++;
			} else {
				temp = 0;
			}
			for (m = 0; m < 8; m++) {
				if (temp & 0x01) {
					OLED_DrawPoint(x, y, mode);
				} else {
					OLED_DrawPoint(x, y, !mode);
				}
				temp >>= 1;
				y++;
			}
			x++;
			if ((x - x0) == sizex) {
				x = x0;
				y0 = (u8)(y0 + 8);
			}
			y = y0;
		}
	}
}

void OLED_Init(void)
{
	static CmdParam_t oel_init_cmds[] = {
		{0xAE, 0, {0}}, // 关闭显示
		{0xD5, 1, {0x80}}, // 设置显示时钟分频比/振荡器频率
		{0xA8, 1, {0x3F}}, // 设置多路复用率(1到64)
		{0xD3, 1, {0x00}}, // 设置显示偏移
		{0x40, 0, {0}}, // 设置起始行地址
		{0x8D, 1, {0x14}}, // 设置电荷泵启用/禁用
		{0x20, 1, {0x02}}, // 设置页面寻址模式
		{0xA1, 0, {0}}, // 设置段/列映射
		{0xC8, 0, {0}}, // 设置COM/行扫描方向
		{0xDA, 1, {0x12}}, // 设置COM引脚硬件配置
		{0x81, 1, {0xCF}}, // 设置对比度控制寄存器
		{0xD9, 1, {0xF1}}, // 设置预充电周期
		{0xDB, 1, {0x30}}, // 设置VCOMH
		{0xA4, 0, {0}}, // 设置正常显示
		{0xA6, 0, {0}}, // 设置正常/反转显示
		{OLED_COL_ADDR_HIGH, 0, {0}}, // 设置高列地址
		{OLED_COL_ADDR_LOW, 0, {0}}, // 设置低列地址
		{0xAF, 0, {0}}, // 开启显示
	};
	u8 idx;
	const u8 ncmd = (u8)(sizeof(oel_init_cmds) / sizeof(oel_init_cmds[0]));

	if (hal_pin_valid(IIC_RES)) {
		OLED_RES_Clr();
		delay_ms(200);
		OLED_RES_Set();
	} else {
		delay_ms(10);
	}

	for (idx = 0; idx < ncmd; idx++) {
		OEL_SendCmdParam(&oel_init_cmds[idx]);
	}
	OLED_Clear();
	OLED_WR_Byte(0xAF, OLED_CMD);
}
