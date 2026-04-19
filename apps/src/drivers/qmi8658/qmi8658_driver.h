/**
 * @file qmi8658_driver.h
 * @brief QMI8658 器件驱动接口：仅依赖共享 I2C 总线，不包含 UI。
 */
#ifndef QMI8658_DRIVER_H
#define QMI8658_DRIVER_H

#include "typedef.h"

/* 1：WHOAMI 匹配；0：未检测到或总线失败 */
int qmi8658_hw_probe(void);

void qmi8658_driver_init_default(void);

int qmi8658_driver_read_reg_u8(u8 reg, u8 *out);

typedef void (*qmi8658_data_ready_cb_t)(void);

/* 占位：后续接 INT 脚与 GPIO 中断时再实现 */
void qmi8658_driver_register_data_ready(qmi8658_data_ready_cb_t cb);

#endif /* QMI8658_DRIVER_H */
