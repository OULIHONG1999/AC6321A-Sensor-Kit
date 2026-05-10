# QMI8658 精确测量示例

## 功能说明

这是一个全面验证QMI8658芯片能力的示例程序，采用模块化设计，包含以下功能模块：

- **数据显示模块** - 显示RAW原始值或转换后的物理量（g, dps）
- **水平仪功能** - 通过加速度计计算姿态角度，可视化显示（左数据右平衡球布局）
- **峰值测试工具** - 记录历史最大加速度和角速度
- **统计信息分析** - 在线计算均值和标准差，评估数据稳定性
- **温度监控** - 实时显示传感器内部温度（已校准）

## 硬件需求

- AC6321A开发板
- QMI8658六轴IMU传感器模块
- 连接方式：I2C

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
#define ENABLE_EXAMPLE_QMI8658_PRECISION  1
#define ENABLE_EXAMPLE_QMI8658            0  // 关闭其他示例
#define ENABLE_EXAMPLE_GYRO_BALL          0
```

### 2. 编译烧录

```bash
make clean
make
# 使用下载工具烧录
```

### 3. 操作说明

- **KEY1短按**：切换显示模式（RAW/Physical/Accel Only/Gyro Only/Level/Peak/Stats/Temp）
- **KEY2短按**：手动校准传感器
- **KEY3短按**：根据当前模式执行不同功能
  - 峰值测试模式：重置峰值记录
- **KEY4短按**：冻结/解冻显示

## 显示模式说明

### RAW模式
显示原始寄存器值（int16格式），直接显示整数：
```
[RAW]
AX: 2796      GX: -1313
AY: 1874      GY: -1749
AZ: -2471     GZ: -3033
```

### Physical模式
显示转换后的物理量：
- 加速度：单位g（重力加速度）
- 角速度：单位dps（度/秒）

### Accel Only模式
只显示三轴加速度数据

### Gyro Only模式
只显示三轴角速度数据

### Level模式（水平仪）
- 显示俯仰角（Pitch）、横滚角（Roll）和Z轴加速度
- **左侧数据显示区**：显示角度和加速度数值
- **右侧平衡球显示区**：64x64像素正方形区域，小球随设备倾斜移动
- 灵敏度可调，响应速度快（5ms刷新周期）

### Peak Test模式（峰值测试）
- 显示当前加速度值和历史峰值
- 用于测试冲击、跌落等场景
- KEY3可重置峰值记录

### Tap Detect模式（敲击检测）
- 启用QMI8658内置Tap检测功能
- 识别单击/双击事件
- KEY3可调节检测阈值

### Stats模式（统计信息）
- 显示采样数量
- 计算并显示X/Y/Z三轴的均值和标准差
- 评估传感器数据稳定性和噪声水平
- 格式：`AX:均值 S:标准差`

### Temp模式（温度监控）
- 实时显示传感器内部温度
- 单位：摄氏度（°C）
- **已校准**：减去固定偏移量以接近环境温度
- 提示：显示的是芯片结温，非环境温度

## 模块配置

### 功能模块开关

在代码开头可以通过宏定义启用/禁用特定模块：

```c
#define ENABLE_MODULE_DATA_DISPLAY  1   // 数据显示模块
#define ENABLE_MODULE_LEVEL         1   // 水平仪模块
#define ENABLE_MODULE_PEAK_TEST     1   // 峰值测试模块
#define ENABLE_MODULE_TAP_DETECT    0   // 敲击检测模块（需要完整寄存器配置，暂时禁用）
#define ENABLE_MODULE_STATS         1   // 统计信息模块
#define ENABLE_MODULE_TEMP_MONITOR  1   // 温度监控模块
```

将不需要的模块设为0可减少资源占用。

**注意**：Tap检测模块当前已禁用，因为SDK缺少完整的Tap配置寄存器定义。如需启用，需要查阅QMI8658数据手册补充相关寄存器配置。

### 传感器量程配置（智能映射）

示例采用**智能量程配置机制**，只需修改配置宏，比例因子会自动适配：

```c
// 配置量程（与初始化代码保持一致）
#define ACC_RANGE_CONFIG    QMI8658_CTRL2_ACC_RANGE_4G    // 加速度计量程
#define GYR_RANGE_CONFIG    QMI8658_CTRL3_GYR_RANGE_2000DPS  // 陀螺仪量程
```

**支持的量程选项：**

| 加速度计量程 | 宏定义 | 比例因子 |
|------------|--------|----------|
| ±2g | `QMI8658_CTRL2_ACC_RANGE_2G` | 16384 LSB/g |
| ±4g | `QMI8658_CTRL2_ACC_RANGE_4G` | 8192 LSB/g |
| ±8g | `QMI8658_CTRL2_ACC_RANGE_8G` | 4096 LSB/g |
| ±16g | `QMI8658_CTRL2_ACC_RANGE_16G` | 2048 LSB/g |

| 陀螺仪量程 | 宏定义 | 比例因子 |
|-----------|--------|----------|
| ±250dps | `QMI8658_CTRL3_GYR_RANGE_250DPS` | 131 LSB/dps |
| ±500dps | `QMI8658_CTRL3_GYR_RANGE_500DPS` | 65.5 LSB/dps |
| ±1000dps | `QMI8658_CTRL3_GYR_RANGE_1000DPS` | 32.8 LSB/dps |
| ±2000dps | `QMI8658_CTRL3_GYR_RANGE_2000DPS` | 16.4 LSB/dps |

**使用方法：**
1. 修改 `ACC_RANGE_CONFIG` 和 `GYR_RANGE_CONFIG` 宏
2. 编译时会自动选择正确的比例因子
3. 无需手动修改转换函数

**优势：**
- ✅ 配置与转换自动同步，避免不一致
- ✅ 编译时检查，不支持的量程会报错
- ✅ 只需修改一处，全局生效

## 技术细节

### 显示方案

本示例使用 **u8g2图形库** 进行显示，提供丰富的图形功能和字体支持。

**u8g2优势：**
- ✅ 支持多种字体（50+种）
- ✅ 支持复杂图形绘制（圆、线、矩形等）
- ✅ 支持动画效果
- ✅ 统一的API接口

**注意事项：**
- ⚠️ 每次刷新前必须调用`u8g2_ClearBuffer()`清除缓冲区，避免残影
- ⚠️ 所有绘制操作在缓冲区中进行，最后调用`u8g2_SendBuffer()`一次性输出

### 初始化流程

1. 电源使能并等待稳定
2. I2C总线初始化
3. u8g2显示屏初始化（SSD1306 I2C接口）
4. QMI8658传感器配置：
   - 加速度计：±4g量程，512Hz采样率
   - 陀螺仪：±2000dps量程，512Hz采样率
   - 低通滤波器：50Hz截止频率
   - 使能加速度计、陀螺仪、温度传感器
5. 主循环：5ms刷新周期，最大化响应速度

### 算法说明

**水平仪角度计算：**
- Pitch（俯仰角）：`atan2(acc_y, sqrt(acc_x² + acc_z²))`
- Roll（横滚角）：`atan2(-acc_x, acc_z)`
- 小球位置映射：`ball_x = 96 + roll * 1.5`, `ball_y = 32 - pitch * 1.5`

**统计计算：**
- 均值：`sum / count`
- 标准差：`sqrt((sq_sum / count) - mean²)`

**温度校准：**
- 原始公式：`T = 25.0 + raw / 256.0`
- 校准后：`T_calibrated = T_raw - TEMP_CALIBRATION_OFFSET`
- 默认偏移量：22°C（可根据实际情况调整）

## 注意事项

- 首次使用前建议进行传感器校准（KEY2长按）
- 水平仪功能需要设备相对静止才能获得准确角度
- 统计信息会随着采样时间增长而更加准确
- 温度显示已校准，如需微调可修改`TEMP_CALIBRATION_OFFSET`宏定义
- Tap检测功能当前已禁用，如需使用需补充完整的寄存器配置

## 扩展开发

如需自定义功能，可以修改：
- `example_key_handler()` - 按键逻辑
- `qmi8658_precision_example_task()` - 主循环逻辑
- 添加新的显示模式或功能模块

## 性能指标

- 刷新率：约100Hz
- RAM占用：<2KB
- Flash占用：<20KB（取决于启用的模块）
