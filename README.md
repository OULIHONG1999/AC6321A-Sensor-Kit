# 杰理 AC6321A 传感器开发板 SDK

## 📋 项目概述

本项目是基于杰理AC6321A蓝牙SoC芯片的传感器开发平台，提供**模块化、可扩展**的传感器驱动和示例代码。

### 核心特点

- ✅ **模块化设计**：每个传感器驱动和示例完全独立
- ✅ **快速上手**：提供完整的示例代码，开箱即用
- ✅ **易于扩展**：标准化的开发流程，轻松添加新传感器
- ✅ **低耦合架构**：与SDK解耦，便于维护和移植

### 硬件平台

所有开发板共享以下基础硬件：
- **主控芯片**：AC6321A 蓝牙SoC
- **显示模块**：OLED显示屏（128×64，SSD1306驱动，I2C地址0x3C）
- **交互模块**：4个按键
- **通信接口**：I2C、SPI、UART、ADC、GPIO

不同开发板搭配不同的传感器模块，形成多样化的产品线。

## 🎯 支持的传感器

### 已实现的示例

#### 1. QMI8658 六轴IMU传感器
- **位置**：`apps/src/examples/example_qmi8658/`
- **功能**：加速度计 + 陀螺仪数据采集和显示
- **特性**：
  - 支持QMI8658A和QMI8658C型号自动检测
  - 陀螺仪和加速度计校准功能
  - FIFO批量数据读取
  - 实时数据显示（加速度、角速度、温度）
- **连接方式**：I2C（地址0x6A）

#### 2. u8g2 图形库演示
- **位置**：`apps/src/examples/example_u8g2_dashboard/`
- **功能**：u8g2图形库功能演示
- **特性**：
  - 圆形仪表盘绘制
  - 数据卡片显示
  - 进度条动画
  - 反色标题栏
- **用途**：学习u8g2图形库的高级用法

### 计划支持的传感器

- **温湿度**：BME280、SHT30/SHT31、DHT11/DHT22
- **测距**：VL53L0X（激光TOF）、HC-SR04（超声波）
- **气体**：MQ系列、CCS811、MH-Z19B
- **光照**：BH1750、GUVA-S12SD
- **运动**：MPU6050、ADXL345、QMC5883L
- **人体感应**：LD2410（毫米波）、HC-SR501（红外）
- **其他**：土壤湿度、震动/倾斜等

详见项目根目录的 `readme.md` 获取完整传感器列表。

## 🚀 快速开始

### 选择并运行示例

1. **编辑配置文件** `apps/src/board/example_config.h`：
```c
// 启用QMI8658示例
#define ENABLE_EXAMPLE_QMI8658          1
#define ENABLE_EXAMPLE_U8G2_DASHBOARD   0
```

2. **编译项目**：
```bash
make clean
make
```

3. **烧录固件**：
使用杰理下载工具烧录到AC6321A芯片

4. **查看效果**：
OLED屏幕将显示传感器数据或演示界面

### 切换示例

只需修改 `example_config.h` 中的宏定义，重新编译即可切换到不同的示例。

---

## 📖 开发指南

### 创建新示例

详细的新示例开发流程请参考：
👉 [apps/src/examples/DEVELOPMENT_GUIDE.md](apps/src/examples/DEVELOPMENT_GUIDE.md)

**简要步骤**：
1. 在 `drivers/` 目录下编写传感器驱动
2. 在 `examples/` 目录下创建示例文件夹
3. 实现标准的示例代码结构（包含初始化、按键处理、主循环）
4. 编写README文档
5. 在 `example_config.h` 中注册新示例
6. 在 `mw_runtime.c` 中添加启动逻辑

### 代码架构

```
apps/src/
├── board/                    # 板级配置
│   ├── board_pins.h         # 引脚定义
│   └── example_config.h     # 示例选择配置 ⭐
│
├── drivers/                  # 驱动层（各传感器独立）
│   ├── i2c/                 # I2C总线驱动
│   ├── oled/                # OLED显示驱动
│   ├── power_en/            # 电源使能驱动
│   ├── qmi8658/             # QMI8658驱动
│   └── [sensor_name]/       # 其他传感器驱动
│
├── examples/                 # 示例代码层 ⭐
│   ├── example_qmi8658/     # QMI8658示例
│   ├── example_u8g2_dashboard/  # u8g2演示
│   ├── example_template/    # 示例模板
│   └── DEVELOPMENT_GUIDE.md # 开发指南 ⭐
│
├── mw/                       # 中间层（极简）
│   ├── mw_runtime.c         # 只负责选择示例
│   └── mw_runtime.h
│
├── hal/                      # 硬件抽象层
└── app/                      # 应用入口
```

#### 数据转换
```c
// 原始数据转物理量
float acc_g = QMI8658_ConvertAccToG(raw_acc);
float gyr_dps = QMI8658_ConvertGyroToDPS(raw_gyro);
float temp_c = QMI8658_ConvertTempToC(raw_temp);
```

#---

## 🔧 编译和烧录

### 编译环境

- **IDE**：Jieli IDE 或 VSCode + clangd
- **编译器**：ARM GCC
- **操作系统**：Windows / Linux / macOS

### 编译步骤

```bash
# 清理
make clean

# 编译
make

# 查看详细编译信息
make V=1
```

### 烧录方法

1. **JTAG烧录**（推荐）：
   - 连接JTAG调试器
   - 使用杰理下载工具烧录
   - 支持断点调试

2. **串口烧录**：
   - 连接USB转串口模块
   - 使用串口下载工具
   - 适合量产

详见 `cpu/bd19/tools/` 目录下的下载工具和脚本。

---

## ⚙️ 配置说明

### 引脚配置

编辑 `apps/src/board/board_pins.h`：

```c
// OLED I2C引脚
#define BOARD_I2C_SCL IO_PORTA_00
#define BOARD_I2C_SDA IO_PORTA_01

// QMI8658 I2C地址
#define BOARD_IMU_I2C_ADDR7 0x6A

// 电源使能引脚
#define BOARD_POWER_EN IO_PORTB_04
```

### 示例选择

编辑 `apps/src/board/example_config.h`：

```c
// 只能启用一个示例
#define ENABLE_EXAMPLE_QMI8658          1
#define ENABLE_EXAMPLE_U8G2_DASHBOARD   0
```

编译时会自动检查配置的正确性，如果启用多个或未启用任何示例，会报错提示。

---

## 💡 注意事项

### 硬件相关

- ✅ OLED显示屏支持1.3寸和0.96寸（128×64分辨率）
- ✅ 1.3寸屏幕已设置向右偏移2个单位（在oled_config.h中配置）
- ✅ QMI8658传感器I2C地址为0x6A（SA0引脚接地）
- ✅ OLED显示屏I2C地址为0x3C
- ⚠️ 确保传感器供电电压匹配（3.3V或5V）
- ⚠️ I2C总线需要上拉电阻（通常模块已集成）

### 软件相关

- ✅ 每个示例都是完全独立的，包含所有必要的初始化
- ✅ 必须实现按键事件处理函数（可以为空）
- ✅ 建议使用 `os_time_dly()` 控制刷新频率，避免占用过多CPU
- ⚠️ 不要同时启用多个示例（会导致资源冲突）
- ⚠️ 修改示例后需要重新编译和烧录

### 开发建议

- 📝 为新传感器编写驱动时，参考现有的驱动代码风格
- 📝 创建示例时，严格遵循 `DEVELOPMENT_GUIDE.md` 的规范
- 📝 添加详细的注释和README文档
- 📝 测试各种边界情况（初始化失败、数据异常等）
- 📝 使用串口打印进行调试（`printf()`）

---

## 🐛 常见问题

### Q1: 编译报错 "请至少启用一个示例"
**A**: 检查 `board/example_config.h`，确保至少有一个示例的宏定义为1。

### Q2: OLED屏幕不显示
**A**: 
1. 检查I2C引脚连接是否正确
2. 确认OLED模块供电正常
3. 运行 `i2c_bus_scan()` 检查是否能检测到OLED（地址0x3C）

### Q3: 传感器初始化失败
**A**:
1. 检查传感器供电和接线
2. 确认I2C地址是否正确
3. 查看串口打印的错误信息
4. 使用示波器或逻辑分析仪检查I2C波形

### Q4: 如何添加新的传感器？
**A**: 参考 `apps/src/examples/DEVELOPMENT_GUIDE.md` 中的完整流程。

### Q5: 按键没有响应
**A**:
1. 确认是否正确注册了按键事件处理器（`SYS_EVENT_HANDLER`）
2. 检查按键值是否正确（不同开发板可能不同）
3. 确认按键事件类型（短按/长按）是否匹配

---

## 📦 项目结构总览

```
fw-AC63_BT_SDK/           # SDK根目录
├── apps/
│   ├── spp_and_le/      # 官方蓝牙示例（参考用）
│   └── src/             # ⭐ 我们的传感器项目
│       ├── app/         # 应用入口
│       ├── board/       # 板级配置
│       ├── drivers/     # 驱动层
│       ├── examples/    # ⭐ 示例代码
│       ├── mw/          # 中间层
│       └── hal/         # 硬件抽象层
├── cpu/bd19/            # CPU相关代码和工具
├── include_lib/         # SDK头文件
├── lib/u8g2/            # u8g2图形库
└── tools/               # 辅助工具
```

---

## 📄 许可证

本项目基于杰理AC63 BT SDK开发，遵循相应的许可协议。

---

## 👥 贡献指南

欢迎提交Issue和Pull Request！

1. Fork本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启Pull Request

---

## 📞 联系方式

- 🌐 官方网站：[https://qianbaii.cn/](https://qianbaii.cn/)
- 📧 作者邮箱：[1756950720@qq.com](mailto:1756950720@qq.com)
-  哔哩哔哩：[千白科技](https://space.bilibili.com/3546810423445874)
- 📱 微信联系：

<div align="center">
  <img src="docs/WeChat_QR.png" alt="千白科技微信二维码" width="200" />
  <p>扫码添加微信，获取技术支持</p>
</div>

- 💬 问题反馈：[Issues页面](../../issues)
- 🤝 技术讨论：[Discussions页面](../../discussions)

---

*最后更新：2026-05-10*
