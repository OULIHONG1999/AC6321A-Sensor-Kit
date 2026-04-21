#ifndef __QMI8658_REG_H
#define __QMI8658_REG_H

#ifdef __cplusplus
extern "C" {
#endif

/************************** 设备信息 **************************/
#define QMI8658_WHO_AM_I_VAL        0x05    // 器件ID
#define QMI8658A_REVISION_ID        0x7C    // QMI8658A版本ID
#define QMI8658C_REVISION_ID        0x79    // QMI8658C版本ID

/************************** 寄存器地址定义 **************************/
// 通用寄存器
#define QMI8658_REG_WHO_AM_I        0x00    // 器件ID寄存器
#define QMI8658_REG_REVISION_ID      0x01    // 版本ID寄存器

// 控制配置寄存器
#define QMI8658_REG_CTRL1           0x02    // 串行接口&传感器使能
#define QMI8658_REG_CTRL2           0x03    // 加速度计配置
#define QMI8658_REG_CTRL3           0x04    // 陀螺仪配置
#define QMI8658_REG_CTRL5           0x06    // 低通滤波配置
#define QMI8658_REG_CTRL6           0x07    // QMI8658C专用：姿态引擎配置
#define QMI8658_REG_CTRL7           0x08    // 传感器使能控制
#define QMI8658_REG_CTRL8           0x09    // 运动检测控制
#define QMI8658_REG_CTRL9           0x0A    // 主机命令寄存器

// 校准寄存器
#define QMI8658_REG_CAL1_L          0x0B    // 校准数据1低字节
#define QMI8658_REG_CAL1_H          0x0C    // 校准数据1高字节
#define QMI8658_REG_CAL2_L          0x0D    // 校准数据2低字节
#define QMI8658_REG_CAL2_H          0x0E    // 校准数据2高字节
#define QMI8658_REG_CAL3_L          0x0F    // 校准数据3低字节
#define QMI8658_REG_CAL3_H          0x10    // 校准数据3高字节
#define QMI8658_REG_CAL4_L          0x11    // 校准数据4低字节
#define QMI8658_REG_CAL4_H          0x12    // 校准数据4高字节

// FIFO寄存器
#define QMI8658_REG_FIFO_WTM_TH     0x13    // FIFO水印阈值
#define QMI8658_REG_FIFO_CTRL       0x14    // FIFO控制
#define QMI8658_REG_FIFO_SMPL_CNT   0x15    // FIFO采样计数
#define QMI8658_REG_FIFO_STATUS     0x16    // FIFO状态
#define QMI8658_REG_FIFO_DATA       0x17    // FIFO数据

// 状态&时间戳寄存器
#define QMI8658_REG_STATUSINT       0x2D    // 传感器数据&锁定状态
#define QMI8658_REG_STATUS0         0x2E    // 数据输出状态
#define QMI8658_REG_STATUS1         0x2F    // 多功能状态
#define QMI8658_REG_TIMESTAMP_L      0x30    // 时间戳低字节
#define QMI8658_REG_TIMESTAMP_MID    0x31    // 时间戳中字节
#define QMI8658_REG_TIMESTAMP_H      0x32    // 时间戳高字节

// 传感器数据输出寄存器
#define QMI8658_REG_TEMP_L          0x33    // 温度低字节
#define QMI8658_REG_TEMP_H          0x34    // 温度高字节
#define QMI8658_REG_AX_L            0x35    // X轴加速度低字节
#define QMI8658_REG_AX_H            0x36    // X轴加速度高字节
#define QMI8658_REG_AY_L            0x37    // Y轴加速度低字节
#define QMI8658_REG_AY_H            0x38    // Y轴加速度高字节
#define QMI8658_REG_AZ_L            0x39    // Z轴加速度低字节
#define QMI8658_REG_AZ_H            0x3A    // Z轴加速度高字节
#define QMI8658_REG_GX_L            0x3B    // X轴陀螺仪低字节
#define QMI8658_REG_GX_H            0x3C    // X轴陀螺仪高字节
#define QMI8658_REG_GY_L            0x3D    // Y轴陀螺仪低字节
#define QMI8658_REG_GY_H            0x3E    // Y轴陀螺仪高字节
#define QMI8658_REG_GZ_L            0x3F    // Z轴陀螺仪低字节
#define QMI8658_REG_GZ_H            0x40    // Z轴陀螺仪高字节

// 校准状态寄存器
#define QMI8658_REG_COD_STATUS      0x46    // 按需校准状态

// QMI8658C专用：姿态引擎数据寄存器
#define QMI8658_REG_DQW_L           0x49    // 四元数W低字节
#define QMI8658_REG_DQW_H           0x4A    // 四元数W高字节
#define QMI8658_REG_DQX_L           0x4B    // 四元数X低字节
#define QMI8658_REG_DQX_H           0x4C    // 四元数X高字节
#define QMI8658_REG_DQY_L           0x4D    // 四元数Y低字节
#define QMI8658_REG_DQY_H           0x4E    // 四元数Y高字节
#define QMI8658_REG_DQZ_L           0x4F    // 四元数Z低字节
#define QMI8658_REG_DQZ_H           0x50    // 四元数Z高字节
#define QMI8658_REG_DVX_L           0x51    // X轴速度增量低字节
#define QMI8658_REG_DVX_H           0x52    // X轴速度增量高字节
#define QMI8658_REG_DVY_L           0x53    // Y轴速度增量低字节
#define QMI8658_REG_DVY_H           0x54    // Y轴速度增量高字节
#define QMI8658_REG_DVZ_L           0x55    // Z轴速度增量低字节
#define QMI8658_REG_DVZ_H           0x56    // Z轴速度增量高字节
#define QMI8658_REG_AE_REG1         0x57    // 姿态引擎寄存器1
#define QMI8658_REG_AE_REG2         0x58    // 姿态引擎寄存器2

// 活动检测输出寄存器
#define QMI8658_REG_TAP_STATUS      0x59    // 敲击检测状态
#define QMI8658_REG_STEP_CNT_LOW    0x5A    // 计步器计数低字节
#define QMI8658_REG_STEP_CNT_MID    0x5B    // 计步器计数中字节
#define QMI8658_REG_STEP_CNT_HIGH   0x5C    // 计步器计数高字节

// 复位寄存器
#define QMI8658_REG_RESET           0x60    // 软件复位寄存器
#define QMI8658_SOFT_RESET_VAL      0xB0    // 软件复位命令值

/************************** CTRL9命令定义 **************************/
#define QMI8658_CMD_ACK             0x00    // 命令应答
#define QMI8658_CMD_RST_FIFO        0x04    // 复位FIFO
#define QMI8658_CMD_REQ_FIFO        0x05    // 请求FIFO数据
#define QMI8658_CMD_SET_WOM         0x08    // 设置运动唤醒
#define QMI8658_CMD_ACC_OFFSET      0x09    // 设置加速度计偏移
#define QMI8658_CMD_GYR_OFFSET      0x0A    // 设置陀螺仪偏移
#define QMI8658_CMD_CFG_TAP         0x0C    // 配置敲击检测
#define QMI8658_CMD_CFG_PEDO        0x0D    // 配置计步器
#define QMI8658_CMD_CFG_MOTION      0x0E    // 配置运动检测
#define QMI8658_CMD_RST_PEDO        0x0F    // 复位计步器
#define QMI8658_CMD_COPY_USID       0x10    // 复制器件唯一ID
#define QMI8658_CMD_SET_RPU         0x11    // 配置内部上拉电阻
#define QMI8658_CMD_AHB_GATE        0x12    // AHB时钟门控
#define QMI8658_CMD_COD_CALIB       0xA2    // 按需校准
#define QMI8658_CMD_APPLY_GYR_GAIN   0xAA    // 应用陀螺仪增益

/************************** 通信地址定义 **************************/
#define QMI8658_I2C_ADDR_H         0x6A    // SA0=1时I2C地址
#define QMI8658_I2C_ADDR_L         0x6B    // SA0=0时I2C地址

#ifdef __cplusplus
}
#endif

#endif