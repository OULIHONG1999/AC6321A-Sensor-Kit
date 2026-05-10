# 按键事件系统说明

## 📋 概述

本文档说明杰理AC6321A传感器开发板的按键事件处理机制。

---

## 🎯 设计理念

**简单、统一、易维护**

- ✅ **示例代码简单**：只需实现一个函数
- ✅ **统一管理**：所有按键事件在一个地方处理
- ✅ **避免冲突**：不会有多个事件处理器
- ✅ **易于调试**：可以统一打印日志

---

## 🔄 工作流程

```
用户按下按键
    ↓
SDK底层检测到按键
    ↓
SDK生成系统事件 (DEVICE_EVENT_FROM_KEY)
    ↓
SDK调用 app_spp_and_le.c 中的 spple_key_event_handler()
    ↓
spple_key_event_handler() 提取按键信息
    ↓
调用 example_key_handler(key_value, event_type)
    ↓
你的示例代码处理按键逻辑
```

---

## 💻 代码实现

### 1. SDK层（app_spp_and_le.c）

```c
/**
 * @brief SDK的按键事件处理入口
 * @note 此函数由SDK的事件系统自动调用
 */
static void spple_key_event_handler(struct sys_event *event) {
    u8 event_type = 0;
    u8 key_value = 0;

    if (event->arg == (void *)DEVICE_EVENT_FROM_KEY) {
        // 提取按键信息
        event_type = event->u.key.event;   // 事件类型（短按/长按）
        key_value = event->u.key.value;    // 按键值（0-3）
        
        log_info("app_key_evnet: %d,%d\n", event_type, key_value);
        
        // 调用示例的按键处理函数
        extern void example_key_handler(u8 key_value, u8 event_type);
        example_key_handler(key_value, event_type);
    }
}
```

### 2. 示例层（example_xxx.c）

```c
/**
 * @brief 示例的按键处理函数
 * @param key_value 按键值（0=KEY1, 1=KEY2, 2=KEY3, 3=KEY4）
 * @param event_type 事件类型（KEY_EVENT_CLICK短按, KEY_EVENT_LONG长按）
 * 
 * @note 必须是全局函数（不能是static），因为会被外部调用
 */
void example_key_handler(u8 key_value, u8 event_type) {
    // 示例：KEY3长按校准传感器
    if (event_type == KEY_EVENT_LONG && key_value == 2) {
        // 执行校准
        sensor_calibrate();
    }
    
    // 示例：KEY1短按切换显示模式
    if (event_type == KEY_EVENT_CLICK && key_value == 0) {
        // 切换模式
        display_mode++;
    }
}
```

---

## 📝 按键定义

### 按键值映射

| 按键值 | 物理按键 | 常用功能 |
|-------|---------|---------|
| 0     | KEY1    | 切换模式/菜单 |
| 1     | KEY2    | 确认/选择 |
| 2     | KEY3    | 返回/校准 |
| 3     | KEY4    | 设置/重置 |

### 事件类型

| 事件类型 | 说明 | 触发条件 |
|---------|------|---------|
| `KEY_EVENT_CLICK` | 短按 | 按下后快速释放（<1秒） |
| `KEY_EVENT_LONG` | 长按 | 按下保持超过1秒 |
| `KEY_EVENT_HOLD` | 持续按住 | 长按后继续按住 |

**注意**：具体的时间阈值在SDK配置中定义。

---

## ✅ 开发规范

### 必须遵守的规则

1. **函数签名固定**
   ```c
   void example_key_handler(u8 key_value, u8 event_type);
   ```

2. **必须是全局函数**
   ```c
   // ✅ 正确
   void example_key_handler(u8 key_value, u8 event_type) { ... }
   
   // ❌ 错误 - 不能用static
   static void example_key_handler(u8 key_value, u8 event_type) { ... }
   ```

3. **不需要注册事件处理器**
   ```c
   // ❌ 错误 - 不要这样做
   SYS_EVENT_HANDLER(DEVICE_EVENT_FROM_KEY, key_event_handler, 0);
   
   // ✅ 正确 - 只需要实现函数即可
   void example_key_handler(u8 key_value, u8 event_type) { ... }
   ```

4. **可以为空实现**
   ```c
   // 如果示例不需要按键功能
   void example_key_handler(u8 key_value, u8 event_type) {
       (void)key_value;
       (void)event_type;
   }
   ```

---

## 🔧 常见用法

### 1. 区分短按和长按

```c
void example_key_handler(u8 key_value, u8 event_type) {
    if (event_type == KEY_EVENT_CLICK) {
        // 短按处理
        switch (key_value) {
            case 0: /* KEY1短按 */ break;
            case 1: /* KEY2短按 */ break;
        }
    } else if (event_type == KEY_EVENT_LONG) {
        // 长按处理
        switch (key_value) {
            case 2: /* KEY3长按 */ break;
        }
    }
}
```

### 2. 状态机模式

```c
typedef enum {
    MODE_NORMAL,
    MODE_SETTING,
    MODE_CALIBRATION
} mode_t;

static mode_t current_mode = MODE_NORMAL;

void example_key_handler(u8 key_value, u8 event_type) {
    if (event_type != KEY_EVENT_CLICK) return;
    
    switch (current_mode) {
        case MODE_NORMAL:
            if (key_value == 0) {
                current_mode = MODE_SETTING;
            }
            break;
            
        case MODE_SETTING:
            if (key_value == 0) {
                current_mode = MODE_NORMAL;
            }
            break;
    }
}
```

### 3. 防抖处理

```c
static u32 last_key_time = 0;
#define KEY_DEBOUNCE_MS 200

void example_key_handler(u8 key_value, u8 event_type) {
    u32 current_time = timer_get_ms();
    
    // 防抖：忽略短时间内的重复按键
    if (current_time - last_key_time < KEY_DEBOUNCE_MS) {
        return;
    }
    
    last_key_time = current_time;
    
    // 处理按键...
}
```

---

## 🐛 常见问题

### Q1: 按键没有响应

**可能原因**：
1. ❌ 函数定义为 `static`
2. ❌ 函数名拼写错误
3. ❌ 参数类型不匹配
4. ❌ 没有包含正确的头文件

**解决方法**：
```c
// ✅ 确保函数签名完全正确
#include "typedef.h"  // 提供 u8 类型定义

void example_key_handler(u8 key_value, u8 event_type) {
    // 你的代码
}
```

### Q2: 编译报错 "redefinition of 'example_key_handler'"

**原因**：多个示例同时被编译

**解决方法**：确保使用了条件编译宏
```c
#include "../../board/example_config.h"

#if ENABLE_EXAMPLE_QMI8658

void example_key_handler(u8 key_value, u8 event_type) {
    // ...
}

#endif
```

### Q3: 如何调试按键事件？

**方法1**：在 `app_spp_and_le.c` 中添加日志
```c
log_info("Key pressed: value=%d, type=%d\n", key_value, event_type);
```

**方法2**：在示例中添加日志
```c
void example_key_handler(u8 key_value, u8 event_type) {
    printf("Example received: key=%d, event=%d\n", key_value, event_type);
}
```

### Q4: 按键响应太慢

**原因**：主循环延迟太长

**解决方法**：
```c
// ❌ 延迟太长，按键响应慢
os_time_dly(100);  // 100ms

// ✅ 延迟适中
os_time_dly(10);   // 10ms
```

---

## 📚 相关文档

- [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md) - 完整的开发指南
- [example_qmi8658/qmi8658_example.c](example_qmi8658/qmi8658_example.c) - QMI8658示例（含按键实现）
- [app_spp_and_le.c](../../spp_and_le/examples/trans_data/app_spp_and_le.c) - SDK按键处理入口

---

## 📅 版本历史

- v1.0 (2026-05-10): 初始版本

---

*如有疑问，请参考示例代码或联系项目维护者。*
