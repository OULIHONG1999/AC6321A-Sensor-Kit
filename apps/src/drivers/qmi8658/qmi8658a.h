#ifndef QMI8658A_H
#define QMI8658A_H

#include <stdint.h>

// 调试开关
#define QMI8658_DEBUG

#ifdef QMI8658_DEBUG
  #define QMI8658_DEBUG_PRINT(fmt, ...) printf("[QMI8658] " fmt, ##__VA_ARGS__)
#else
  #define QMI8658_DEBUG_PRINT(fmt, ...)
#endif

/************************** CTRL1 (0x02) 串行接口 & 基本配置 **************************
 *  bit7    : SPI4_3          0=SPI 4线, 1=SPI 3线
 *  bit6    : ADDR_AI         0=禁止地址自动增量, 1=使能地址自动增量 (推荐开启)
 *  bit5    : Reserved        保留
 *  bit4    : INT_PIN         0=INT推挽输出, 1=INT开漏输出
 *  bit3    : INT_POL         0=INT低电平有效, 1=INT高电平有效
 *  bit2    : Reserved        保留
 *  bit1    : Reserved        保留
 *  bit0    : Reserved        保留
 */
#define QMI8658_CTRL1_SPI4_3_Pos        7U
#define QMI8658_CTRL1_SPI4_3_Msk        (1U << 7)
#define QMI8658_CTRL1_SPI_4WIRE         (0U << 7)
#define QMI8658_CTRL1_SPI_3WIRE         (1U << 7)

#define QMI8658_CTRL1_ADDR_AI_Pos       6U
#define QMI8658_CTRL1_ADDR_AI_Msk        (1U << 6)
#define QMI8658_CTRL1_ADDR_AI_EN        (1U << 6)  // 使能自动地址递增（必须开）

#define QMI8658_CTRL1_INT_PIN_Pos       4U
#define QMI8658_CTRL1_INT_PIN_Msk       (1U << 4)
#define QMI8658_CTRL1_INT_PUSHPULL      (0U << 4)
#define QMI8658_CTRL1_INT_OPENDRAIN     (1U << 4)

#define QMI8658_CTRL1_INT_POL_Pos       3U
#define QMI8658_CTRL1_INT_POL_Msk       (1U << 3)
#define QMI8658_CTRL1_INT_ACTIVE_LOW    (0U << 3)
#define QMI8658_CTRL1_INT_ACTIVE_HIGH   (1U << 3)

/************************** CTRL2 (0x03) 加速度计配置 **************************
 *  bit7-5  : ACC_RANGE        加速度计量程
 *              000: ±2g     001: ±4g     010: ±8g     011: ±16g
 *  bit4-0  : ACC_ODR         加速度输出数据速率
 */
#define QMI8658_CTRL2_ACC_RANGE_Pos     5U
#define QMI8658_CTRL2_ACC_RANGE_Msk     (7U << 5)
#define QMI8658_CTRL2_ACC_RANGE_2G      (0U << 5)
#define QMI8658_CTRL2_ACC_RANGE_4G      (1U << 5)
#define QMI8658_CTRL2_ACC_RANGE_8G      (2U << 5)
#define QMI8658_CTRL2_ACC_RANGE_16G     (3U << 5)

#define QMI8658_CTRL2_ACC_ODR_Pos       0U
#define QMI8658_CTRL2_ACC_ODR_Msk       (0x1FU)
#define QMI8658_CTRL2_ACC_ODR_8HZ       0x01
#define QMI8658_CTRL2_ACC_ODR_16HZ      0x02
#define QMI8658_CTRL2_ACC_ODR_32HZ      0x03
#define QMI8658_CTRL2_ACC_ODR_64HZ      0x04
#define QMI8658_CTRL2_ACC_ODR_128HZ     0x05
#define QMI8658_CTRL2_ACC_ODR_256HZ     0x06
#define QMI8658_CTRL2_ACC_ODR_512HZ     0x07
#define QMI8658_CTRL2_ACC_ODR_1024HZ    0x08

/************************** CTRL3 (0x04) 陀螺仪配置 **************************
 *  bit7-5  : GYR_RANGE        陀螺仪量程
 *              000: ±250dps    001: ±500dps    010: ±1000dps    011: ±2000dps
 *  bit4-0  : GYR_ODR          陀螺仪输出数据速率
 */
#define QMI8658_CTRL3_GYR_RANGE_Pos     5U
#define QMI8658_CTRL3_GYR_RANGE_Msk     (7U << 5)
#define QMI8658_CTRL3_GYR_RANGE_250DPS  (0U << 5)
#define QMI8658_CTRL3_GYR_RANGE_500DPS  (1U << 5)
#define QMI8658_CTRL3_GYR_RANGE_1000DPS (2U << 5)
#define QMI8658_CTRL3_GYR_RANGE_2000DPS (3U << 5)

#define QMI8658_CTRL3_GYR_ODR_Pos       0U
#define QMI8658_CTRL3_GYR_ODR_Msk       (0x1FU)
#define QMI8658_CTRL3_GYR_ODR_8HZ       0x01
#define QMI8658_CTRL3_GYR_ODR_16HZ      0x02
#define QMI8658_CTRL3_GYR_ODR_32HZ      0x03
#define QMI8658_CTRL3_GYR_ODR_64HZ      0x04
#define QMI8658_CTRL3_GYR_ODR_128HZ     0x05
#define QMI8658_CTRL3_GYR_ODR_256HZ     0x06
#define QMI8658_CTRL3_GYR_ODR_512HZ     0x07
#define QMI8658_CTRL3_GYR_ODR_1024HZ    0x08

/************************** CTRL5 (0x06) 低通滤波配置 **************************
 *  bit5-4  : ACC_LPF         加速度低通滤波
 *  bit1-0  : GYR_LPF         陀螺低通滤波
 */
#define QMI8658_CTRL5_ACC_LPF_Pos       4U
#define QMI8658_CTRL5_ACC_LPF_Msk       (3U << 4)
#define QMI8658_CTRL5_ACC_LPF_DIS      (0U << 4)
#define QMI8658_CTRL5_ACC_LPF_50HZ     (1U << 4)
#define QMI8658_CTRL5_ACC_LPF_100HZ    (2U << 4)
#define QMI8658_CTRL5_ACC_LPF_200HZ    (3U << 4)

#define QMI8658_CTRL5_GYR_LPF_Pos       0U
#define QMI8658_CTRL5_GYR_LPF_Msk       (3U << 0)
#define QMI8658_CTRL5_GYR_LPF_DIS      (0U << 0)
#define QMI8658_CTRL5_GYR_LPF_50HZ     (1U << 0)
#define QMI8658_CTRL5_GYR_LPF_100HZ    (2U << 0)
#define QMI8658_CTRL5_GYR_LPF_200HZ    (3U << 0)

/************************** CTRL6 (0x07) QMI8658C 专用 姿态引擎配置 **************************
 *  仅 QMI8658C 有效, QMI8658A 保留
 */
#define QMI8658_CTRL6_AE_ODR_Pos        0U
#define QMI8658_CTRL6_AE_ODR_Msk        (0x3FU)

/************************** CTRL7 (0x08) 传感器使能控制 **************************
 *  bit7    : TEMP_EN         温度传感器使能
 *  bit1    : GYR_EN          陀螺仪使能
 *  bit0    : ACC_EN          加速度计使能
 */
#define QMI8658_CTRL7_TEMP_EN_Pos       7U
#define QMI8658_CTRL7_TEMP_EN_Msk       (1U << 7)
#define QMI8658_CTRL7_TEMP_EN           (1U << 7)

#define QMI8658_CTRL7_GYR_EN_Pos        1U
#define QMI8658_CTRL7_GYR_EN_Msk        (1U << 1)
#define QMI8658_CTRL7_GYR_EN            (1U << 1)

#define QMI8658_CTRL7_ACC_EN_Pos        0U
#define QMI8658_CTRL7_ACC_EN_Msk        (1U << 0)
#define QMI8658_CTRL7_ACC_EN            (1U << 0)

/************************** STATUSINT (0x2D) 状态寄存器 **************************
 *  bit1    : DRDY_GYR        陀螺仪数据就绪
 *  bit0    : DRDY_ACC        加速度计数据就绪
 */
#define QMI8658_STATUSINT_DRDY_GYR_Pos  1U
#define QMI8658_STATUSINT_DRDY_GYR_Msk  (1U << 1)
#define QMI8658_STATUSINT_DRDY_ACC_Pos  0U
#define QMI8658_STATUSINT_DRDY_ACC_Msk  (1U << 0)

/************************** CTRL8 (0x09) 运动检测控制 **************************
 *  bit5    : TAP_EN          单击/双击使能
 *  bit4    : PEDO_EN         计步器使能
 *  bit0    : WOM_EN          运动唤醒使能
 */
#define QMI8658_CTRL8_TAP_EN_Pos        5U
#define QMI8658_CTRL8_TAP_EN_Msk        (1U << 5)
#define QMI8658_CTRL8_TAP_EN            (1U << 5)

#define QMI8658_CTRL8_PEDO_EN_Pos       4U
#define QMI8658_CTRL8_PEDO_EN_Msk       (1U << 4)
#define QMI8658_CTRL8_PEDO_EN           (1U << 4)

#define QMI8658_CTRL8_WOM_EN_Pos        0U
#define QMI8658_CTRL8_WOM_EN_Msk        (1U << 0)
#define QMI8658_CTRL8_WOM_EN            (1U << 0)

/************************** CTRL9 (0x0A) 主机命令寄存器 **************************
 *  写入命令值执行对应操作
 */
#define QMI8658_CMD_NOP                 0x00
#define QMI8658_CMD_RST_FIFO            0x04
#define QMI8658_CMD_SET_WOM             0x08
#define QMI8658_CMD_ACC_OFFSET          0x09
#define QMI8658_CMD_GYR_OFFSET          0x0A
#define QMI8658_CMD_CFG_TAP             0x0C
#define QMI8658_CMD_CFG_PEDO            0x0D
#define QMI8658_CMD_RST_PEDO            0x0F
#define QMI8658_CMD_CALIB               0xA2

/************************** FIFO 配置 (0x14) **************************
 *  bit7    : FIFO_EN         FIFO使能
 *  bit6    : FIFO_STOP       FIFO停止（数据满时停止）
 *  bit5    : FIFO_AE         姿态引擎数据使能
 *  bit4    : TIME_EN         时间戳使能
 *  bit3    : TEMP_EN         温度数据使能
 *  bit2    : GYR_EN          陀螺仪数据使能
 *  bit1    : ACC_EN          加速度计数据使能
 *  bit0    : SLINK           顺序链接模式
 */
#define QMI8658_FIFO_CTRL_EN_Pos        7U
#define QMI8658_FIFO_CTRL_EN_Msk        (1U << 7)
#define QMI8658_FIFO_CTRL_EN            (1U << 7)

#define QMI8658_FIFO_CTRL_STOP_Pos      6U
#define QMI8658_FIFO_CTRL_STOP_Msk       (1U << 6)
#define QMI8658_FIFO_CTRL_STOP          (1U << 6)

#define QMI8658_FIFO_CTRL_AE_Pos        5U
#define QMI8658_FIFO_CTRL_AE_Msk        (1U << 5)
#define QMI8658_FIFO_CTRL_AE            (1U << 5)

#define QMI8658_FIFO_CTRL_TIME_Pos      4U
#define QMI8658_FIFO_CTRL_TIME_Msk      (1U << 4)
#define QMI8658_FIFO_CTRL_TIME          (1U << 4)

#define QMI8658_FIFO_CTRL_TEMP_Pos      3U
#define QMI8658_FIFO_CTRL_TEMP_Msk      (1U << 3)
#define QMI8658_FIFO_CTRL_TEMP          (1U << 3)

#define QMI8658_FIFO_CTRL_GYR_Pos       2U
#define QMI8658_FIFO_CTRL_GYR_Msk       (1U << 2)
#define QMI8658_FIFO_CTRL_GYR           (1U << 2)

#define QMI8658_FIFO_CTRL_ACC_Pos       1U
#define QMI8658_FIFO_CTRL_ACC_Msk       (1U << 1)
#define QMI8658_FIFO_CTRL_ACC           (1U << 1)

#define QMI8658_FIFO_CTRL_SLINK_Pos     0U
#define QMI8658_FIFO_CTRL_SLINK_Msk     (1U << 0)
#define QMI8658_FIFO_CTRL_SLINK         (1U << 0)

/************************** FIFO 状态 (0x16) **************************
 *  bit7    : FIFO_EMPTY      FIFO为空
 *  bit6    : FIFO_FULL       FIFO为满
 *  bit5    : FIFO_OVF        FIFO溢出
 *  bit4    : FIFO_WTM        FIFO达到水印阈值
 *  bit3-0  : FIFO_COUNT      FIFO数据计数
 */
#define QMI8658_FIFO_STATUS_EMPTY_Pos   7U
#define QMI8658_FIFO_STATUS_EMPTY_Msk   (1U << 7)

#define QMI8658_FIFO_STATUS_FULL_Pos    6U
#define QMI8658_FIFO_STATUS_FULL_Msk   (1U << 6)

#define QMI8658_FIFO_STATUS_OVF_Pos     5U
#define QMI8658_FIFO_STATUS_OVF_Msk    (1U << 5)

#define QMI8658_FIFO_STATUS_WTM_Pos     4U
#define QMI8658_FIFO_STATUS_WTM_Msk    (1U << 4)

#define QMI8658_FIFO_STATUS_COUNT_Pos   0U
#define QMI8658_FIFO_STATUS_COUNT_Msk   (0xFU << 0)

// 设备类型枚举
typedef enum {
    QMI8658_TYPE_UNKNOWN = 0,
    QMI8658_TYPE_A = 0x7C,
    QMI8658_TYPE_C = 0x79
} QMI8658_Type_t;

// 量程配置（与qmi8658a.c中实际配置一致）
#define QMI8658_ACC_RANGE       4       // ±4G
#define QMI8658_GYR_RANGE       2000    // ±2000dps

// 比例因子（LSB/单位）
#define QMI8658_ACC_SCALE_4G    8192.0f     // LSB/g
#define QMI8658_GYR_SCALE_2000DPS 16.0f     // LSB/dps
#define QMI8658_TEMP_SCALE      256.0f      // LSB/°C

// 数据结构（原始数据）
typedef struct {
    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;
    int16_t gyr_x;
    int16_t gyr_y;
    int16_t gyr_z;
} QMI8658_Data_t;

// 物理量数据结构
typedef struct {
    float acc_x_g;      // 加速度 (g)
    float acc_y_g;
    float acc_z_g;
    float gyr_x_dps;    // 角速度 (°/s)
    float gyr_y_dps;
    float gyr_z_dps;
    float temp_c;       // 温度 (°C)
} QMI8658_Physical_t;

// 校准数据结构
typedef struct {
    int16_t gyr_offset_x;  // 陀螺仪X轴偏移
    int16_t gyr_offset_y;  // 陀螺仪Y轴偏移
    int16_t gyr_offset_z;  // 陀螺仪Z轴偏移
    int16_t acc_offset_x;  // 加速度计X轴偏移
    int16_t acc_offset_y;  // 加速度计Y轴偏移
    int16_t acc_offset_z;  // 加速度计Z轴偏移
    uint8_t calibrated;    // 是否已校准
} QMI8658_Calibration_t;

// 函数声明
int  QMI8658_Init(void);
QMI8658_Type_t QMI8658_GetDeviceType(void);
int  QMI8658_ReadData(QMI8658_Data_t *data);
int  QMI8658_ReadPhysical(QMI8658_Physical_t *physical);
int  QMI8658_ReadTemperature(float *temperature);
int  QMI8658_IsDataReady(void);
int  QMI8658_WaitForDataReady(int timeout_ms);
void QMI8658_SoftReset(void);

// 数据转换函数
float QMI8658_ConvertAccToG(int16_t raw);
float QMI8658_ConvertGyroToDPS(int16_t raw);
float QMI8658_ConvertTempToC(int16_t raw);

// 校准函数
int  QMI8658_CalibrateGyro(void);
int  QMI8658_CalibrateAccel(void);
int  QMI8658_ApplyCalibration(void);
int  QMI8658_GetCalibration(QMI8658_Calibration_t *calib);
int  QMI8658_SetCalibration(QMI8658_Calibration_t *calib);

// FIFO函数
int  QMI8658_FifoInit(void);
int  QMI8658_FifoRead(QMI8658_Data_t *data, uint8_t max_count, uint8_t *actual_count);
int  QMI8658_FifoGetStatus(uint8_t *status);
int  QMI8658_FifoGetCount(uint8_t *count);
int  QMI8658_FifoReset(void);


#endif /* QMI8658A_H */
