/**
 * @file led_control.h
 * @brief LED控制模块头文件
 */

#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include "main.h"
#include "gpio.h"

/**
 * @brief LED状态枚举
 */
typedef enum {
    LED_OFF,        ///< LED关闭
    LED1_ON,        ///< LED1开启
    LED2_ON,        ///< LED2开启
    LED_ON,         ///< 所有LED开启
    LED_TOGGLE      ///< LED翻转状态
} LedState;

/* 函数声明 */
void led_init(void);
void change_led(LedState led_state);
void led_tim_set(uint32_t new_val);
uint32_t get_led_tim(void);
LedState read_current_led_state(void);

#endif /* LED_CONTROL_H */

