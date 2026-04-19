/**
 * @file mw_runtime.c
 * @brief 中间层实现：聚合 OLED / IMU 等驱动，应用层不直接依赖寄存器级 API。
 */
#include "mw_runtime.h"
#include "../drivers/i2c/i2c_bus_bb.h"
#include "../drivers/oled/bmp.h"
#include "../drivers/oled/oled.h"
#include "../drivers/qmi8658/qmi8658_driver.h"
#include "os/os_api.h"


void mw_runtime_init(void) {

  power_en_init();
  power_en_enable(1);
  os_time_dly(10);

  board_i2c_bus0_init();
  i2c_bus_bb_scan();
  qmi8658_hw_probe();

  OLED_Init();
  OLED_ColorTurn(0);
  OLED_DisplayTurn(0);
  // 设置OLED亮度为最大
  OLED_Contrast(0xFF);
  OLED_Clear();

  u8 t=' ';

	while(1)
	{
		OLED_ShowPicture(0,0,128,64,BMP1,1);
		OLED_Refresh();
		os_time_dly(200);
		OLED_Clear();
		OLED_ShowChinese(0,0,0,16,1);//中
		OLED_ShowChinese(18,0,1,16,1);//景
		OLED_ShowChinese(36,0,2,16,1);//园
		OLED_ShowChinese(54,0,3,16,1);//电
		OLED_ShowChinese(72,0,4,16,1);//子
		OLED_ShowChinese(90,0,5,16,1);//技
		OLED_ShowChinese(108,0,6,16,1);//术
		OLED_ShowString(8,16,"ZHONGJINGYUAN",16,1);
		OLED_ShowString(20,32,"2014/05/01",16,1);
		OLED_ShowString(0,48,"ASCII:",16,1);  
		OLED_ShowString(63,48,"CODE:",16,1);
		OLED_ShowChar(48,48,t,16,1);//显示ASCII字符	   
		t++;
		if(t>'~')t=' ';
		OLED_ShowNum(103,48,t,3,16,1);
		OLED_Refresh();
		os_time_dly(200);
		OLED_Clear();
		OLED_ShowChinese(0,0,0,16,1);  //16*16 中
	  OLED_ShowChinese(16,0,0,24,1); //24*24 中
		OLED_ShowChinese(24,20,0,32,1);//32*32 中
	  OLED_ShowChinese(64,0,0,64,1); //64*64 中
		OLED_Refresh();
	  os_time_dly(200);
  	OLED_Clear();
		OLED_ShowString(0,0,"ABC",8,1);//6*8 “ABC”
		OLED_ShowString(0,8,"ABC",12,1);//6*12 “ABC”
	  OLED_ShowString(0,20,"ABC",16,1);//8*16 “ABC”
		OLED_ShowString(0,36,"ABC",24,1);//12*24 “ABC”
	  OLED_Refresh();
		os_time_dly(200);
		OLED_ScrollDisplay(11,4,1);
	}
}
