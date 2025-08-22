/**
 * @file key_handler.c
 * @brief 按键处理模块实现
 */

#include "key_handler.h"
#include "led_control.h"
#include "gpio.h"
#include "tim.h"
#include <stdio.h>
#include <stdint.h>

/* 外部变量声明 */
extern volatile uint32_t system_tick;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_uart4_rx;

/* 私有变量 */
static uint32_t last_key_time = 0;
static uint32_t last_click_time = 0;
static uint8_t short_press_count = 0;
static uint8_t threshold_set = 0;
static KeyState current_key_state = KEY_STATE_WAIT;

/* 私有函数声明 */
static void state_handler_wait(void);
static void state_handler_short_press(void);
static void state_handler_long_press(void);
static void state_handler_double_wait(void);
static void state_handler_blinking(void);
static void state_handler_config_wait(void);
static void state_handler_config_mode(void);
static void state_handler_key1_add(void);
static void state_handler_key2_minus(void);
static void state_handler_key2_config(void);
static void state_handler_save_config(void);

/* 状态处理函数表 */
static void (*state_table[])(void) = {
    [KEY_STATE_WAIT]        = state_handler_wait,
    [KEY_STATE_SHORT_PRESS] = state_handler_short_press,
    [KEY_STATE_LONG_PRESS]  = state_handler_long_press,
    [KEY_STATE_DOUBLE_WAIT] = state_handler_double_wait,
    [KEY_STATE_BLINKING]    = state_handler_blinking,
    [KEY_STATE_CONFIG_WAIT] = state_handler_config_wait,
    [KEY_STATE_CONFIG_MODE] = state_handler_config_mode,
    [KEY_STATE_KEY1_ADD]    = state_handler_key1_add,
    [KEY_STATE_KEY2_MINUS]  = state_handler_key2_minus,
    [KEY_STATE_KEY2_CONFIG] = state_handler_key2_config,
    [KEY_STATE_SAVE_CONFIG] = state_handler_save_config,
};

#define TIME_AFTER(now,old,interval) ((uint32_t)((now)-(old)) >= (interval))

/**
 * @brief 按键初始化函数
 */
void key_init(void)
{
    current_key_state = KEY_STATE_WAIT;
    short_press_count = 0;
    threshold_set = 0;
}

/**
 * @brief 按键处理主函数
 */
void key_press_handler(void)
{
    state_table[current_key_state]();
}

/**
 * @brief 重置按键状态
 */
void reset_key_state(void)
{
    current_key_state = KEY_STATE_WAIT;
}

/**
 * @brief 获取当前按键状态
 * @return 当前按键状态
 */
KeyState get_key_state(void)
{
    return current_key_state;
}

/**
 * @brief 等待按键状态处理
 */
static void state_handler_wait(void)
{
    // 检查是否按键按下，记录按下时间，并进入相应按键状态
    if (HAL_GPIO_ReadPin(GPIOC, KEY1_Pin) == RESET && HAL_GPIO_ReadPin(GPIOC, KEY2_Pin) == RESET) {
        printf("按键1\n");
        last_key_time = system_tick;
        current_key_state = KEY_STATE_CONFIG_WAIT;
    }
    else if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == RESET) {
        last_key_time = system_tick;
        current_key_state = KEY_STATE_SHORT_PRESS;
    }
}

/**
 * @brief 短按状态处理
 */
static void state_handler_short_press(void)
{
    // 检查按键1是否释放
    if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == SET) {
        // 检查按键时间是否在短按范围内：>20ms 且 <2000ms
        if (TIME_AFTER(system_tick, last_key_time, 20) && 
            !TIME_AFTER(system_tick, last_key_time, 2000)) {

            short_press_count++;
            printf("cnt = %d\n", short_press_count);
            
            // 双击检测：500ms内的多次按下
            if (short_press_count >= 2) {
                current_key_state = KEY_STATE_DOUBLE_WAIT;
                printf("进入双击模式\n");
            } else {
                // 切换LED状态
                last_click_time = system_tick; // 记录本次释放时间，用于双击检测
                if (read_current_led_state() == 3)
                {
                    change_led(LED_OFF);
                }else{
                    LedState led_state = read_current_led_state();
                    LedState new_led_state = (LedState)((led_state + 1) % 3);
                    change_led(new_led_state);
                }
                current_key_state = KEY_STATE_WAIT;
                printf("切换LED,短按结束\n");
            }
        }
    }
    // 检查是否长按
    else if (TIME_AFTER(system_tick, last_key_time, 2000)) {
        current_key_state = KEY_STATE_LONG_PRESS;
    }
}

/**
 * @brief 等待双击状态处理
 */
static void state_handler_double_wait(void)
{
    // 从第一次释放开始检查是否超时300ms，超时则认为是单击
    if (TIME_AFTER(system_tick, last_click_time, 500)) {
        // 超时，返回单击模式
        last_click_time = system_tick;
        if (read_current_led_state() == 3)
        {
            change_led(LED_OFF);
        }else{
            LedState led_state = read_current_led_state();
            LedState new_led_state = (LedState)((led_state + 1) % 3);
            change_led(new_led_state);
        }
        current_key_state = KEY_STATE_WAIT;
        printf("超时，取消双击\n");
    } else {
        // 未超时，确认双击，进入闪烁状态
        short_press_count = 0;
        change_led(LED_OFF);
        current_key_state = KEY_STATE_BLINKING;
        printf("进入闪烁状态\n");
    }
}

/**
 * @brief 长按状态处理
 */
static void state_handler_long_press(void)
{
    change_led(LED_OFF);
    if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == SET) {
        printf("长按结束\n");
        current_key_state = KEY_STATE_WAIT;
    }
}

/**
 * @brief 闪烁状态处理
 */
static void state_handler_blinking(void)
{
    static uint32_t double_key_now = 0;
    static uint32_t double_key_last = 0;
    static uint8_t double_key_cnt = 0;
    double_key_now = system_tick;
    
    if (TIME_AFTER(double_key_now, double_key_last, 200)) {
        change_led(LED_TOGGLE);
        double_key_last = double_key_now;
        if (++double_key_cnt >= 6) {
            double_key_cnt = 0;
            change_led(LED_OFF);
            current_key_state = KEY_STATE_WAIT;
            printf("闪烁结束\n");
        }
    }
}

/**
 * @brief 等待配置状态处理
 */
static void state_handler_config_wait(void)
{
    // 检查双键是否仍然按下
    if ((HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == RESET) && 
        (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == RESET)) {
        
        if (TIME_AFTER(system_tick, last_key_time, 3000)) {
            // 按下3秒，进入配置模式
            change_led(LED_OFF);
            current_key_state = KEY_STATE_CONFIG_MODE;
            printf("key1和key2按下3s,进入配置模式\n");
        }
    } else {
        // 双键释放，未达到配置时间，返回初始状态
        current_key_state = KEY_STATE_WAIT;
    }
}

/**
 * @brief 配置模式状态处理
 */
static void state_handler_config_mode(void)
{
    static uint32_t config_blink_now = 0;
    static uint32_t config_blink_last = 0;
    // 用于判断是否可以进入add/minus操作的时间
    static uint32_t enter_threshold_time = 0;

    config_blink_now = system_tick;
    if (TIME_AFTER(config_blink_now, config_blink_last, get_led_tim())) {
        change_led(LED_TOGGLE);
        config_blink_last = config_blink_now;
    }

    // 初始化进入配置状态的时间，只在进入状态时执行一次
    if (!threshold_set) {
        enter_threshold_time = system_tick;
        threshold_set = 1;
    }
    
    if (TIME_AFTER(system_tick, enter_threshold_time, 2000)) {
        // 按键选择
        if (HAL_GPIO_ReadPin(GPIOC, KEY1_Pin) == RESET) {
            last_key_time = system_tick;
            current_key_state = KEY_STATE_KEY1_ADD; // 进入增加模式
        }
        else if (HAL_GPIO_ReadPin(GPIOC, KEY2_Pin) == RESET) {
            last_key_time = system_tick;
            current_key_state = KEY_STATE_KEY2_CONFIG; // 进入减少/配置模式
        }
    }
}

/**
 * @brief KEY1增加模式处理，增加LED的闪烁间隔，每一次增加50ms
 */
static void state_handler_key1_add(void)
{
    // 检查按键是否释放（短按确认）
    if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_SET) {
        if (TIME_AFTER(system_tick, last_key_time, 20)) {
            led_tim_set(get_led_tim() + 50);
            printf("LED_TIM + 50 = %lu\n", get_led_tim());
        }
        current_key_state = KEY_STATE_CONFIG_MODE;
    }
}

/**
 * @brief KEY2配置模式处理，短按进入KEY2减少模式处理，长按3s进入保存配置模式处理
 */
static void state_handler_key2_config(void)
{
    // 检查按键是否释放
    if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == SET) {
        // 短按进入减少模式
        if (TIME_AFTER(system_tick, last_key_time, 20) && !TIME_AFTER(system_tick, last_key_time, 3000)) {
            current_key_state = KEY_STATE_KEY2_MINUS;
            printf("KEY2短按,进入减少模式\n");
        }
        // 长按或超时进入保存配置模式
        else {
            current_key_state = KEY_STATE_SAVE_CONFIG;
        }
    }
    // 检查是否长按
    else {
        if (TIME_AFTER(system_tick, last_key_time, 3000)) {
            current_key_state = KEY_STATE_SAVE_CONFIG;
        }
    }
}

/**
 * @brief KEY2减少模式处理，减少LED的闪烁间隔，每一次减少50ms
 */
static void state_handler_key2_minus(void)
{
    // 确认释放时间在20 < tick < 3000 范围内，执行减少模式
    if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_SET) {
        if (TIME_AFTER(system_tick, last_key_time, 20) && 
            !TIME_AFTER(system_tick, last_key_time, 3000)) {
            led_tim_set(get_led_tim() - 50);
            printf("LED_TIM - 50 = %lu\n", get_led_tim());
        }
        current_key_state = KEY_STATE_CONFIG_MODE;
    }
}

/**
 * @brief 保存配置模式处理
 */
static void state_handler_save_config(void)
{
    if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_SET) {
        change_led(LED_OFF);
        printf("保存成功，返回等待按键状态。");
        threshold_set = 0;
        current_key_state = KEY_STATE_WAIT;
    }
}

