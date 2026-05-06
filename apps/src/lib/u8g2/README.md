# U8g2 图形库移植说明

## 概述
本项目已成功将 U8g2 图形库移植到 AC6321A 平台，用于驱动 SSD1306 OLED 显示屏（128x64）。

## 目录结构
```
apps/src/lib/u8g2/
├── csrc/              # U8g2 核心源文件（从 lib/u8g2/csrc 复制）
│   ├── u8g2.h         # 主头文件
│   ├── u8x8.h         # U8x8 头文件
│   ├── u8g2_*.c       # U8g2 核心功能实现
│   └── u8x8_*.c       # U8x8 核心功能实现
└── port/              # 平台适配层
    ├── u8g2_port.h    # 平台适配头文件
    └── u8g2_port.c    # 平台适配实现（I2C 通信、延迟函数）
```

## 硬件配置
- **显示屏**: SSD1306 OLED (128x64)
- **通信接口**: I2C（软件模拟）
- **I2C 引脚**: 
  - SCL: PA00
  - SDA: PA01
- **I2C 地址**: 0x3C (7-bit)
- **复位引脚**: 无（可选）

## 已移植的核心功能
- ✅ 基础图形绘制（线条、矩形、圆形、多边形）
- ✅ 位图显示
- ✅ 字体渲染（ASCII）
- ✅ 缓冲区管理（全缓冲模式）
- ✅ SSD1306 驱动支持
- ✅ I2C 通信适配

## 使用方法

### 1. 初始化 U8g2
```c
#include "../lib/u8g2/csrc/u8g2.h"
#include "../lib/u8g2/port/u8g2_port.h"

static u8g2_t u8g2;

// 在任务或初始化函数中
u8g2_Setup_ssd1306_i2c_128x64_noname_f(
    &u8g2,
    U8G2_R0,              // 旋转方向（无旋转）
    u8g2_byte_cb,         // I2C 字节回调
    u8g2_gpio_and_delay_cb // GPIO 和延迟回调
);

u8g2_InitDisplay(&u8g2);
u8g2_SetPowerSave(&u8g2, 0);
```

### 2. 绘制内容
```c
// 清空缓冲区
u8g2_ClearBuffer(&u8g2);

// 设置字体
u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);

// 绘制文本
u8g2_DrawStr(&u8g2, 10, 20, "Hello U8g2!");

// 绘制图形
u8g2_DrawFrame(&u8g2, 5, 5, 118, 54);  // 矩形框
u8g2_DrawBox(&u8g2, 100, 45, 20, 10);  // 填充矩形

// 发送到屏幕
u8g2_SendBuffer(&u8g2);
```

### 3. 常用 API
- `u8g2_ClearBuffer()` - 清空缓冲区
- `u8g2_SendBuffer()` - 发送缓冲区到屏幕
- `u8g2_DrawStr()` - 绘制字符串
- `u8g2_DrawLine()` - 绘制直线
- `u8g2_DrawFrame()` - 绘制矩形框
- `u8g2_DrawBox()` - 绘制填充矩形
- `u8g2_DrawCircle()` - 绘制圆形
- `u8g2_DrawBitmap()` - 绘制位图
- `u8g2_SetFont()` - 设置字体

## 测试代码
测试代码位于 `apps/src/mw/mw_runtime.c` 中的 `u8g2_test()` 函数。

运行后会显示：
- 标题 "U8g2 Test"
- 递增计数器 "Count: X"
- 矩形边框
- 填充小矩形

## 编译配置
由于项目使用自动扫描源文件的构建系统，无需手动修改 Makefile。
确保以下路径在编译器的头文件搜索路径中：
- `apps/src/lib/u8g2/csrc`
- `apps/src/lib/u8g2/port`

## 内存占用
- **RAM**: 约 1KB（全缓冲模式，128x64/8 = 1024 字节）
- **Flash**: 约 10-15KB（取决于使用的功能）

## 注意事项
1. **延迟精度**: `u8g2_delay_us()` 使用粗略的软件延迟，可能需要根据实际主频调整系数
2. **I2C 总线**: 复用现有的 `i2c_bus` 驱动，确保在使用前调用 `board_i2c_bus0_init()`
3. **字体选择**: 默认使用 ASCII 字体，如需中文需要额外添加中文字体文件
4. **缓冲区模式**: 当前使用全缓冲模式（`_f` 后缀），适合小分辨率屏幕

## 扩展功能
如需添加更多功能：
1. 从 `lib/u8g2/csrc/` 复制更多源文件到 `apps/src/lib/u8g2/csrc/`
2. 在 `u8g2.h` 中启用相应的宏定义
3. 重新编译项目

## 参考资源
- U8g2 官方文档: https://github.com/olikraus/u8g2
- 支持的显示控制器列表: 查看 u8g2.h 中的 Setup 函数
