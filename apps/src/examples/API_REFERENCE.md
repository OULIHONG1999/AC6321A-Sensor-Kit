# 传感器示例 API 参考

## ⚠️ 重要约束

### ❌ 禁止使用的头文件

**`<stdio.h>`** - **严禁在示例代码中包含！**

**原因**：
- 杰理SDK使用自定义的printf实现
- 包含`<stdio.h>`会导致编译失败或链接错误
- SDK已提供`printf()`函数，无需额外包含

**正确做法**：
```c
// ✅ 正确：直接使用printf，无需包含任何头文件
printf("Hello, World!\n");

// ❌ 错误：不要包含stdio.h
#include <stdio.h>  // 会导致编译失败！
```

**其他标准库**：
- ✅ `<math.h>` - 可以使用（需要数学运算时）
- ✅ `<string.h>` - 谨慎使用（优先使用SDK提供的字符串函数）
- ❌ `<stdio.h>` - **禁止使用**
- ❌ `<stdlib.h>` - **禁止使用**

---

## 📖 概述

本文档定义传感器示例开发的**公共接口规范**。所有传感器示例必须遵循这些接口，确保模块化和标准化。

---

## 🔧 配置宏

### 1. 示例启用宏

**位置**：`apps/src/board/example_config.h`

```c
// 同一时间只能启用一个示例
#define ENABLE_EXAMPLE_QMI8658        1
#define ENABLE_EXAMPLE_U8G2_DASHBOARD 0
#define ENABLE_EXAMPLE_BME280         0
```

### 2. 显示方式宏

**位置**：每个示例的 `.c` 文件顶部

```c
// 0 = 原生OLED驱动
// 1 = u8g2图形库
#define USE_U8G2_DISPLAY    1
```

**注意**：
- ✅ 每个示例独立配置，互不影响
- ✅ QMI8658等传感器示例支持双显示方式切换
- ⚠️ u8g2_dashboard等纯u8g2演示示例不需要此宏（始终使用u8g2）

---

## 📋 标准接口

### 1. 启动函数（必需）

```c
/**
 * @brief 启动传感器示例
 * @note 由 mw_runtime_init() 调用
 */
void <sensor>_example_start(void);
```

**命名规则**：`<sensor_name>_example_start`  
**示例**：
- `qmi8658_example_start()`
- `bme280_example_start()`
- `mpu6050_example_start()`

---

### 2. 按键处理函数（必需）

```c
/**
 * @brief 按键事件处理函数
 * @param key_value  按键值 (0=KEY1, 1=KEY2, 2=KEY3, 3=KEY4)
 * @param event_type 事件类型 (KEY_EVENT_CLICK短按, KEY_EVENT_LONG长按)
 * @note 必须是全局函数，由 app_spp_and_le.c 直接调用
 */
void example_key_handler(u8 key_value, u8 event_type);
```

**注意**：
- ✅ 函数名固定为 `example_key_handler`
- ✅ 必须是全局函数（非static）
- ❌ 不要使用 SYS_EVENT_HANDLER 注册
- 📝 在 `app_spp_and_le.c` 中通过 `extern` 声明调用：
  ```c
  extern void example_key_handler(u8 key_value, u8 event_type);
  example_key_handler(key_value, event_type);
  ```

---

## 🎨 显示接口

### 原生 OLED 驱动

**头文件**：
```c
#include "../../drivers/oled/oled.h"
#include "../../drivers/oled/oled_utils.h"
```

**核心函数**：
```c
// 初始化
void OLED_Init(void);
void OLED_ColorTurn(u8 mode);      // 0=正常, 1=反色
void OLED_DisplayTurn(u8 mode);    // 0=正常, 1=翻转
void OLED_Contrast(u8 level);      // 对比度 0x00-0xFF

// 显示
void OLED_ShowString(u8 x, u8 y, u8 *str, u8 size, u8 mode);
// x: 列坐标 (0-127)
// y: 行坐标 (0-7, 每行8像素)
// size: 字体大小 (8, 12, 16)
// mode: 0=正常, 1=反色

// 刷新
void OLED_Clear(void);
void OLED_Refresh(void);
```

---

### u8g2 图形库

**头文件**：
```c
#include "../../lib/u8g2/port/u8g2_port.h"
```

**类型定义**：
```c
u8g2_t u8g2;  // u8g2实例
```

**初始化函数**：
```c
// SSD1306 I2C 初始化
u8g2_Setup_ssd1306_i2c_128x64_noname_f(
    &u8g2,
    U8G2_R0,                    // 旋转方向
    u8g2_byte_cb,               // 字节回调（项目定制）
    u8g2_gpio_and_delay_cb      // GPIO和延时回调（项目定制）
);
u8g2_InitDisplay(&u8g2);
u8g2_SetPowerSave(&u8g2, 0);
```

**核心函数**：
```c
// 字体设置
void u8g2_SetFont(u8g2_t *u8g2, const void *font);

// 文本绘制
u8g2_uint_t u8g2_DrawStr(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, const char *s);
// x: 像素坐标 (0-127)
// y: 基线坐标（不是顶部！）

// 图形绘制
void u8g2_DrawLine(u8g2_t *u8g2, u8g2_uint_t x1, u8g2_uint_t y1, u8g2_uint_t x2, u8g2_uint_t y2);
void u8g2_DrawBox(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t h);
void u8g2_DrawFrame(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t h);
void u8g2_DrawCircle(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t radius, u8g2_uint_t opt);
void u8g2_DrawHLine(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t width);
void u8g2_DrawVLine(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t height);

// 缓冲区操作（⚠️ 重要）
void u8g2_ClearBuffer(u8g2_t *u8g2);   // 清除缓冲区
void u8g2_SendBuffer(u8g2_t *u8g2);    // 发送缓冲区到屏幕
```

**⚠️ 残影避免规则**：
```c
// 每帧必须执行以下顺序：
u8g2_ClearBuffer(&u8g2);   // 1. 清除
// 2. 绘制所有内容
u8g2_SendBuffer(&u8g2);    // 3. 发送
```

---

## 📐 屏幕布局规范

### 128x64 OLED 屏幕

**分辨率**：128 × 64 像素

**u8g2 坐标系统**：
- Y坐标是**基线位置**（不是顶部）
- 基线 = 顶部边距 + 字体高度

**常用字体尺寸**：

| 字体 | 高度 | 推荐Y坐标 |
|------|------|----------|
| `u8g2_font_ncenB10_tr` | 10px | 12, 30, 50 |
| `u8g2_font_7x13_tr` | 13px | 14, 28, 42, 56 |
| `u8g2_font_6x10_tr` | 10px | 12, 24, 36, 48, 60 |
| `u8g2_font_5x7_tr` | 7px | 8, 16, 24, 32, 40, 48, 56, 63 |

**行间距公式**：
```
下一行Y = 当前行Y + (字体高度 + 1~2)
```

---

## 🔌 硬件接口

### 电源管理

```c
#include "../../drivers/power_en/power_en.h"

void power_en_enable(u8 state);  // 1=开启, 0=关闭
```

### I2C 总线

```c
#include "../../drivers/i2c/i2c_bus.h"

void board_i2c_bus0_init(void);  // 初始化I2C总线0
void i2c_bus_scan(void);         // 扫描I2C设备（调试用）
```

---

## ⏱️ RTOS 接口

```c
#include "os/os_api.h"

// 创建任务
void os_task_create(
    void (*task)(void *),  // 任务函数
    void *arg,             // 任务参数
    u8 prio,               // 优先级 (1-31)
    u16 stack_size,        // 栈大小 (字节)
    u8 cpu,                // CPU编号 (通常0)
    const char *name       // 任务名称 (最多8字符)
);

// 示例：创建传感器任务
os_task_create(sensor_task, NULL, 5, 1024, 0, "sensor_ex");

// 延时
void os_time_dly(u32 tick);  // 延时tick数
```

---

## 📝 工具函数

### 数据格式化

```c
#include "../../drivers/oled/oled_utils.h"

/**
 * @brief 将缩放整数转换为字符串
 * @param value     缩放整数值
 * @param str       输出字符串缓冲区
 * @param max_len   最大长度
 * @param scale     缩放因子
 */
void scaled_int_to_str(int32_t value, char *str, u8 max_len, int32_t scale);
```

**示例**：
```c
char str[8];
scaled_int_to_str(data.acc_x, str, 7, 8192);
// 如果 data.acc_x = 4096, 则 str = "+0.500"
```

---

## 📂 文件结构

### 标准示例目录

```
apps/src/examples/
├── example_<sensor>/
│   ├── <sensor>_example.c      # 主实现文件
│   ├── <sensor>_example.h      # 头文件
│   └── README.md               # 使用说明
├── example_config.h            # 配置宏定义
├── DEVELOPMENT_GUIDE.md        # 开发指南
├── DISPLAY_MODE_GUIDE.md       # 显示模式说明
└── KEY_EVENT_SYSTEM.md         # 按键系统说明
```

### 必需文件

每个示例必须包含：
- ✅ `<sensor>_example.c` - 主实现
- ✅ `<sensor>_example.h` - 头文件（可选，如无外部依赖可省略）
- ✅ `README.md` - 使用说明

---

## ✅ 检查清单

创建新示例时，确保：

- [ ] 在 `example_config.h` 中添加 `ENABLE_EXAMPLE_XXX` 宏
- [ ] 实现 `<sensor>_example_start()` 函数
- [ ] 实现 `example_key_handler()` 函数（全局）
- [ ] 添加条件编译保护 `#if ENABLE_EXAMPLE_XXX ... #endif`
- [ ] 支持双显示方式（USE_U8G2_DISPLAY 宏）
- [ ] u8g2 模式下每帧清除缓冲区
- [ ] 编写 README.md 文档
- [ ] 测试编译无错误
- [ ] 测试功能正常

---

## 📚 相关文档

- [开发指南](DEVELOPMENT_GUIDE.md) - 完整的开发流程
- [显示模式指南](DISPLAY_MODE_GUIDE.md) - 双显示方案详解
- [按键系统说明](KEY_EVENT_SYSTEM.md) - 按键事件处理机制

---

*最后更新：2026-05-10*
