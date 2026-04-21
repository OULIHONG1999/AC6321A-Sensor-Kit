#include "oled_utils.h"

void int_to_str(int num, char *str, int width) {
  int i = 0;
  int is_negative = 0;
  
  if (num < 0) {
    is_negative = 1;
    num = -num;
  }
  
  if (num == 0) {
    str[i++] = '0';
  }
  
  while (num > 0) {
    str[i++] = (num % 10) + '0';
    num /= 10;
  }
  
  if (is_negative) {
    str[i++] = '-';
  }
  
  while (i < width) {
    str[i++] = ' ';
  }
  
  str[i] = '\0';
  
  int start = 0;
  int end = i - 1;
  while (start < end) {
    char temp = str[start];
    str[start] = str[end];
    str[end] = temp;
    start++;
    end--;
  }
}

void scaled_int_to_str(int16_t raw, char *str, int width, int scale_factor) {
  int i = 0;
  int is_negative = 0;
  int scaled_value;
  
  if (raw < 0) {
    is_negative = 1;
    scaled_value = (-raw * 100) / scale_factor;
  } else {
    scaled_value = (raw * 100) / scale_factor;
  }
  
  int int_part = scaled_value / 100;
  int frac_part = scaled_value % 100;
  
  if (is_negative) {
    str[i++] = '-';
  }
  
  char int_buf[10];
  int j = 0;
  int temp = int_part;
  if (temp == 0) {
    int_buf[j++] = '0';
  }
  while (temp > 0) {
    int_buf[j++] = (temp % 10) + '0';
    temp /= 10;
  }
  for (int k = j - 1; k >= 0; k--) {
    str[i++] = int_buf[k];
  }
  
  str[i++] = '.';
  str[i++] = (frac_part / 10) + '0';
  str[i++] = (frac_part % 10) + '0';
  
  while (i < width) {
    str[i++] = ' ';
  }
  
  str[i] = '\0';
}
