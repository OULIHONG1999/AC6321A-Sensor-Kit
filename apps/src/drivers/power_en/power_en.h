#ifndef __POWER_EN_H__
#define __POWER_EN_H__
#include "asm/gpio.h"

void power_en_init(void);
void power_en_enable(u8 en);
#endif /* __POWER_EN_H__ */