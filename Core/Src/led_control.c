/**
 * @file led_control.c
 * @brief LED控制模块实现
 */

#include "led_control.h"
#include "gpio.h"
#include <stdint.h>

/* 私有变量 */
static uint32_t LED_TIM = 300;      //LED闪烁时间间隔
static LedState led_state = LED_OFF;//LED状态

/**
 * @brief LED初始化函数
 */
void led_init(void)
{
    // 初始化时关闭所有LED
    change_led(LED_OFF);
}

/**
 * @brief 改变LED状态
 * @param led_state 要设置的LED状态
 */
void change_led(LedState led_state)
{
    // 更新内部状态记录
    if (led_state != LED_TOGGLE) {
        led_state = led_state;
    }
    switch (led_state)
    {
        case LED_OFF:
            HAL_GPIO_WritePin(GPIOA, LED1_Pin|LED2_Pin, GPIO_PIN_SET);
            break;
        case LED1_ON:
            HAL_GPIO_WritePin(GPIOA, LED1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOA, LED2_Pin, GPIO_PIN_SET);
            break;
        case LED2_ON:
            HAL_GPIO_WritePin(GPIOA, LED1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOA, LED2_Pin, GPIO_PIN_RESET);
            break;
        case LED_ON:
            HAL_GPIO_WritePin(GPIOA, LED1_Pin|LED2_Pin, GPIO_PIN_RESET);
            break;
        case LED_TOGGLE:
            HAL_GPIO_TogglePin(GPIOA, LED1_Pin|LED2_Pin);
            break;
    }
    
}

/**
 * @brief 设置LED闪烁时间间隔
 * @param new_val 新的时间间隔值(毫秒)
 */
void led_tim_set(uint32_t new_val)
{
    if(new_val < 50) 
        LED_TIM = 50;
    else if(new_val > 1000) 
        LED_TIM = 1000;
    else 
        LED_TIM = new_val;
}

/**
 * @brief 获取LED闪烁时间间隔
 * @return 当前LED闪烁时间间隔
 */
uint32_t get_led_tim(void)
{
    return LED_TIM;
}

/**
 * @brief 读取当前实际LED状态
 * @return 当前LED状态
 */
LedState read_current_led_state(void)
{
    uint8_t led1_state = HAL_GPIO_ReadPin(LED1_GPIO_Port, LED1_Pin);
    uint8_t led2_state = HAL_GPIO_ReadPin(LED2_GPIO_Port, LED2_Pin);
    
    // 注意：STM32中GPIO_PIN_RESET表示点亮，GPIO_PIN_SET表示熄灭
    if (led1_state == GPIO_PIN_RESET && led2_state == GPIO_PIN_RESET) {
        return LED_ON;      // 两个LED都亮
    } else if (led1_state == GPIO_PIN_RESET) {
        return LED1_ON;     // 只有LED1亮
    } else if (led2_state == GPIO_PIN_RESET) {
        return LED2_ON;     // 只有LED2亮
    } else {
        return LED_OFF;     // 两个LED都灭
    }
}
