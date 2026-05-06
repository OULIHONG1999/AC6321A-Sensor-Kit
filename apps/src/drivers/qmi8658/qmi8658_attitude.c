/**
 * @file qmi8658_attitude.c
 * @brief QMI8658 姿态解算实现
 *
 * 包含两种方案：
 * - 方案A: Madgwick 软件滤波器（适用于 A/C 系列）
 * - 方案B: 硬件姿态引擎读取（仅适用于 C 系列）
 */

#include "qmi8658_attitude.h"
#include "qmi8658a.h"
#include "qmi8658_reg.h"
#include "../i2c/i2c_bus.h"
#include "../../board/board_pins.h"
#include <math.h>
#include <string.h>
#include <stdbool.h>

// ==================== 内部辅助函数 ====================

static int attitude_read_reg(uint8_t reg_addr, uint8_t *data) {
    int ret = i2c_bus_read_reg8(BOARD_IMU_I2C_ADDR7, reg_addr);
    if (ret < 0) {
        return ret;
    }
    *data = (uint8_t)ret;
    return 0;
}

static int attitude_read_regs(uint8_t reg_addr, uint8_t *buf, uint8_t len) {
    return i2c_bus_read_buf(BOARD_IMU_I2C_ADDR7, reg_addr, buf, len);
}

// ==================== 方案A: Madgwick 滤波器实现 ====================

// Madgwick 滤波器状态变量
static float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;  // 四元数
static float invSampleFreq = 0.01f;  // 采样周期 (秒)
static float beta = 0.04f;           // 滤波增益（基准值）
static uint8_t attitude_initialized = 0;  // 使用 uint8_t 代替 bool

// 静止检测变量
static float gyr_history_x[10] = {0}, gyr_history_y[10] = {0}, gyr_history_z[10] = {0};
static int history_index = 0;

int QMI8658_Attitude_Init(float sampleFreq, float beta_val) {
    if (sampleFreq <= 0.0f) {
        return -1;
    }

    invSampleFreq = 1.0f / sampleFreq;
    beta = beta_val;

    // 重置四元数为初始状态
    q0 = 1.0f;
    q1 = 0.0f;
    q2 = 0.0f;
    q3 = 0.0f;

    attitude_initialized = 1;
    return 0;
}

void QMI8658_Attitude_Update(float gx, float gy, float gz,
                              float ax, float ay, float az) {
    if (!attitude_initialized) {
        return;
    }

    // ========== 数据有效性检查 ==========
    // 1. 检查加速度幅值（应在 0.1g - 2.0g 之间）
    float acc_mag = sqrtf(ax * ax + ay * ay + az * az);
    if (acc_mag < 0.1f || acc_mag > 2.0f) {
        // 加速度异常，跳过本次更新，避免引入错误校正
        // printf("[ATTITUDE] Skip update: acc_mag=%.3f out of range\n", acc_mag);
        return;
    }

    // 2. 检查陀螺仪数据是否合理（超过 2000dps 视为异常）
    if (fabsf(gx) > 2000.0f || fabsf(gy) > 2000.0f || fabsf(gz) > 2000.0f) {
        // printf("[ATTITUDE] Skip update: gyro out of range\n");
        return;
    }

    // ========== 动态 Beta 调整 ==========
    // 记录最近 10 次陀螺仪数据
    gyr_history_x[history_index] = fabsf(gx);
    gyr_history_y[history_index] = fabsf(gy);
    gyr_history_z[history_index] = fabsf(gz);
    history_index = (history_index + 1) % 10;

    // 计算平均角速度
    float avg_gyro = 0;
    for (int i = 0; i < 10; i++) {
        avg_gyro += gyr_history_x[i] + gyr_history_y[i] + gyr_history_z[i];
    }
    avg_gyro /= 30.0f;  // 3轴 * 10次

    // 根据运动状态动态调整 beta
    float dynamic_beta;
    if (avg_gyro < 2.0f) {
        // 静止状态：减小 beta，更依赖陀螺仪积分，减少加速度计噪声影响
        dynamic_beta = 0.01f;
    } else if (avg_gyro < 10.0f) {
        // 轻微运动：使用基准 beta
        dynamic_beta = beta;
    } else {
        // 剧烈运动：增大 beta，更依赖加速度计快速校正
        dynamic_beta = beta * 2.0f;
    }

    float recipNorm;
    float s0, s1, s2, s3;
    float qDot1, qDot2, qDot3, qDot4;
    float _2q0, _2q1, _2q2, _2q3, _4q0, _4q1, _4q2, _8q1, _8q2, q0q0, q1q1, q2q2, q3q3;

    // 将陀螺仪数据从度/秒转换为弧度/秒
    gx *= 0.01745329251f;  // π/180
    gy *= 0.01745329251f;
    gz *= 0.01745329251f;

    // 预计算四元数相关值
    _2q0 = 2.0f * q0;
    _2q1 = 2.0f * q1;
    _2q2 = 2.0f * q2;
    _2q3 = 2.0f * q3;
    _4q0 = 4.0f * q0;
    _4q1 = 4.0f * q1;
    _4q2 = 4.0f * q2;
    _8q1 = 8.0f * q1;
    _8q2 = 8.0f * q2;
    q0q0 = q0 * q0;
    q1q1 = q1 * q1;
    q2q2 = q2 * q2;
    q3q3 = q3 * q3;

    // 梯度下降算法
    s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
    s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * q1 - _2q0 * ay - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
    s2 = 4.0f * q0q0 * q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
    s3 = 4.0f * q1q1 * q3 - _2q1 * ax + 4.0f * q2q2 * q3 - _2q2 * ay;

    // 归一化梯度
    recipNorm = 1.0f / sqrtf(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
    if (!isfinite(recipNorm)) {
        // 防止除零错误
        return;
    }
    s0 *= recipNorm;
    s1 *= recipNorm;
    s2 *= recipNorm;
    s3 *= recipNorm;

    // 计算四元数导数（使用动态 beta）
    qDot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz) - dynamic_beta * s0;
    qDot2 = 0.5f * (q0 * gx + q2 * gz - q3 * gy) - dynamic_beta * s1;
    qDot3 = 0.5f * (q0 * gy - q1 * gz + q3 * gx) - dynamic_beta * s2;
    qDot4 = 0.5f * (q0 * gz + q1 * gy - q2 * gx) - dynamic_beta * s3;

    // 积分更新四元数
    q0 += qDot1 * invSampleFreq;
    q1 += qDot2 * invSampleFreq;
    q2 += qDot3 * invSampleFreq;
    q3 += qDot4 * invSampleFreq;

    // 归一化四元数（防止漂移累积）
    recipNorm = 1.0f / sqrtf(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    if (!isfinite(recipNorm) || recipNorm == 0.0f) {
        // 如果四元数异常，重置为单位四元数
        q0 = 1.0f;
        q1 = 0.0f;
        q2 = 0.0f;
        q3 = 0.0f;
        printf("[ATTITUDE] Quaternion reset due to numerical error\n");
        return;
    }
    q0 *= recipNorm;
    q1 *= recipNorm;
    q2 *= recipNorm;
    q3 *= recipNorm;
}

void QMI8658_Attitude_GetEuler(QMI8658_Euler_t *euler) {
    if (euler == NULL || !attitude_initialized) {
        return;
    }

    // 四元数转欧拉角
    float roll, pitch, yaw;

    // Roll (横滚角)
    roll = atan2f(2.0f * (q0 * q1 + q2 * q3),
                  1.0f - 2.0f * (q1 * q1 + q2 * q2));

    // Pitch (俯仰角)
    pitch = asinf(2.0f * (q0 * q2 - q3 * q1));

    // Yaw (偏航角)
    yaw = atan2f(2.0f * (q0 * q3 + q1 * q2),
                 1.0f - 2.0f * (q2 * q2 + q3 * q3));

    // 转换为角度
    euler->roll = roll * 57.295779513f;   // 180/π
    euler->pitch = pitch * 57.295779513f;
    euler->yaw = yaw * 57.295779513f;
}

void QMI8658_Attitude_GetQuaternion(QMI8658_Quaternion_t *quat) {
    if (quat == NULL || !attitude_initialized) {
        return;
    }

    quat->w = q0;
    quat->x = q1;
    quat->y = q2;
    quat->z = q3;
}

void QMI8658_Attitude_Reset(void) {
    q0 = 1.0f;
    q1 = 0.0f;
    q2 = 0.0f;
    q3 = 0.0f;
}

// ==================== 方案B: QMI8658C 硬件姿态引擎 ====================

bool QMI8658_IsAttitudeEngineSupported(void) {
    return (QMI8658_GetDeviceType() == QMI8658_TYPE_C);
}

int QMI8658_EnableHardwareAttitude(uint8_t odr) {
    // 检查设备类型
    if (!QMI8658_IsAttitudeEngineSupported()) {
        printf("[ATTITUDE] Error: Hardware attitude engine not supported on this device\n");
        return -1;
    }

    // 参数验证
    if (odr < 1 || odr > 8) {
        printf("[ATTITUDE] Error: Invalid ODR value (%d), must be 1-8\n", odr);
        return -2;
    }

    // 检查 CTRL6 寄存器是否可读（安全防范）
    uint8_t test_val;
    int ret = attitude_read_reg(QMI8658_REG_CTRL6, &test_val);
    if (ret < 0) {
        printf("[ATTITUDE] Error: Cannot access CTRL6 register (ret=%d)\n", ret);
        return -2;
    }

    // 配置姿态引擎输出速率
    ret = i2c_bus_write_reg8(BOARD_IMU_I2C_ADDR7, QMI8658_REG_CTRL6, odr);
    if (ret < 0) {
        printf("[ATTITUDE] Error: Failed to configure attitude engine ODR\n");
        return -2;
    }

    printf("[ATTITUDE] Hardware attitude engine enabled with ODR=%d\n", odr);
    return 0;
}

int QMI8658_ReadHardwareQuaternion(QMI8658_Quaternion_t *quat) {
    if (quat == NULL) {
        return -2;
    }

    // 检查设备类型
    if (!QMI8658_IsAttitudeEngineSupported()) {
        return -1;
    }

    // 检查寄存器是否可访问（安全防范）
    uint8_t test_buf[2];
    int ret = attitude_read_regs(QMI8658_REG_DQW_L, test_buf, 2);
    if (ret < 0) {
        printf("[ATTITUDE] Error: Cannot read quaternion registers (ret=%d)\n", ret);
        return -2;
    }

    // 读取四元数数据 (8字节: W, X, Y, Z，每个16位)
    uint8_t buf[8];
    ret = attitude_read_regs(QMI8658_REG_DQW_L, buf, 8);
    if (ret < 0) {
        printf("[ATTITUDE] Error: Failed to read quaternion data\n");
        return -2;
    }

    // 检查数据有效性（全0表示无效）
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 0 &&
        buf[4] == 0 && buf[5] == 0 && buf[6] == 0 && buf[7] == 0) {
        printf("[ATTITUDE] Warning: Quaternion data is invalid (all zeros)\n");
        return -3;
    }

    // 转换为浮点数 (Q14格式: 除以 2^14 = 16384)
    int16_t qw_raw = (int16_t)((buf[1] << 8) | buf[0]);
    int16_t qx_raw = (int16_t)((buf[3] << 8) | buf[2]);
    int16_t qy_raw = (int16_t)((buf[5] << 8) | buf[4]);
    int16_t qz_raw = (int16_t)((buf[7] << 8) | buf[6]);

    quat->w = (float)qw_raw / 16384.0f;
    quat->x = (float)qx_raw / 16384.0f;
    quat->y = (float)qy_raw / 16384.0f;
    quat->z = (float)qz_raw / 16384.0f;

    return 0;
}

int QMI8658_ReadHardwareEuler(QMI8658_Euler_t *euler) {
    if (euler == NULL) {
        return -2;
    }

    // 检查设备类型
    if (!QMI8658_IsAttitudeEngineSupported()) {
        return -1;
    }

    // 先读取四元数
    QMI8658_Quaternion_t quat;
    int ret = QMI8658_ReadHardwareQuaternion(&quat);
    if (ret < 0) {
        return ret;
    }

    // 四元数转欧拉角
    float roll, pitch, yaw;

    // Roll (横滚角)
    roll = atan2f(2.0f * (quat.w * quat.x + quat.y * quat.z),
                  1.0f - 2.0f * (quat.x * quat.x + quat.y * quat.y));

    // Pitch (俯仰角)
    pitch = asinf(2.0f * (quat.w * quat.y - quat.z * quat.x));

    // Yaw (偏航角)
    yaw = atan2f(2.0f * (quat.w * quat.z + quat.x * quat.y),
                 1.0f - 2.0f * (quat.y * quat.y + quat.z * quat.z));

    // 转换为角度
    euler->roll = roll * 57.295779513f;   // 180/π
    euler->pitch = pitch * 57.295779513f;
    euler->yaw = yaw * 57.295779513f;

    return 0;
}

int QMI8658_IsHardwareAttitudeReady(void) {
    // 检查设备类型
    if (!QMI8658_IsAttitudeEngineSupported()) {
        return -1;
    }

    // 读取状态寄存器检查数据就绪
    uint8_t status;
    int ret = attitude_read_reg(QMI8658_REG_STATUS0, &status);
    if (ret < 0) {
        return -1;
    }

    // 检查姿态数据就绪位（假设 bit2 为 AE_DRDY）
    // 具体位定义需查阅数据手册确认
    return (status & 0x04) ? 1 : 0;
}

int QMI8658_DisableHardwareAttitude(void) {
    // 检查设备类型
    if (!QMI8658_IsAttitudeEngineSupported()) {
        return -1;
    }

    // 禁用姿态引擎（写入0到CTRL6）
    int ret = i2c_bus_write_reg8(BOARD_IMU_I2C_ADDR7, QMI8658_REG_CTRL6, 0x00);
    if (ret < 0) {
        printf("[ATTITUDE] Error: Failed to disable attitude engine\n");
        return -2;
    }

    printf("[ATTITUDE] Hardware attitude engine disabled\n");
    return 0;
}
