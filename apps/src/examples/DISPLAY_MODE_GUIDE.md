# OLED显示方式选择指南

## 📋 概述

每个传感器示例都支持两种OLED显示方式，可以在示例代码中独立配置，互不影响。

---

## 🎨 两种显示方式对比

### 方式1：原生OLED驱动

**宏定义**：
```c
#define USE_U8G2_DISPLAY    0
```

**特点**：
- ✅ **速度快**：直接操作硬件，刷新率高
- ✅ **资源少**：占用Flash和RAM较少
- ✅ **简单**：API直观易用
- ❌ **功能有限**：只能显示文字、基础图形
- ❌ **字体单一**：只有几种固定大小的字体

**适用场景**：
- 简单的数据展示（温度、湿度等）
- 对刷新率要求高的应用
- 资源受限的场景

**常用API**：
```c
// 初始化
OLED_Init();
OLED_ColorTurn(0);
OLED_DisplayTurn(0);
OLED_Contrast(0xFF);

// 清屏
OLED_Clear();

// 显示字符串
OLED_ShowString(x, y, "Hello", font_size, color);

// 显示数字
OLED_ShowNum(x, y, number, length, font_size, color);

// 刷新屏幕
OLED_Refresh();
```

---

### 方式2：u8g2图形库

**宏定义**：
```c
#define USE_U8G2_DISPLAY    1
```

**特点**：
- ✅ **功能强大**：丰富的图形绘制能力
- ✅ **字体丰富**：支持多种字体和大小
- ✅ **动画效果**：可以轻松实现动画
- ✅ **跨平台**：同一套代码可用于不同显示屏
- ❌ **速度慢**：需要缓冲区，刷新率较低
- ❌ **资源多**：占用更多Flash和RAM
- ⚠️ **残影问题**：每帧需清除缓冲区并完整重绘

**适用场景**：
- 复杂的UI界面（仪表盘、进度条等）
- 需要美观的字体和图形
- 动画效果需求

**常用API**：
```c
// 初始化
u8g2_Setup_ssd1306_i2c_128x64_noname_f(
    &u8g2,
    U8G2_R0,
    u8g2_byte_cb,              // 使用项目中的字节回调
    u8g2_gpio_and_delay_cb     // 使用项目中的GPIO和延时回调
);
u8g2_InitDisplay(&u8g2);
u8g2_SetPowerSave(&u8g2, 0);

// 清屏（清除缓冲区）
u8g2_ClearBuffer(&u8g2);

// 设置字体
u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);

// 绘制文本
u8g2_DrawStr(&u8g2, x, y, "Hello");

// 绘制图形
u8g2_DrawCircle(&u8g2, cx, cy, radius, option);
u8g2_DrawBox(&u8g2, x, y, w, h);
u8g2_DrawFrame(&u8g2, x, y, w, h);

// 刷新屏幕（发送缓冲区到显示屏）
u8g2_SendBuffer(&u8g2);
```

---

## 🔧 配置方法

### 在示例中配置

每个示例的 `.c` 文件开头都有显示方式配置：

```c
// ========== 显示方式选择（在示例内独立配置）==========
// 0 = 使用原生OLED驱动（直接调用OLED函数）
// 1 = 使用u8g2图形库（更丰富的图形功能）
#define USE_U8G2_DISPLAY    0
```

**修改步骤**：
1. 打开示例的 `.c` 文件（如 `qmi8658_example.c`）
2. 找到 `USE_U8G2_DISPLAY` 宏定义
3. 修改为 `0` 或 `1`
4. 重新编译烧录

### 示例对比

#### QMI8658示例
```c
// qmi8658_example.c
#define USE_U8G2_DISPLAY    0  // 默认使用原生OLED
```

#### u8g2 Dashboard示例
```c
// u8g2_dashboard.c
// 此示例专门演示u8g2功能，不使用宏切换
```

---

## 💡 代码结构

### 条件编译模式

所有显示相关的代码都使用条件编译包裹：

```c
#if USE_U8G2_DISPLAY
    // u8g2显示代码
    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawStr(&u8g2, 10, 20, "Hello");
    u8g2_SendBuffer(&u8g2);
#else
    // 原生OLED显示代码
    OLED_Clear();
    OLED_ShowString(10, 10, "Hello", 16, 1);
    OLED_Refresh();
#endif
```

### 优点

✅ **灵活性**：可以轻松切换显示方式  
✅ **可维护性**：两种方式的代码都在同一个文件中  
✅ **独立性**：每个示例可以独立选择显示方式  
✅ **兼容性**：不影响其他示例  

---

## ⚠️ u8g2残影问题及解决方案

### 问题描述

使用u8g2时，如果**只绘制新内容而不清除旧内容**，会出现残影：

**示例场景**：
```
第1帧: acc_x_str = "12.34567"  (7个字符)
显示: [1][2][.][3][4][5][6][7]

第2帧: acc_x_str = "9.12345"   (6个字符)
显示: [9][.][1][2][3][4][5][7]  ← 最后一个'7'是残留！
```

**原因**：
- u8g2使用**双缓冲机制**
- 短字符串无法覆盖长字符串的像素
- 需要每帧完整重绘或手动擦除

---

### ✅ 正确做法：清除缓冲区 + 完整重绘

```c
while (1) {
    // 读取数据
    read_sensor_data(&data);
    
    // 格式化字符串
    format_data(data, str_buffer);
    
#if USE_U8G2_DISPLAY
    // 1. 清除整个缓冲区
    u8g2_ClearBuffer(&u8g2);
    
    // 2. 重新绘制所有元素（静态+动态）
    u8g2_SetFont(&u8g2, u8g2_font_7x13_tr);
    
    // 静态元素（标签、标题等）
    u8g2_DrawStr(&u8g2, 0, 10, "Title");
    u8g2_DrawStr(&u8g2, 0, 25, "Label:");
    
    // 动态元素（数据）
    u8g2_DrawStr(&u8g2, 50, 25, str_buffer);
    
    // 3. 发送缓冲区到屏幕
    u8g2_SendBuffer(&u8g2);
#else
    // 原生OLED不需要此步骤
    OLED_ShowString(50, 16, str_buffer, 8, 1);
    OLED_Refresh();
#endif
    
    os_time_dly(10);
}
```

**关键点**：
1. ✅ **每帧调用** `u8g2_ClearBuffer()`
2. ✅ **重绘所有元素**（包括不变的静态元素）
3. ✅ **最后调用** `u8g2_SendBuffer()`

---

### ❌ 错误做法：只绘制变化部分

```c
// ❌ 错误示例 - 会出现残影
while (1) {
    read_sensor_data(&data);
    format_data(data, str_buffer);
    
#if USE_U8G2_DISPLAY
    // 没有清除缓冲区！
    u8g2_DrawStr(&u8g2, 50, 25, str_buffer);  // ← 只绘制新数据
    u8g2_SendBuffer(&u8g2);
#endif
}
```

**后果**：
- 长短不一的字符串产生残影
- 残影会累积，越来越严重
- 显示效果混乱

---

### 其他解决方案（不推荐）

#### 方案2：用空格填充固定宽度

```c
// 格式化为固定8位，不足补空格
char fixed_str[9];
snprintf(fixed_str, sizeof(fixed_str), "%-8s", raw_str);

u8g2_DrawStr(&u8g2, x, y, fixed_str);  // 总是8个字符
```

**缺点**：需要额外的字符串处理，不够灵活

#### 方案3：绘制背景色擦除

```c
// 先用黑色矩形擦除区域
u8g2_SetDrawColor(&u8g2, 0);  // 黑色（擦除）
u8g2_DrawBox(&u8g2, x, y-8, width, 10);  // 擦除该区域
u8g2_SetDrawColor(&u8g2, 1);  // 恢复白色（绘制）

// 再绘制新内容
u8g2_DrawStr(&u8g2, x, y, new_str);
```

**缺点**：代码复杂，需要精确计算每个字段的区域

---

### 💡 最佳实践总结

| 方案 | 可靠性 | 复杂度 | 性能 | 推荐度 |
|-----|-------|-------|------|--------|
| 清除缓冲区 + 完整重绘 | ⭐⭐⭐⭐⭐ | 低 | 中 | ✅ **推荐** |
| 空格填充固定宽度 | ⭐⭐⭐ | 中 | 高 | ⚠️ 可选 |
| 背景色擦除 | ⭐⭐⭐⭐ | 高 | 高 | ❌ 不推荐 |

**推荐方案1**：简单、可靠、无副作用

---

## 📊 性能对比

| 指标 | 原生OLED | u8g2 |
|-----|---------|------|
| Flash占用 | ~5KB | ~15KB |
| RAM占用 | ~1KB | ~2KB |
| 刷新率 | ~1000Hz | ~30-50Hz |
| 字体数量 | 3种 | 50+种 |
| 图形功能 | 基础 | 丰富 |
| 学习难度 | 低 | 中 |

---

## 🎯 选择建议

### 选择原生OLED驱动，如果：

- ✅ 只需要显示简单的文本数据
- ✅ 对刷新率要求高（>100Hz）
- ✅ Flash/RAM资源紧张
- ✅ 快速原型开发

### 选择u8g2图形库，如果：

- ✅ 需要复杂的UI界面
- ✅ 需要美观的字体和图形
- ✅ 需要动画效果
- ✅ 有足够的Flash/RAM资源

---

## 🚀 快速开始

### 切换到u8g2显示

1. **修改宏定义**
   ```c
   #define USE_U8G2_DISPLAY    1
   ```

2. **添加头文件**（已自动包含）
   ```c
   #if USE_U8G2_DISPLAY
       #include "../../lib/u8g2/port/u8g2_port.h"
   #endif
   ```

3. **修改初始化代码**（已自动处理）
   ```c
   #if USE_U8G2_DISPLAY
       u8g2_Setup_ssd1306_i2c_128x64_noname_f(...);
       u8g2_InitDisplay(&u8g2);
   #else
       OLED_Init();
   #endif
   ```

4. **修改显示代码**（参考示例中的实现）

5. **编译测试**

---

## 📚 学习资源

### u8g2官方文档
- GitHub: https://github.com/olikraus/u8g2
- Wiki: https://github.com/olikraus/u8g2/wiki
- 字体列表: https://github.com/olikraus/u8g2/wiki/fntlistall

### 常用字体推荐

| 字体名称 | 大小 | 用途 |
|---------|-----|------|
| `u8g2_font_7x13_tr` | 小 | 数据显示 |
| `u8g2_font_ncenB14_tr` | 中 | 标题 |
| `u8g2_font_fub25_tr` | 大 | 大数字 |
| `u8g2_font_logisoso32_tr` | 超大 | 突出显示 |

---

## ⚠️ 注意事项

1. **不要同时启用两种方式**
   - 每个示例只能选择一种显示方式
   - 通过 `USE_U8G2_DISPLAY` 宏控制

2. **u8g2需要缓冲区**
   - u8g2使用双缓冲机制
   - 所有绘图操作先写入缓冲区
   - 调用 `u8g2_SendBuffer()` 才真正显示

3. **坐标系统不同**
   - 原生OLED：`(列, 行)`，行以8像素为单位
   - u8g2：`(x, y)`，y是像素坐标

4. **字体选择影响显示效果**
   - u8g2字体很多，选择合适的字体很重要
   - 注意字体的基线位置

---

## 🔍 调试技巧

### 检查显示是否正常

```c
// 测试显示
#if USE_U8G2_DISPLAY
    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawStr(&u8g2, 10, 20, "Test OK");
    u8g2_SendBuffer(&u8g2);
#else
    OLED_Clear();
    OLED_ShowString(10, 10, "Test OK", 16, 1);
    OLED_Refresh();
#endif
```

### 检查I2C通信

```c
// 扫描I2C设备
i2c_bus_scan();

// SSD1306地址应该是 0x3C 或 0x3D
```

---

## 📅 版本历史

- v1.0 (2026-05-10): 初始版本，支持双显示方式切换

---

*如有疑问，请参考示例代码或联系项目维护者。*
