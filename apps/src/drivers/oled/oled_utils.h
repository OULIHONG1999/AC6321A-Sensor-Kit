#ifndef OLED_UTILS_H
#define OLED_UTILS_H

#include <stdint.h>

void int_to_str(int num, char *str, int width);
void scaled_int_to_str(int16_t raw, char *str, int width, int scale_factor);

#endif /* OLED_UTILS_H */
