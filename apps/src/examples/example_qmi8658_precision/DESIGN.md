# QMI8658 精确测量示例开发文档

## 📋 项目概述

### 1.1 项目目标

创建一个**全面验证QMI8658芯片能力**的示例程序，通过模块化设计实现：
- ✅ 基础数据读取与显示（RAW值、物理量）
- ✅ 水平仪功能（姿态可视化，左数据右平衡球布局）
- ✅ 峰值测试工具（冲击力度测量）
- ✅ 统计信息分析（均值、标准差）
- ✅ 温度监控（实时温度显示，已校准）

### 1.2 核心设计理念

**模块化 + 条件编译**
- 每个功能模块独立，可通过宏开关启用/禁用
- 支持阶段性开发和验证
- 最终可整合为完整的全能测试平台

**初始化流程透明化**
- 所有寄存器配置在示例代码中直接实现
- 引用驱动头文件的宏定义，避免重复定义
- 调用底层I2C接口，不依赖驱动的"黑盒"初始化

---

## 🏗️ 系统架构

### 2.1 目录结构

```
fw-AC63_BT_SDK/apps/src/examples/example_qmi8658_precision/
├── qmi8658_precision_example.c    # 主实现文件（约800-1000行）
├── qmi8658_precision_example.h    # 头文件（仅声明启动函数）
└── README.md                      # 使用说明文档
```

### 2.2 模块划分

| 模块名称 | 宏定义 | 功能描述 | 优先级 | 状态 |
|---------|--------|---------|--------|------|
| 数据显示 | `ENABLE_MODULE_DATA_DISPLAY` | RAW值/物理量/单独轴显示 | ⭐⭐⭐ 必须 | ✅ 完成 |
| 水平仪 | `ENABLE_MODULE_LEVEL` | 姿态可视化（左数据右平衡球） | ⭐⭐⭐ 高 | ✅ 完成 |
| 峰值测试 | `ENABLE_MODULE_PEAK_TEST` | 最大加速度记录 | ⭐⭐ 中 | ✅ 完成 |
| 敲击检测 | `ENABLE_MODULE_TAP_DETECT` | Tap事件识别 | ⭐⭐ 中 | ⚠️ 禁用 |
| 统计信息 | `ENABLE_MODULE_STATS` | 均值/标准差计算 | ⭐ 低 | ✅ 完成 |
| 温度监控 | `ENABLE_MODULE_TEMP_MONITOR` | 实时温度显示（已校准） | ⭐ 低 | ✅ 完成 |

### 2.3 显示模式枚举

```c
typedef enum {
    MODE_RAW = 0,           // RAW原始值
    MODE_PHYSICAL,          // 物理量(g, dps)
    MODE_ACCEL_ONLY,        // 仅加速度
    MODE_GYRO_ONLY,         // 仅角速度
    MODE_LEVEL,             // 水平仪
    MODE_PEAK_TEST,         // 峰值测试
    MODE_TAP_DETECT,        // 敲击检测
    MODE_STATS,             // 统计信息
    MODE_TEMP,              // 温度监控
    MODE_COUNT              // 模式总数
} display_mode_t;
```

---

## 🔧 硬件接口设计

### 3.1 I2C通信

**设备地址：**
```c
#define QMI8658_I2C_ADDR    BOARD_IMU_I2C_ADDR7  // 从board_pins.h获取
```

**底层接口（引用i2c_bus.h）：**
```c
int i2c_bus_write_buf(u8 addr7, const u8 *tx, unsigned tx_len);
int i2c_bus_read_buf(u8 addr7, u8 reg, u8 *rx, unsigned rx_len);
int i2c_bus_read_reg8(u8 addr7, u8 reg);
```

### 3.2 寄存器引用

**引用驱动头文件：**
```c
#include "../../drivers/qmi8658/qmi8658a.h"      // 配置宏、数据结构
#include "../../drivers/qmi8658/qmi8658_reg.h"   // 寄存器地址
```

**关键寄存器：**
- `QMI8658_REG_WHO_AM_I` (0x00) - 器件ID
- `QMI8658_REG_CTRL1` (0x02) - 地址自动递增
- `QMI8658_REG_CTRL2` (0x03) - 加速度计配置
- `QMI8658_REG_CTRL3` (0x04) - 陀螺仪配置
- `QMI8658_REG_CTRL5` (0x06) - 低通滤波
- `QMI8658_REG_CTRL7` (0x08) - 传感器使能
- `QMI8658_REG_CTRL8` (0x09) - 运动检测（Tap）
- `QMI8658_REG_CTRL9` (0x0A) - 主机命令
- `QMI8658_REG_AX_L` (0x35) - 加速度数据起始
- `QMI8658_REG_TEMP_L` (0x33) - 温度数据
- `QMI8658_REG_TAP_STATUS` (0x59) - Tap状态

### 3.3 显示屏（u8g2）

**初始化：**
```c
#include "../../lib/u8g2/port/u8g2_port.h"
static u8g2_t u8g2;

u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, 
    u8g2_byte_cb, u8g2_gpio_and_delay_cb);
u8g2_InitDisplay(&u8g2);
u8g2_SetPowerSave(&u8g2, 0);
```

**屏幕尺寸：** 128x64像素

---

## 📝 详细模块设计

### 4.1 数据显示模块（MODULE_DATA_DISPLAY）

#### 4.1.1 功能说明

提供4种数据显示模式：
1. **RAW模式**：显示原始寄存器值（int16）
2. **Physical模式**：显示转换后的物理量（g, dps）
3. **Accel Only**：只显示三轴加速度
4. **Gyro Only**：只显示三轴角速度

#### 4.1.2 数据结构

```c
// 使用驱动提供的数据结构
typedef struct {
    int16_t acc_x, acc_y, acc_z;
    int16_t gyr_x, gyr_y, gyr_z;
} QMI8658_Data_t;

typedef struct {
    float acc_x_g, acc_y_g, acc_z_g;
    float gyr_x_dps, gyr_y_dps, gyr_z_dps;
    float temp_c;
} QMI8658_Physical_t;
```

#### 4.1.3 核心函数

```c
/**
 * @brief 读取传感器原始数据
 * @param raw_data 输出缓冲区
 */
static void read_sensor_data(QMI8658_Data_t *raw_data);

/**
 * @brief 转换为物理量
 * @param raw_data 原始数据
 * @param phys_data 输出物理量
 */
static void convert_to_physical(QMI8658_Data_t *raw_data, 
                                 QMI8658_Physical_t *phys_data);

/**
 * @brief 根据当前模式显示数据
 * @param mode 显示模式
 * @param raw_data 原始数据
 * @param phys_data 物理量数据
 */
static void display_data_mode(display_mode_t mode, 
                               QMI8658_Data_t *raw_data,
                               QMI8658_Physical_t *phys_data);
```

#### 4.1.4 UI布局

**RAW模式：**
```
┌─────────────────────┐
│ [RAW]               │
│ AX:1234 AY:-567     │
│ AZ:8901             │
│ GX:234 GY:-56       │
│ GZ:789              │
└─────────────────────┘
```

**Physical模式：**
```
┌─────────────────────┐
│ [PHYSICAL]          │
│ AX:0.15g AY:-0.07g  │
│ AZ:1.09g            │
│ GX:14.6dps GY:-3.5  │
│ GZ:49.3dps          │
└─────────────────────┘
```

---

### 4.2 水平仪模块（MODULE_LEVEL）

#### 4.2.1 功能说明

利用加速度计计算设备倾斜角度，通过虚拟小球直观显示姿态。

#### 4.2.2 算法设计

**角度计算：**
```c
// Pitch（俯仰角）：绕Y轴旋转
float pitch = atan2f(acc_y_g, sqrtf(acc_x_g*acc_x_g + acc_z_g*acc_z_g)) 
              * 180.0f / PI;

// Roll（横滚角）：绕X轴旋转
float roll = atan2f(-acc_x_g, acc_z_g) * 180.0f / PI;
```

**坐标映射（左数据右平衡球布局）：**
```c
// 左侧数据显示区：0-63像素
// 右侧平衡球显示区：64-127像素，64x64正方形

// 平衡球中心：(96, 32)
// 灵敏度：每度对应1.5像素
int ball_x = 96 + (int)(roll * 1.5f);      // X轴由Roll控制
int ball_y = 32 - (int)(pitch * 1.5f);     // Y轴由Pitch控制（反转）

// 边界限制（在64x64区域内）
ball_x = clamp(ball_x, 64, 127);
ball_y = clamp(ball_y, 0, 63);
```

**注意**：Y轴方向需要反转，因为屏幕坐标系向下为正，而Pitch向上为正。

#### 4.2.3 UI布局（左数据右平衡球）

```
┌─────────────────────┐
│LEVEL                │  ← 标题
│P:5.2°               │  ← Pitch角度
│R:-3.1°              │  ← Roll角度
│Z:0.98g              │  ← Z轴加速度
│                     │
│     ┌──────────┐    │
│     │          │    │
│     │    ●     │    │  ← 平衡球（64x64区域）
│     │          │    │
│     └──────────┘    │
└─────────────────────┘
 0-63   64-127像素
 数据区  平衡球区
```

#### 4.2.4 核心函数

```c
/**
 * @brief 水平仪模式任务
 * @param data 原始传感器数据
 */
static void level_mode_task(QMI8658_Data_t *data);

/**
 * @brief 计算倾斜角度
 * @param acc_x_g X轴加速度(g)
 * @param acc_y_g Y轴加速度(g)
 * @param acc_z_g Z轴加速度(g)
 * @param pitch 输出俯仰角
 * @param roll 输出横滚角
 */
static void calculate_angles(float acc_x_g, float acc_y_g, float acc_z_g,
                              float *pitch, float *roll);
```

---

### 4.3 峰值测试模块（MODULE_PEAK_TEST）

#### 4.3.1 功能说明

记录历史最大加速度和角速度，用于测试冲击、跌落等场景。

#### 4.3.2 数据结构

```c
typedef struct {
    float acc_peak_x, acc_peak_y, acc_peak_z;  // 加速度峰值(g)
    float gyr_peak_x, gyr_peak_y, gyr_peak_z;  // 角速度峰值(dps)
    uint32_t peak_timestamp;                    // 峰值时间戳
} peak_data_t;
```

#### 4.3.3 核心逻辑

```c
/**
 * @brief 更新峰值数据
 * @param phys 当前物理量数据
 */
static void update_peak_data(QMI8658_Physical_t *phys) {
    if (fabsf(phys->acc_x_g) > fabsf(g_peak.acc_peak_x)) {
        g_peak.acc_peak_x = phys->acc_x_g;
        g_peak.peak_timestamp = os_time_get();
    }
    // ... 其他轴同理
}

/**
 * @brief 重置峰值记录
 */
static void reset_peak_data(void) {
    memset(&g_peak, 0, sizeof(peak_data_t));
}
```

#### 4.3.4 UI布局

```
┌─────────────────────┐
│ [PEAK TEST]         │
│ Current:            │
│ AX:0.15g AY:-0.07g  │
│ Peak:               │
│ AX:2.34g* AY:1.89g* │  ← *表示新峰值
└─────────────────────┘
```

---

### 4.4 敲击检测模块（MODULE_TAP_DETECT）⚠️ 已禁用

#### 4.4.1 功能说明

**当前状态：已禁用**

QMI8658内置的Tap检测功能需要完整的寄存器配置，但当前SDK缺少相关寄存器定义。

**禁用原因：**
- SDK头文件中未定义Tap配置所需的专用寄存器地址
- 仅使能CTRL8的TAP_EN位不足以激活完整功能
- 需要配置阈值、持续时间、静默时间等参数

**如需启用：**
1. 查阅QMI8658数据手册，找到Tap配置寄存器地址
2. 在 `qmi8658_reg.h` 中补充寄存器定义
3. 实现完整的配置流程
4. 将 `ENABLE_MODULE_TAP_DETECT` 改为 1

#### 4.4.2 理论配置参数（供参考）

```c
// Tap灵敏度配置（需要在完整实现时配置）
#define TAP_THRESHOLD_DEFAULT    50    // 阈值（LSB）
#define TAP_DURATION_DEFAULT     2     // 持续时间（ms）
#define TAP_LATENCY_DEFAULT      1     // 延迟（ms）

static uint8_t g_tap_threshold = TAP_THRESHOLD_DEFAULT;
```

**注意**：以上参数仅为示例，实际配置需要查阅数据手册确定正确的寄存器和位域。

#### 4.4.3 初始化流程

```c
/**
 * @brief 配置Tap检测
 */
static void configure_tap_detection(void) {
    // 1. 写入配置命令
    i2c_bus_write_buf(QMI8658_I2C_ADDR, 
        (uint8_t[]){QMI8658_REG_CTRL9, QMI8658_CMD_CFG_TAP}, 2);
    os_time_dly(10);
    
    // 2. 使能Tap检测
    i2c_bus_write_buf(QMI8658_I2C_ADDR, 
        (uint8_t[]){QMI8658_REG_CTRL8, QMI8658_CTRL8_TAP_EN}, 2);
    
    printf("[TAP] Detection enabled\n");
}
```

#### 4.4.4 事件检测

```c
/**
 * @brief 检查Tap事件
 */
static void check_tap_event(void) {
    uint8_t tap_status;
    i2c_bus_read_reg8(QMI8658_I2C_ADDR, QMI8658_REG_TAP_STATUS);
    
    if (tap_status & 0x01) {
        printf("[TAP] Single Tap detected!\n");
        // OLED显示提示
    }
    if (tap_status & 0x02) {
        printf("[TAP] Double Tap detected!\n");
    }
}
```

#### 4.4.5 UI布局

```
┌─────────────────────┐
│ [TAP DETECT]        │
│ Threshold: 50       │
│                     │
│ Waiting for tap...  │
│                     │
│ ✓ Single Tap!       │  ← 检测到事件
└─────────────────────┘
```

---

### 4.5 统计信息模块（MODULE_STATS）

#### 4.5.1 功能说明

在线计算均值和标准差，评估传感器数据稳定性。

#### 4.5.2 数据结构

```c
typedef struct {
    float acc_x_sum, acc_y_sum, acc_z_sum;
    float acc_x_sq_sum, acc_y_sq_sum, acc_z_sq_sum;
    float gyr_x_sum, gyr_y_sum, gyr_z_sum;
    float gyr_x_sq_sum, gyr_y_sq_sum, gyr_z_sq_sum;
    uint32_t sample_count;
} stats_data_t;
```

#### 4.5.3 统计算法

```c
/**
 * @brief 更新统计数据
 * @param phys 当前物理量数据
 */
static void update_stats(QMI8658_Physical_t *phys) {
    g_stats.sample_count++;
    
    // 累加
    g_stats.acc_x_sum += phys->acc_x_g;
    g_stats.acc_x_sq_sum += phys->acc_x_g * phys->acc_x_g;
    
    // ... 其他轴同理
}

/**
 * @brief 计算均值和标准差
 * @param sum 累加和
 * @param sq_sum 平方和
 * @param count 样本数
 * @param mean 输出均值
 * @param stddev 输出标准差
 */
static void calculate_stddev(float sum, float sq_sum, uint32_t count,
                              float *mean, float *stddev) {
    *mean = sum / count;
    float variance = (sq_sum / count) - (*mean * *mean);
    *stddev = sqrtf(variance > 0 ? variance : 0);
}
```

#### 4.5.4 UI布局

```
┌─────────────────────┐
│ [STATS] N=1234      │
│ ACC_X:              │
│ Mean: 0.002g        │
│ Std:  0.015g        │
│ ACC_Y:              │
│ Mean:-0.001g        │
│ Std:  0.012g        │
└─────────────────────┘
```

---

### 4.6 温度监控模块（MODULE_TEMP_MONITOR）

#### 4.6.1 功能说明

实时显示传感器内部温度，**已添加校准功能**。

**重要说明：**
- QMI8658测量的是芯片结温（Die Temperature），不是环境温度
- 原始读数通常比环境温度高20-30°C
- 已通过减去固定偏移量进行校准，使显示值接近实际温度

#### 4.6.2 温度计算公式

```c
// 原始公式（来自QMI8658数据手册）
float temperature_raw = 25.0f + (float)raw_temp / 256.0f;

// 校准后公式
#define TEMP_CALIBRATION_OFFSET  22.0f  // 校准偏移量
float temperature_calibrated = temperature_raw - TEMP_CALIBRATION_OFFSET;
```

**校准方法：**
1. 在已知环境温度下（如25°C），记录显示值
2. 计算偏移量：`offset = 显示值 - 环境温度`
3. 修改 `TEMP_CALIBRATION_OFFSET` 宏定义

#### 4.6.3 核心函数

```c
/**
 * @brief 温度监控模式
 */
static void temp_monitor_mode(void) {
    float temperature = 0.0f;
    QMI8658_ReadTemperature(&temperature);
    
    // 应用校准
    #define TEMP_CALIBRATION_OFFSET  22.0f
    float calibrated_temp = temperature - TEMP_CALIBRATION_OFFSET;
    
    // 显示
    char str[32];
    int temp_fixed = (int)(calibrated_temp * 100);
    sprintf(str, "%d.%02d C", temp_fixed/100, temp_abs%100);
    u8g2_DrawStr(&u8g2, 15, 45, str);
}
```

#### 4.6.4 UI布局

```
┌─────────────────────┐
│ Temp                │
│                     │
│   30.25 C           │
│                     │
│ (Calibrated)        │
└─────────────────────┘
```

---

## ⚙️ 初始化流程设计

### 5.1 完整初始化步骤

```c
static int qmi8658_precision_init(void) {
    int ret;
    uint8_t whoami;
    
    // Step 1: 读取WHOAMI验证连接
    ret = i2c_bus_read_reg8(QMI8658_I2C_ADDR, QMI8658_REG_WHO_AM_I);
    if (ret < 0) {
        printf("[QMI8658] ERROR: read WHOAMI failed\n");
        return -1;
    }
    whoami = (uint8_t)ret;
    printf("[QMI8658] WHOAMI: 0x%02X\n", whoami);
    
    // Step 2: 软复位
    i2c_bus_write_buf(QMI8658_I2C_ADDR, 
                      (uint8_t[]){QMI8658_REG_RESET, QMI8658_SOFT_RESET_VAL}, 2);
    os_time_dly(50);  // 等待复位完成
    
    // Step 3: 使能地址自动递增
    i2c_bus_write_buf(QMI8658_I2C_ADDR, 
                      (uint8_t[]){QMI8658_REG_CTRL1, QMI8658_CTRL1_ADDR_AI_EN}, 2);
    
    // Step 4: 配置加速度计：±4g, 512Hz
    uint8_t ctrl2 = QMI8658_CTRL2_ACC_RANGE_4G | QMI8658_CTRL2_ACC_ODR_512HZ;
    i2c_bus_write_buf(QMI8658_I2C_ADDR, 
                      (uint8_t[]){QMI8658_REG_CTRL2, ctrl2}, 2);
    
    // Step 5: 配置陀螺仪：±2000dps, 512Hz
    uint8_t ctrl3 = QMI8658_CTRL3_GYR_RANGE_2000DPS | QMI8658_CTRL3_GYR_ODR_512HZ;
    i2c_bus_write_buf(QMI8658_I2C_ADDR, 
                      (uint8_t[]){QMI8658_REG_CTRL3, ctrl3}, 2);
    
    // Step 6: 配置低通滤波器：50Hz
    uint8_t ctrl5 = QMI8658_CTRL5_ACC_LPF_50HZ | QMI8658_CTRL5_GYR_LPF_50HZ;
    i2c_bus_write_buf(QMI8658_I2C_ADDR, 
                      (uint8_t[]){QMI8658_REG_CTRL5, ctrl5}, 2);
    
    // Step 7: 使能传感器（ACC + GYR + TEMP）
    uint8_t ctrl7 = QMI8658_CTRL7_ACC_EN | QMI8658_CTRL7_GYR_EN | QMI8658_CTRL7_TEMP_EN;
    i2c_bus_write_buf(QMI8658_I2C_ADDR, 
                      (uint8_t[]){QMI8658_REG_CTRL7, ctrl7}, 2);
    
    // Step 8: 【可选】配置Tap检测
    #if ENABLE_MODULE_TAP_DETECT
    i2c_bus_write_buf(QMI8658_I2C_ADDR, 
        (uint8_t[]){QMI8658_REG_CTRL9, QMI8658_CMD_CFG_TAP}, 2);
    os_time_dly(10);
    i2c_bus_write_buf(QMI8658_I2C_ADDR, 
        (uint8_t[]){QMI8658_REG_CTRL8, QMI8658_CTRL8_TAP_EN}, 2);
    #endif
    
    // Step 9: 等待传感器稳定
    os_time_dly(100);
    
    // Step 10: 验证数据就绪
    ret = i2c_bus_read_reg8(QMI8658_I2C_ADDR, QMI8658_REG_STATUSINT);
    if (ret >= 0) {
        printf("[QMI8658] Status: 0x%02X\n", (uint8_t)ret);
    }
    
    printf("[QMI8658] Init OK\n");
    return 0;
}
```

### 5.2 校准流程

```c
/**
 * @brief 智能校准（在示例中实现）
 */
static int calibrate_in_example(void) {
    int32_t sum_x = 0, sum_y = 0, sum_z = 0;
    const int samples = 200;
    
    printf("Calibrating gyro...\n");
    
    for (int i = 0; i < samples; i++) {
        uint8_t buf[12];
        i2c_bus_read_buf(QMI8658_I2C_ADDR, QMI8658_REG_AX_L, buf, 12);
        
        int16_t gyr_x = (int16_t)((buf[7] << 8) | buf[6]);
        int16_t gyr_y = (int16_t)((buf[9] << 8) | buf[8]);
        int16_t gyr_z = (int16_t)((buf[11] << 8) | buf[10]);
        
        sum_x += gyr_x;
        sum_y += gyr_y;
        sum_z += gyr_z;
        
        os_time_dly(1);
    }
    
    g_calib.gyr_offset_x = (int16_t)(sum_x / samples);
    g_calib.gyr_offset_y = (int16_t)(sum_y / samples);
    g_calib.gyr_offset_z = (int16_t)(sum_z / samples);
    g_calibrated = 1;
    
    printf("Offset: (%d, %d, %d)\n", 
           g_calib.gyr_offset_x, g_calib.gyr_offset_y, g_calib.gyr_offset_z);
    
    return 0;
}
```

---

## 🎮 按键交互设计

### 6.1 按键映射

| 按键 | 短按功能 | 长按功能 | 双击功能 |
|------|---------|---------|---------|
| KEY1 | 切换显示模式 | - | - |
| KEY2 | 手动校准 | - | - |
| KEY3 | 重置峰值 / 调节Tap阈值 | 进入水平仪模式 | - |
| KEY4 | 冻结/解冻显示 | - | - |

### 6.2 模式切换逻辑

```c
void example_key_handler(u8 key_value, u8 event_type) {
    if (event_type != KEY_EVENT_CLICK) return;
    
    switch (key_value) {
        case 0:  // KEY1: 切换模式
            g_current_mode = (g_current_mode + 1) % MODE_COUNT;
            printf("[MODE] Switch to mode %d\n", g_current_mode);
            break;
            
        case 1:  // KEY2: 校准
            calibrate_in_example();
            break;
            
        case 2:  // KEY3: 功能键（根据当前模式）
            #if ENABLE_MODULE_PEAK_TEST
            if (g_current_mode == MODE_PEAK_TEST) {
                reset_peak_data();
            }
            #endif
            #if ENABLE_MODULE_TAP_DETECT
            if (g_current_mode == MODE_TAP_DETECT) {
                g_tap_threshold = (g_tap_threshold + 10) % 200;
                printf("[TAP] Threshold: %d\n", g_tap_threshold);
            }
            #endif
            break;
            
        case 3:  // KEY4: 冻结显示
            g_frozen = !g_frozen;
            printf("[DISPLAY] %s\n", g_frozen ? "Frozen" : "Active");
            break;
    }
}
```

---

## 📊 串口调试输出

### 7.1 调试宏定义

```c
#define ENABLE_SERIAL_DEBUG    1

#if ENABLE_SERIAL_DEBUG
    #define DEBUG_PRINT(fmt, ...) printf("[QMI_PREC] " fmt, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...)
#endif
```

### 7.2 输出内容

**初始化阶段：**
```
[QMI_PREC] WHOAMI: 0x05
[QMI_PREC] Init OK
[TAP] Detection enabled
```

**数据采集：**
```
[QMI_PREC] Mode: RAW
[QMI_PREC] AX:1234 AY:-567 AZ:8901
[QMI_PREC] GX:234 GY:-56 GZ:789
```

**事件触发：**
```
[TAP] Single Tap detected!
[PEAK] New peak: AX=2.34g
[CAL] Offset: (12, -8, 5)
```

---

## 🔍 测试计划

### 8.1 阶段性验证

**阶段1：基础数据读取**
- [ ] WHOAMI读取正确
- [ ] RAW数据正常变化
- [ ] 物理量转换准确
- [ ] 串口输出正常

**阶段2：水平仪功能**
- [ ] 角度计算准确
- [ ] 小球移动平滑
- [ ] 边界限制有效

**阶段3：峰值测试**
- [ ] 峰值记录正确
- [ ] 重置功能正常
- [ ] 显示清晰

**阶段4：敲击检测**
- [ ] Tap配置成功
- [ ] 单击识别准确
- [ ] 双击识别准确

**阶段5：统计信息**
- [ ] 均值计算正确
- [ ] 标准差合理
- [ ] 采样计数准确

**阶段6：整合测试**
- [ ] 所有模块同时运行
- [ ] 模式切换流畅
- [ ] 无内存泄漏

### 8.2 性能指标

| 指标 | 目标值 | 测量方法 |
|------|--------|---------|
| 刷新率 | ≥50Hz | 示波器测量OLED刷新 |
| CPU占用 | <30% | SDK性能监测工具 |
| RAM占用 | <2KB | 编译器报告 |
| Flash占用 | <20KB | 编译器报告 |

---

## 📦 配置系统集成

### 9.1 example_config.h

```c
/* ========== 示例选择 ========== */
#define ENABLE_EXAMPLE_QMI8658_PRECISION    1
#define ENABLE_EXAMPLE_QMI8658              0  // 保留旧的作为参考

/* ========== 配置检查 ========== */
#define ENABLED_EXAMPLE_COUNT ( \
    ENABLE_EXAMPLE_QMI8658_PRECISION + \
    ENABLE_EXAMPLE_QMI8658 + \
    /* ... */ \
)
```

### 9.2 mw_runtime.c

```c
#elif ENABLE_EXAMPLE_QMI8658_PRECISION
    #include "../examples/example_qmi8658_precision/qmi8658_precision_example.h"
    qmi8658_precision_example_start();
```

---

## 🚀 实施路线图

### Week 1: 基础框架
- [ ] 创建目录和文件
- [ ] 实现初始化流程
- [ ] 实现数据显示模块
- [ ] 测试基础功能

### Week 2: 核心功能
- [ ] 实现水平仪模块
- [ ] 实现峰值测试模块
- [ ] 测试两个模块

### Week 3: 高级功能
- [ ] 实现敲击检测模块
- [ ] 实现统计信息模块
- [ ] 实现温度监控模块

### Week 4: 整合优化
- [ ] 整合所有模块
- [ ] 优化UI布局
- [ ] 性能测试和优化
- [ ] 编写README

---

## ❓ 常见问题预留

### Q1: 如何添加新的功能模块？
**A:** 
1. 在枚举中添加新模式
2. 定义新的宏开关
3. 实现模块函数
4. 在主循环switch中添加case
5. 更新按键映射

### Q2: 如何优化内存占用？
**A:**
- 禁用不需要的模块
- 减少统计数据的精度
- 使用局部变量代替全局变量

### Q3: Tap检测灵敏度如何调整？
**A:**
- 修改`TAP_THRESHOLD_DEFAULT`宏
- 或通过KEY3按键动态调节
- 参考QMI8658数据手册第X章

### Q4: 如何实现平滑的模式切换动画？
**A:**
- 使用u8g2的淡入淡出效果
- 添加过渡帧
- 参考u8g2文档的动画示例

---

## 📚 参考资料

1. QMI8658数据手册
2. u8g2图形库文档
3. 杰理SDK开发指南
4. 本项目DEVELOPMENT_GUIDE.md

---

*文档版本：v1.0*  
*最后更新：2026-05-10*
