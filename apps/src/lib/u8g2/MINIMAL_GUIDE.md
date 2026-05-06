# U8g2 最小化移植指南

## 📌 当前状态

这是一个**逐步移植**的方案，从最基础的点亮屏幕开始。

## 📁 已复制的文件（13个）

### 核心头文件（2个）
- `u8x8.h` - 底层显示接口
- `u8g2.h` - 主头文件

### u8x8 层（7个）- 底层驱动
- `u8x8_setup.c` - 初始化
- `u8x8_display.c` - 显示控制
- `u8x8_byte.c` - 字节通信
- `u8x8_gpio.c` - GPIO 控制
- `u8x8_cad.c` - 命令/数据通信
- `u8x8_d_ssd1306_128x64_noname.c` - SSD1306 驱动

### u8g2 层（4个）- 图形功能（暂未使用）
- `u8g2_setup.c`
- `u8g2_buffer.c`
- `u8g2_ll_hvline.c`
- `u8g2_d_setup.c`
- `u8g2_d_memory.c`

## 🎯 第一步测试：点亮屏幕

### 测试代码位置
`apps/src/mw/mw_runtime.c` 中的 `u8g2_test()` 函数

### 初始化方式
```c
// 声明 SSD1306 驱动回调
extern uint8_t u8x8_d_ssd1306_128x64_noname(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

// 初始化 u8x8
u8x8_Setup(
    &u8x8,
    u8x8_d_ssd1306_128x64_noname,  // 显示驱动回调
    u8g2_byte_cb,                   // I2C 字节回调
    u8g2_gpio_and_delay_cb          // GPIO 和延迟回调
);

u8x8_InitDisplay(&u8x8);
u8x8_SetPowerSave(&u8x8, 0);
```

### 测试功能
- ✅ 初始化 SSD1306 OLED
- ✅ 清屏（全黑）
- ✅ 填充屏幕（全白）
- ✅ 每2秒切换一次

### 预期效果
OLED 屏幕会在全黑和全白之间交替闪烁，每2秒切换一次。

## 🔧 平台适配层

位置：`apps/src/lib/u8g2/port/`

### 文件清单
- `u8g2_port.h` - 头文件
- `u8g2_port.c` - 实现文件

### 提供的功能
1. **I2C 通信回调** (`u8g2_byte_cb`)
   - 复用现有的 `i2c_bus_write_buf()` 函数
   - 自动处理 I2C 地址和控制字节

2. **延迟函数** (`u8g2_gpio_and_delay_cb`)
   - 毫秒延迟：使用 `os_time_dly()`
   - 微秒延迟：软件循环

## 📝 下一步计划

### 第二步：添加文本显示
需要补充的文件：
- `u8x8_8x8.c` - UTF8 支持
- `u8x8_string.c` - 字符串处理
- 至少一个字体文件（或使用内置 8x8 字体）

### 第三步：添加图形功能
需要使用 u8g2 层的 API：
- 线条、矩形、圆形绘制
- 位图显示
- 更多字体支持

## ⚠️ 注意事项

1. **编译配置**
   - 确保编译器能找到头文件路径：
     - `apps/src/lib/u8g2/csrc`
     - `apps/src/lib/u8g2/port`

2. **内存占用**
   - 当前方案 RAM 占用很小（只有几个变量）
   - Flash 占用约 100KB（所有 .c 文件）

3. **I2C 总线**
   - 必须先调用 `board_i2c_bus0_init()` 初始化 I2C
   - 使用 PA00 (SCL) 和 PA01 (SDA)

## 🚀 编译和测试

1. 编译项目
2. 烧录到开发板
3. 观察 OLED 屏幕是否黑白交替闪烁
4. 查看串口输出是否有 "U8x8 initialized" 等信息

## ❓ 常见问题

**Q: 屏幕没有反应？**
A: 检查：
- I2C 总线是否正确初始化
- OLED 电源是否正常
- I2C 地址是否正确（0x3C）

**Q: 编译错误？**
A: 检查：
- 所有必需文件是否已复制
- 头文件路径是否正确配置
- 平台适配层是否完整

---

**准备好后，我们可以继续添加文本显示功能！**
