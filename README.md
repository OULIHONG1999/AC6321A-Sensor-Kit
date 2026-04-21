# AC6321A QMI8658 六轴加速度陀螺仪开发板

## 项目概述

本项目基于AC6321A蓝牙SoC芯片，集成了QMI8658六轴加速度陀螺仪传感器和OLED显示屏，实现了人体运动检测功能。

## 硬件组成

- AC6321A蓝牙SoC芯片
- QMI8658六轴加速度陀螺仪传感器（I2C地址：0x6A）
- OLED显示屏（128×64分辨率，支持1.3和0.96英寸屏幕，使用SSD1306驱动，I2C地址：0x3C）
- 四个按键
- 相关外围电路

## 软件功能

- OLED显示屏驱动和显示功能（1.3寸屏幕已设置向右偏移2个单位）
- QMI8658传感器数据采集和处理
- 蓝牙通信功能
- 人体运动检测算法
- 按键输入处理

## QMI8658 驱动功能

### 基础功能
- 传感器初始化 `QMI8658_Init()`
- 数据读取 `QMI8658_ReadData()` / `QMI8658_ReadPhysical()`
- 温度读取 `QMI8658_ReadTemperature()`
- 设备类型检测（QMI8658A / QMI8658C）

### 校准功能
- 陀螺仪零偏校准 `QMI8658_CalibrateGyro()`
- 加速度计校准 `QMI8658_CalibrateAccel()`
- 校准数据读写 `QMI8658_GetCalibration()` / `QMI8658_SetCalibration()`

### FIFO 缓冲（批量读取）
初始化并启用 FIFO：
```c
QMI8658_FifoInit();
```

批量读取 FIFO 数据：
```c
QMI8658_Data_t fifo_data[16];
uint8_t actual_count;

int ret = QMI8658_FifoRead(fifo_data, 16, &actual_count);
if (ret == 0) {
    printf("FIFO frames: %d\n", actual_count);
    for (int i = 0; i < actual_count; i++) {
        printf("Frame %d: A(%d,%d,%d) G(%d,%d,%d)\n",
            i, fifo_data[i].acc_x, fifo_data[i].acc_y, fifo_data[i].acc_z,
            fifo_data[i].gyr_x, fifo_data[i].gyr_y, fifo_data[i].gyr_z);
    }
}
```

获取 FIFO 状态：
```c
uint8_t status;
QMI8658_FifoGetStatus(&status);
// status bit7=FIFO_EMPTY, bit6=FIFO_FULL, bit5=FIFO_OVF, bit4=FIFO_WTM

uint8_t count;
QMI8658_FifoGetCount(&count);  // 获取已缓存的帧数
```

FIFO 重置：
```c
QMI8658_FifoReset();
```

### 数据转换
```c
// 原始数据转物理量
float acc_g = QMI8658_ConvertAccToG(raw_acc);
float gyr_dps = QMI8658_ConvertGyroToDPS(raw_gyro);
float temp_c = QMI8658_ConvertTempToC(raw_temp);
```

### OLED 显示
OLED 显示使用 8px 字号，数据格式为 `X.XX`：
```c
// 需要包含头文件
#include "oled_utils.h"

// 整数转字符串
char str[8];
int_to_str(12345, str, 8);

// 带比例因子的整数转字符串（用于传感器数据）
scaled_int_to_str(raw_acc, str, 7, 8192);  // 显示为 "X.XXg"
```

## 目录结构

- `apps/` - 应用层代码
  - `src/` - 源代码
    - `app/` - 应用程序
    - `board/` - 板级配置
    - `drivers/` - 驱动程序
      - `i2c/` - I2C总线驱动
      - `oled/` - OLED显示屏驱动
      - `power_en/` - 电源使能驱动
      - `qmi8658/` - QMI8658传感器驱动
    - `hal/` - 硬件抽象层
    - `mw/` - 中间层
- `cpu/` - CPU相关代码
- `doc/` - 文档
- `include_lib/` - 包含库
- `tools/` - 工具

## 编译和烧录

1. 使用Makefile编译项目
2. 使用专用工具烧录到AC6321A芯片

## 注意事项

- 确保OLED显示屏的分辨率设置正确（128×64）
- 1.3寸屏幕已设置向右偏移2个单位
- QMI8658传感器I2C地址为0x6A
- OLED显示屏I2C地址为0x3C
- 蓝牙功能需要根据实际需求进行配置
