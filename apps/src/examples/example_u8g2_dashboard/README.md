# u8g2图形库仪表盘演示

## 功能说明

本示例演示u8g2图形库的高级用法，绘制美观的仪表盘界面。

### 主要功能
- ✅ 圆形仪表盘（0-100%动态显示）
- ✅ 数据卡片（CPU、温度模拟数据）
- ✅ 进度条动画（内存使用模拟）
- ✅ 反色标题栏
- ✅ 流畅的动画效果

## 硬件需求

- AC6321A开发板
- OLED显示屏（128×64，SSD1306驱动）

## 引脚连接

| OLED引脚 | 开发板引脚 | 说明 |
|---------|-----------|------|
| VCC     | 3.3V      | 电源 |
| GND     | GND       | 地   |
| SDA     | PA1       | I2C数据 |
| SCL     | PA0       | I2C时钟 |

**注意**：OLED的I2C地址为 `0x3C`

## 使用方法

### 1. 启用示例

编辑 `board/example_config.h`：

```c
#define ENABLE_EXAMPLE_QMI8658          0  // 关闭其他示例
#define ENABLE_EXAMPLE_U8G2_DASHBOARD   1  // 启用
```

### 2. 编译烧录

```bash
make clean
make
# 使用杰理下载工具烧录
```

### 3. 查看效果

OLED屏幕将显示动态仪表盘界面，数值从0到100循环变化。

## 显示说明

### 界面布局

```
┌─────────────────┐
│ DASHBOARD       │  ← 反色标题栏
├────────┬────────┤
│        │ CPU 67%│  ← 数据卡片1
│  ◐     │        │     圆形仪表盘
│ 67%    ├────────┤
│        │ TEMP 67│  ← 数据卡片2
├────────┴────────┤
│MEM [████░░]     │  ← 进度条
└─────────────────┘
```

### 显示元素

1. **顶部标题栏**（反色）
   - 高度：9像素
   - 文字："DASHBOARD"
   - 背景白色，文字黑色

2. **左侧圆形仪表**
   - 圆心位置：(28, 26)
   - 半径：13像素
   - 角度范围：210° ~ 330°（240°弧形）
   - 12个刻度线
   - 指针动态旋转
   - 百分比显示在下方

3. **右侧数据卡片**
   - CPU卡片：显示百分比（0-100%）
   - TEMP卡片：显示温度值（45-78°C模拟）
   - 圆角矩形边框
   - 标签用小字，数值用大字

4. **底部进度条**
   - 标签："MEM"
   - 圆角边框
   - 填充比例与百分比同步
   - 动态增长/缩减

## 技术细节

### u8g2配置

```c
// 使用SSD1306 128x64 I2C全缓冲区模式
u8g2_Setup_ssd1306_i2c_128x64_noname_f(
    &u8g2,
    U8G2_R0,              // 旋转0度
    u8g2_byte_cb,         // 字节回调
    u8g2_gpio_and_delay_cb // GPIO和延迟回调
);
```

### 绘图函数

#### 1. 粗线绘制
```c
drawThickLine(x1, y1, x2, y2, thickness);
```
用于绘制较粗的仪表指针

#### 2. 数据卡片
```c
drawCard(x, y, width, height, label, value);
```
- 自动调整文字基线
- 标签使用5x7字体
- 数值使用8x13B粗体字

#### 3. 圆形仪表
```c
drawGauge(centerX, centerY, radius, percent);
```
- 绘制外圆
- 绘制12个刻度
- 计算指针角度并绘制
- 显示百分比文字

#### 4. 进度条
```c
drawProgressBar(x, y, width, height, percent);
```
- 圆角边框
- 内部填充根据百分比计算
- 最小边距2像素

### 动画原理

```c
int percent = 0;
while (1) {
    percent = (percent + 1) % 101;  // 0→100→0循环
    
    // 清空缓冲区
    u8g2_ClearBuffer(&u8g2);
    
    // 绘制所有元素
    drawGauge(..., percent);
    drawCard(..., percent);
    drawProgressBar(..., percent);
    
    // 发送到显示器
    u8g2_SendBuffer(&u8g2);
    
    // 延迟50ms（20fps）
    os_time_dly(5);
}
```

### 性能优化

- **全缓冲区模式**：先在内存中绘制，再一次性发送
- **刷新率控制**：50ms延迟，约20fps
- **避免频繁清屏**：使用 `ClearBuffer` 而非 `ClearDisplay`

## 按键功能

此示例**不需要按键**，按键处理函数为空实现。

如需添加按键功能，可以：
- KEY1短按：暂停/继续动画
- KEY2短按：切换动画方向
- KEY3长按：重置计数器

## 扩展开发

### 修改布局常量

在文件开头修改布局参数：

```c
#define TITLE_H     9       // 标题栏高度
#define GAUGE_CX    28      // 仪表圆心X
#define GAUGE_CY    26      // 仪表圆心Y
#define GAUGE_R     13      // 仪表半径
// ... 更多常量
```

### 添加新元素

参考现有的绘图函数，添加新的UI元素：

```c
// 例如：添加折线图
static void drawChart(...) {
    // 绘制坐标轴
    // 绘制数据点
    // 连接线
}
```

### 接入真实数据

将模拟数据替换为传感器数据：

```c
// 替换前（模拟）
sprintf(buf, "%d%%", percent);

// 替换后（真实）
float temperature = sensor_read_temp();
sprintf(buf, "%.1f", temperature);
```

### 创建多页面

使用状态机实现页面切换：

```c
typedef enum {
    PAGE_DASHBOARD,
    PAGE_SETTINGS,
    PAGE_ABOUT
} page_t;

static page_t current_page = PAGE_DASHBOARD;

// 在按键处理中切换页面
if (key == KEY1) {
    current_page = (current_page + 1) % PAGE_COUNT;
}
```

## 注意事项

- ⚠️ u8g2全缓冲区模式需要较多RAM（128×64/8 = 1024字节）
- ⚠️ 避免在循环中频繁调用 `InitDisplay`
- ⚠️ 绘图操作必须在 `ClearBuffer` 和 `SendBuffer` 之间
- ⚠️ 复杂图形会降低刷新率

## 常见问题

### Q1: 屏幕闪烁
**A**: 
1. 确保使用全缓冲区模式（`_f` 结尾的setup函数）
2. 检查是否在每个循环都调用 `SendBuffer`
3. 降低刷新频率

### Q2: 图形显示不完整
**A**: 
1. 检查坐标是否超出屏幕范围（0-127, 0-63）
2. 确认字体大小合适
3. 查看是否有绘图顺序问题

### Q3: 内存不足
**A**: 
1. 切换到页缓冲区模式（`_1` 或 `_2` 结尾）
2. 减少同时显示的元素
3. 使用更小的字体

### Q4: 动画卡顿
**A**: 
1. 简化绘图逻辑
2. 降低刷新率（增加延迟）
3. 减少每次重绘的区域

## u8g2学习资源

- [u8g2官方Wiki](https://github.com/olikraus/u8g2/wiki)
- [u8g2字体工具](https://github.com/olikraus/u8g2/wiki/fntgrp)
- [DEVELOPMENT_GUIDE.md](../DEVELOPMENT_GUIDE.md) - 本项目开发指南

## 参考代码

- [u8g2_dashboard.c](u8g2_dashboard.c) - 完整示例代码
- [u8g2.h](../../lib/u8g2/csrc/u8g2.h) - u8g2 API头文件
- [u8g2_port.c](../../lib/u8g2/port/u8g2_port.c) - 平台移植层

---

*最后更新：2026-05-10*
