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
