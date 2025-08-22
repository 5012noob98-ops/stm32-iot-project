/**
 * @file key_handler.h
 * @brief 按键处理模块头文件
 */

#ifndef KEY_HANDLER_H
#define KEY_HANDLER_H

#include "main.h"

/**
 * @brief 按键状态枚举
 */
typedef enum {
    KEY_STATE_WAIT,              ///< 等待按键状态
    KEY_STATE_SHORT_PRESS,       ///< 短按状态
    KEY_STATE_LONG_PRESS,        ///< 长按状态
    KEY_STATE_DOUBLE_WAIT,       ///< 等待双击状态
    KEY_STATE_BLINKING,          ///< 闪烁状态
    KEY_STATE_CONFIG_WAIT,       ///< 等待配置状态
    KEY_STATE_CONFIG_MODE,       ///< 配置模式状态
    KEY_STATE_KEY1_ADD,          ///< KEY1增加模式
    KEY_STATE_KEY2_MINUS,        ///< KEY2减少模式
    KEY_STATE_KEY2_CONFIG,       ///< KEY2配置模式
    KEY_STATE_SAVE_CONFIG        ///< 保存配置状态
} KeyState;


/* 外部变量声明 */
extern volatile uint32_t system_tick;

/* 函数声明 */
void key_init(void);
void key_press_handler(void);
void reset_key_state(void);
KeyState get_key_state(void);

#endif /* KEY_HANDLER_H */

