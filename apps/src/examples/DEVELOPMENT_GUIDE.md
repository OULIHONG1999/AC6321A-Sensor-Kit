# 传感器示例开发指南

## ⚠️ 重要警告

### ❌ 禁止包含 `<stdio.h>`

**在杰理SDK中，严禁在示例代码中包含 `<stdio.h>`！**

```c
// ❌ 错误：会导致编译失败
#include <stdio.h>
printf("Hello");

// ✅ 正确：直接使用printf，无需包含头文件
printf("Hello");
```

**原因**：杰理SDK使用自定义的printf实现，与标准库冲突。

---

## 📋 概述

本文档指导如何为杰理AC6321A传感器开发板创建新的示例代码。每个示例都是**完全独立**的模块，包含完整的初始化和业务逻辑。

---

## 🎯 核心设计原则

### 1. 独立性原则
- ✅ 每个示例自包含所有必要的初始化代码
- ✅ 不依赖其他示例或共享的中间层逻辑
- ✅ 可以单独编译、运行和测试

### 2. 标准化原则
- ✅ 统一的目录结构
- ✅ 固定的函数命名规范
- ✅ 一致的代码风格

### 3. 完整性原则
- ✅ 必须包含硬件初始化（电源、I2C、显示等）
- ✅ 必须实现按键事件处理（即使为空）
- ✅ 必须提供README说明文档

---

## 📁 目录结构规范

```
apps/src/
├── board/
│   ├── board_pins.h              # 引脚配置
│   └── example_config.h          # 示例选择配置
│
├── drivers/                      # 驱动层（只读，不要修改）
│   ├── i2c/
│   ├── oled/
│   ├── power_en/
│   └── [sensor_name]/            # 你的传感器驱动
│
├── examples/                     # 示例代码目录
│   ├── example_[sensor_name]/    # 你的示例
│   │   ├── [sensor_name]_example.c
│   │   ├── [sensor_name]_example.h
│   │   └── README.md
│   └── ...
│
└── mw/                           # 中间层（只负责选择示例）
    ├── mw_runtime.c
    └── mw_runtime.h
```

---

## 🚀 开发流程

### 第1步：准备传感器驱动

在 `drivers/` 目录下创建传感器驱动（如果还没有）：

```
drivers/[sensor_name]/
├── [sensor_name].c      # 驱动实现
└── [sensor_name].h      # 驱动接口
```

**驱动要求**：
- 提供初始化函数：`int [sensor_name]_init(void)`
- 提供数据读取函数：`int [sensor_name]_read(void *data)`
- 使用标准的数据结构定义

---

### 第2步：创建示例目录

在 `examples/` 下创建示例文件夹：

```bash
mkdir -p examples/example_[sensor_name]
```

**命名规范**：
- 目录名：`example_[sensor_name]`（小写，下划线分隔）
- 文件名：`[sensor_name]_example.c/h`

---

### 第3步：编写示例代码

#### 头文件：`[sensor_name]_example.h`

```c
#ifndef [SENSOR_NAME]_EXAMPLE_H
#define [SENSOR_NAME]_EXAMPLE_H

/**
 * @brief 启动示例
 * @note 此函数由 mw_runtime_init() 调用
 */
void [sensor_name]_example_start(void);

#endif
```

#### 源文件：`[sensor_name]_example.c`

**必须包含的部分**：

```c
// ========== 0. 条件编译宏（必须）==========
#include "../../board/example_config.h"

#if ENABLE_EXAMPLE_[SENSOR_NAME]

// ========== 0.5. 显示方式选择（可选）==========
// 如果示例需要OLED显示，可以选择两种方式：
// 0 = 使用原生OLED驱动（速度快，资源少）
// 1 = 使用u8g2图形库（功能丰富，支持动画）
#define USE_U8G2_DISPLAY    0

#if USE_U8G2_DISPLAY
    #include "../../lib/u8g2/port/u8g2_port.h"
    static u8g2_t u8g2;  // u8g2实例
#endif

// ========== 1. 头文件包含 ==========
#include "[sensor_name]_example.h"
#include "../../drivers/power_en/power_en.h"
#include "../../drivers/i2c/i2c_bus.h"

#if !USE_U8G2_DISPLAY
    #include "../../drivers/oled/oled.h"        // 原生OLED驱动
#endif

#include "../../drivers/[sensor_name]/[sensor_name].h"
#include "os/os_api.h"
#include "system/event.h"

// ========== 1. 按键事件处理（必须实现）==========

/**
 * @brief 按键事件处理函数（由 app_spp_and_le.c 调用）
 * @param key_value 按键值（0=KEY1, 1=KEY2, 2=KEY3, 3=KEY4）
 * @param event_type 事件类型（KEY_EVENT_CLICK短按, KEY_EVENT_LONG长按）
 * 
 * @note 此函数必须是全局函数（不能是static），因为会被外部调用
 * @note 如果示例不需要按键功能，可以实现为空函数
 */
void example_key_handler(u8 key_value, u8 event_type) {
    // 示例：KEY3长按校准传感器
    if (event_type == KEY_EVENT_LONG && key_value == 2) {
        // 执行校准逻辑
    }
    
    // 示例：KEY1短按切换显示模式
    if (event_type == KEY_EVENT_CLICK && key_value == 0) {
        // 切换显示模式
    }
}

// 注意：不需要注册 SYS_EVENT_HANDLER！
// SDK的按键事件会在 app_spp_and_le.c 中统一处理并调用此函数


// ========== 2. 示例主任务（必须实现）==========

/**
 * @brief 示例主任务
 * @param p_arg 任务参数（通常为NULL）
 * 
 * @note 此任务包含完整的初始化流程和主循环
 */
static void [sensor_name]_example_task(void *p_arg) {
    
    // ---- 2.1 硬件初始化（必须）----
    
    // 电源使能
    power_en_enable(1);
    os_time_dly(10);  // 等待电源稳定
    
    // I2C总线初始化（如果传感器使用I2C）
    board_i2c_bus0_init();
    i2c_bus_scan();  // 可选：扫描I2C设备
    
    // 显示屏初始化（OLED或u8g2二选一）
    
    // 方式A：使用原生OLED驱动
    OLED_Init();
    OLED_ColorTurn(0);
    OLED_DisplayTurn(0);
    OLED_Contrast(0xFF);
    
    // 方式B：使用u8g2图形库
    // u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, 
    //     u8g2_byte_cb, u8g2_gpio_and_delay_cb);
    // u8g2_InitDisplay(&u8g2);
    // u8g2_SetPowerSave(&u8g2, 0);
    
    
    // ---- 2.2 显示欢迎界面（推荐）----
    
    OLED_Clear();
    OLED_ShowString(16, 16, "[Sensor Name]", 16, 1);
    OLED_ShowString(16, 32, "Demo", 16, 1);
    OLED_Refresh();
    os_time_dly(200);  // 显示2秒
    
    
    // ---- 2.3 传感器初始化（必须）----
    
    if ([sensor_name]_init() < 0) {
        // 初始化失败处理
        OLED_Clear();
        OLED_ShowString(8, 24, "Init Failed", 16, 1);
        OLED_Refresh();
        while (1) os_time_dly(100);  // 死循环等待
    }
    
    
    // ---- 2.4 主循环（必须）----
    
    [sensor_data_t] data;  // 传感器数据结构
    
    while (1) {
        // 读取传感器数据
        if ([sensor_name]_read(&data) == 0) {
            // 处理数据
            // ...
            
            // 更新显示
            OLED_Clear();
            // 显示数据...
            OLED_Refresh();
        }
        
        // 控制刷新频率
        os_time_dly(10);  // 10ms = 100Hz
    }
}


// ========== 3. 启动函数（必须实现）==========

/**
 * @brief 启动示例
 * @note 此函数由 mw_runtime_init() 调用
 */
void [sensor_name]_example_start(void) {
    // 创建OS任务
    // 参数：任务函数, 参数, 优先级, 栈大小, CPU, 任务名
    os_task_create([sensor_name]_example_task, NULL, 5, 1024, 0, "[sensor]_ex");
}

#endif /* ENABLE_EXAMPLE_[SENSOR_NAME] */
```

---

### 第4步：编写README文档

创建 `examples/example_[sensor_name]/README.md`：

```markdown
# [传感器名称] 示例

## 功能说明
简要描述此示例的功能和用途。

## 硬件需求
- AC6321A开发板
- [传感器名称]模块
- 连接方式：[I2C/SPI/UART/ADC]

## 引脚连接
| 传感器引脚 | 开发板引脚 | 说明 |
|-----------|-----------|------|
| VCC       | 3.3V      | 电源 |
| GND       | GND       | 地   |
| SDA       | PA1       | I2C数据 |
| SCL       | PA0       | I2C时钟 |

## 使用方法

### 1. 启用示例
编辑 `board/example_config.h`：
```c
#define ENABLE_EXAMPLE_[SENSOR_NAME]  1
#define ENABLE_EXAMPLE_QMI8658        0  // 关闭其他示例
```

### 2. 编译烧录
```bash
make clean
make
# 使用下载工具烧录
```

### 3. 操作说明
- **KEY1短按**：[功能说明]
- **KEY2短按**：[功能说明]
- **KEY3长按**：[功能说明]
- **KEY4短按**：[功能说明]

## 显示说明
描述OLED显示的内容和布局。

## 注意事项
- [注意事项1]
- [注意事项2]

## 扩展开发
如需自定义功能，可以修改：
- `example_key_handler()` - 按键逻辑
- `[sensor_name]_example_task()` - 主循环逻辑
```

---

### 第5步：注册示例到配置系统

#### 5.1 添加配置宏

编辑 `board/example_config.h`：

```c
/* ========== 示例选择（只能开启一个）========== */

// QMI8658 IMU 传感器示例
#define ENABLE_EXAMPLE_QMI8658          0

// u8g2 图形库仪表盘演示
#define ENABLE_EXAMPLE_U8G2_DASHBOARD   0

// [你的传感器] 示例
#define ENABLE_EXAMPLE_[SENSOR_NAME]    1  // ← 启用你的示例

// ... 后续添加更多示例
```

#### 5.2 添加到检查宏

```c
/* ========== 配置检查 ========== */
#define ENABLED_EXAMPLE_COUNT ( \
    ENABLE_EXAMPLE_QMI8658 + \
    ENABLE_EXAMPLE_U8G2_DASHBOARD + \
    ENABLE_EXAMPLE_[SENSOR_NAME] + \  // ← 添加你的示例
    /* 后续添加更多... */ \
)
```

#### 5.3 注册到 mw_runtime.c

编辑 `mw/mw_runtime.c`：

```c
#include "mw_runtime.h"
#include "../board/example_config.h"

#if ENABLE_EXAMPLE_QMI8658
    #include "../examples/example_qmi8658/qmi8658_example.h"
#elif ENABLE_EXAMPLE_U8G2_DASHBOARD
    #include "../examples/example_u8g2_dashboard/u8g2_dashboard.h"
#elif ENABLE_EXAMPLE_[SENSOR_NAME]  // ← 添加你的示例
    #include "../examples/example_[sensor_name]/[sensor_name]_example.h"
#endif

void mw_runtime_init(void) {
#if ENABLE_EXAMPLE_QMI8658
    qmi8658_example_start();
    
#elif ENABLE_EXAMPLE_U8G2_DASHBOARD
    u8g2_dashboard_start();
    
#elif ENABLE_EXAMPLE_[SENSOR_NAME]  // ← 添加你的示例
    [sensor_name]_example_start();
#endif
}
```

---

## ✅ 检查清单

在提交示例之前，请确认：

### 代码完整性
- [ ] 实现了 `[sensor_name]_example_start()` 函数
- [ ] 实现了 `[sensor_name]_example_task()` 任务
- [ ] 实现了 `example_key_handler()` 按键处理（**必须是全局函数，不能是static**）
- [ ] 包含了所有必要的头文件
- [ ] **添加了条件编译宏**（`#if ENABLE_EXAMPLE_[SENSOR_NAME]` ... `#endif`）
- [ ] **没有注册 SYS_EVENT_HANDLER**（SDK会统一调用）

### 初始化完整性
- [ ] 调用了 `power_en_enable(1)`
- [ ] 初始化了I2C总线（如果使用I2C设备）
- [ ] 初始化了显示屏（OLED或u8g2）
- [ ] 初始化了传感器
- [ ] 处理了初始化失败的情况

### 代码规范
- [ ] 使用了统一的命名规范
- [ ] 添加了必要的注释
- [ ] 遵循了代码风格
- [ ] 没有硬编码的魔法数字

### 文档完整性
- [ ] 创建了 `README.md`
- [ ] 说明了硬件连接
- [ ] 说明了使用方法
- [ ] 说明了按键功能

### 配置正确性
- [ ] 在 `example_config.h` 中添加了配置宏
- [ ] 在 `ENABLED_EXAMPLE_COUNT` 中添加了计数
- [ ] 在 `mw_runtime.c` 中注册了示例

---

## 💡 最佳实践

### 0. 条件编译宏（重要）

**为什么需要条件编译？**

所有示例的源文件都会被编译器扫描，如果同时编译多个示例，会导致：
- ❌ 函数重定义错误（如 `example_key_handler`）
- ❌ 符号冲突
- ❌ 编译失败

**解决方案**：

在每个示例的 `.c` 文件开头和结尾添加条件编译：

```c
#include "../../board/example_config.h"

#if ENABLE_EXAMPLE_QMI8658  // 只在此示例启用时编译

// ... 所有代码 ...

#endif /* ENABLE_EXAMPLE_QMI8658 */
```

**工作原理**：
- `example_config.h` 中只有一个示例的宏为1
- 其他示例的宏为0，其代码不会被编译
- 避免重定义问题

---

### 0.5. 按键事件处理机制（重要）

**按键事件如何工作？**

SDK的按键系统工作流程：

```
用户按下按键
    ↓
SDK检测到按键事件
    ↓
SDK调用 app_spp_and_le.c 中的 spple_key_event_handler()
    ↓
spple_key_event_handler() 提取 key_value 和 event_type
    ↓
调用 example_key_handler(key_value, event_type)
    ↓
你的示例代码处理按键逻辑
```

**在 `app_spp_and_le.c` 中的实现**：

```c
static void spple_key_event_handler(struct sys_event *event) {
    u8 event_type = 0;
    u8 key_value = 0;

    if (event->arg == (void *)DEVICE_EVENT_FROM_KEY) {
        event_type = event->u.key.event;
        key_value = event->u.key.value;
        
        // 调用示例的按键处理函数
        extern void example_key_handler(u8 key_value, u8 event_type);
        example_key_handler(key_value, event_type);
    }
}
```

**你需要做的**：

1. ✅ 实现 `example_key_handler()` 函数（**必须是全局函数**）
2. ❌ **不要**注册 `SYS_EVENT_HANDLER`
3. ❌ **不要**使用 `static` 修饰 `example_key_handler`

**为什么这样设计？**

- ✅ **简化示例代码**：不需要理解SDK的事件系统
- ✅ **统一管理**：所有示例的按键都在同一个地方处理
- ✅ **避免冲突**：不会有多个事件处理器冲突
- ✅ **易于调试**：可以在 `app_spp_and_le.c` 中统一打印日志

---

### 0.5. OLED显示方式选择（重要）

**每个示例可以独立选择OLED显示方式**：

#### 方式1：原生OLED驱动（默认）

```c
#define USE_U8G2_DISPLAY    0
```

**优点**：
- ✅ 速度快，刷新率高（~1000Hz）
- ✅ 资源占用少（Flash ~5KB, RAM ~1KB）
- ✅ API简单直观

**缺点**：
- ❌ 功能相对简单
- ❌ 字体选择有限

**适用场景**：简单的数据展示、对刷新率要求高的应用

#### 方式2：u8g2图形库

```c
#define USE_U8G2_DISPLAY    1
```

**优点**：
- ✅ 功能强大，支持复杂图形
- ✅ 丰富的字体选择（50+种）
- ✅ 支持动画效果

**缺点**：
- ❌ 速度较慢（~30-50Hz）
- ❌ 资源占用多（Flash ~15KB, RAM ~2KB）

**适用场景**：复杂UI界面、需要美观的图形和动画

#### 配置方法

在示例文件开头设置宏：

```c
// ========== 显示方式选择（在示例内独立配置）==========
#define USE_U8G2_DISPLAY    0  // 改为1启用u8g2

#if USE_U8G2_DISPLAY
    #include "../../lib/u8g2/port/u8g2_port.h"
    static u8g2_t u8g2;
#endif
```

**注意**：
- ✅ 此宏只在当前示例中生效
- ✅ 不同示例可以使用不同的显示方式
- ✅ 所有显示代码都需要用条件编译包裹
- ⚠️ **u8g2残影问题**：使用u8g2时，每帧必须清除缓冲区并完整重绘，否则会出现残影

**详细对比**：参考 `DISPLAY_MODE_GUIDE.md`

---

### 1. 错误处理
```c
// ✅ 好的做法：检查返回值并处理错误
if (sensor_init() < 0) {
    OLED_ShowString(0, 0, "Init Error", 16, 1);
    OLED_Refresh();
    while (1) os_time_dly(100);
}

// ❌ 不好的做法：忽略错误
sensor_init();  // 可能失败但没处理
```

### 2. 刷新频率控制
```c
// ✅ 好的做法：控制刷新频率
while (1) {
    sensor_read(&data);
    update_display(&data);
    os_time_dly(10);  // 100Hz刷新
}

// ❌ 不好的做法：无延迟，浪费CPU
while (1) {
    sensor_read(&data);
    update_display(&data);
    // 没有延迟，占用过多CPU资源
}
```

### 3. 按键响应
```c
// ✅ 好的做法：区分短按和长按
if (event_type == KEY_EVENT_CLICK) {
    // 短按逻辑
} else if (event_type == KEY_EVENT_LONG) {
    // 长按逻辑
}

// ❌ 不好的做法：不区分事件类型
// 所有按键都执行相同逻辑
```

### 4. 显示优化
```c
// ✅ 好的做法：局部刷新（如果支持）
OLED_ShowString(x, y, str, size, mode);
OLED_Refresh();

// ✅ 好的做法：清屏后重绘
OLED_Clear();
// 绘制所有内容...
OLED_Refresh();

// ❌ 不好的做法：频繁全刷
OLED_ShowString(...);
OLED_Refresh();  // 每次显示都刷新，闪烁且慢
OLED_ShowString(...);
OLED_Refresh();
```

---

## 🔧 调试技巧

### 1. 串口打印
```c
printf("Sensor value: %d\n", data.value);
```

### 2. I2C设备检测
```c
i2c_bus_scan();  // 扫描并打印所有I2C设备地址
```

### 3. 分步验证
```c
// 第1步：验证电源
power_en_enable(1);
printf("Power enabled\n");

// 第2步：验证I2C
board_i2c_bus0_init();
i2c_bus_scan();

// 第3步：验证OLED
OLED_Init();
OLED_ShowString(0, 0, "Test", 16, 1);
OLED_Refresh();

// 第4步：验证传感器
if (sensor_init() == 0) {
    printf("Sensor OK\n");
}
```

---

## 📚 参考示例

### 简单示例：QMI8658 IMU
- 位置：`examples/example_qmi8658/`
- 特点：使用原生OLED驱动，实时显示传感器数据
- 适合学习：基础的传感器读取和显示

### 图形示例：u8g2 Dashboard
- 位置：`examples/example_u8g2_dashboard/`
- 特点：使用u8g2图形库，绘制仪表盘和进度条
- 适合学习：高级图形绘制

---

## ❓ 常见问题

### Q1: 为什么每个示例都要重复初始化代码？
**A**: 为了保证示例的独立性和可移植性。用户可以复制任何一个示例作为模板，无需关心外部依赖。

### Q2: 可以不实现按键处理吗？
**A**: 必须实现 `example_key_handler()` 函数，但如果不需要按键功能，可以实现为空函数。

### Q3: 可以同时运行多个示例吗？
**A**: 不可以。`example_config.h` 中的配置检查会确保只启用一个示例。这是为了避免资源冲突。

### Q4: 如何切换不同的示例？
**A**: 修改 `board/example_config.h` 中的宏定义，将需要的示例设为1，其他设为0，然后重新编译。

### Q5: 可以使用SPI或UART传感器吗？
**A**: 可以。只需在示例中初始化对应的接口（SPI或UART），而不是I2C。参考现有的SPI/UART驱动。

---

## 🎓 学习路径

### 初级：理解基础
1. 阅读 `example_qmi8658` 的代码
2. 理解初始化的顺序
3. 理解按键事件的注册机制

### 中级：修改现有示例
1. 修改QMI8658示例的显示布局
2. 添加新的按键功能
3. 调整刷新频率

### 高级：创建新示例
1. 为新传感器编写驱动
2. 创建完整的示例代码
3. 编写详细的文档

---

## 📝 版本历史

- v1.0 (2026-05-10): 初始版本，建立基本框架

---

*如有疑问，请参考现有示例代码或联系项目维护者。*
