/**
 * @file key_handler.c
 * @brief ��������ģ��ʵ��
 */

#include "key_handler.h"
#include "led_control.h"
#include "gpio.h"
#include "tim.h"
#include <stdio.h>
#include <stdint.h>

/* �ⲿ�������� */
extern volatile uint32_t system_tick;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_uart4_rx;

/* ˽�б��� */
static uint32_t last_key_time = 0;
static uint32_t last_click_time = 0;
static uint8_t short_press_count = 0;
static uint8_t threshold_set = 0;
static KeyState current_key_state = KEY_STATE_WAIT;

/* ˽�к������� */
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

/* ״̬�������� */
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
 * @brief ������ʼ������
 */
void key_init(void)
{
    current_key_state = KEY_STATE_WAIT;
    short_press_count = 0;
    threshold_set = 0;
}

/**
 * @brief ��������������
 */
void key_press_handler(void)
{
    state_table[current_key_state]();
}

/**
 * @brief ���ð���״̬
 */
void reset_key_state(void)
{
    current_key_state = KEY_STATE_WAIT;
}

/**
 * @brief ��ȡ��ǰ����״̬
 * @return ��ǰ����״̬
 */
KeyState get_key_state(void)
{
    return current_key_state;
}

/**
 * @brief �ȴ�����״̬����
 */
static void state_handler_wait(void)
{
    // ����Ƿ񰴼����£���¼����ʱ�䣬��������Ӧ����״̬
    if (HAL_GPIO_ReadPin(GPIOC, KEY1_Pin) == RESET && HAL_GPIO_ReadPin(GPIOC, KEY2_Pin) == RESET) {
        printf("����1\n");
        last_key_time = system_tick;
        current_key_state = KEY_STATE_CONFIG_WAIT;
    }
    else if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == RESET) {
        last_key_time = system_tick;
        current_key_state = KEY_STATE_SHORT_PRESS;
    }
}

/**
 * @brief �̰�״̬����
 */
static void state_handler_short_press(void)
{
    // ��鰴��1�Ƿ��ͷ�
    if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == SET) {
        // ��鰴��ʱ���Ƿ��ڶ̰���Χ�ڣ�>20ms �� <2000ms
        if (TIME_AFTER(system_tick, last_key_time, 20) && 
            !TIME_AFTER(system_tick, last_key_time, 2000)) {

            short_press_count++;
            printf("cnt = %d\n", short_press_count);
            
            // ˫����⣺500ms�ڵĶ�ΰ���
            if (short_press_count >= 2) {
                current_key_state = KEY_STATE_DOUBLE_WAIT;
                printf("����˫��ģʽ\n");
            } else {
                // �л�LED״̬
                last_click_time = system_tick; // ��¼�����ͷ�ʱ�䣬����˫�����
                if (read_current_led_state() == 3)
                {
                    change_led(LED_OFF);
                }else{
                    LedState led_state = read_current_led_state();
                    LedState new_led_state = (LedState)((led_state + 1) % 3);
                    change_led(new_led_state);
                }
                current_key_state = KEY_STATE_WAIT;
                printf("�л�LED,�̰�����\n");
            }
        }
    }
    // ����Ƿ񳤰�
    else if (TIME_AFTER(system_tick, last_key_time, 2000)) {
        current_key_state = KEY_STATE_LONG_PRESS;
    }
}

/**
 * @brief �ȴ�˫��״̬����
 */
static void state_handler_double_wait(void)
{
    // �ӵ�һ���ͷſ�ʼ����Ƿ�ʱ300ms����ʱ����Ϊ�ǵ���
    if (TIME_AFTER(system_tick, last_click_time, 500)) {
        // ��ʱ�����ص���ģʽ
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
        printf("��ʱ��ȡ��˫��\n");
    } else {
        // δ��ʱ��ȷ��˫����������˸״̬
        short_press_count = 0;
        change_led(LED_OFF);
        current_key_state = KEY_STATE_BLINKING;
        printf("������˸״̬\n");
    }
}

/**
 * @brief ����״̬����
 */
static void state_handler_long_press(void)
{
    change_led(LED_OFF);
    if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == SET) {
        printf("��������\n");
        current_key_state = KEY_STATE_WAIT;
    }
}

/**
 * @brief ��˸״̬����
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
            printf("��˸����\n");
        }
    }
}

/**
 * @brief �ȴ�����״̬����
 */
static void state_handler_config_wait(void)
{
    // ���˫���Ƿ���Ȼ����
    if ((HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == RESET) && 
        (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == RESET)) {
        
        if (TIME_AFTER(system_tick, last_key_time, 3000)) {
            // ����3�룬��������ģʽ
            change_led(LED_OFF);
            current_key_state = KEY_STATE_CONFIG_MODE;
            printf("key1��key2����3s,��������ģʽ\n");
        }
    } else {
        // ˫���ͷţ�δ�ﵽ����ʱ�䣬���س�ʼ״̬
        current_key_state = KEY_STATE_WAIT;
    }
}

/**
 * @brief ����ģʽ״̬����
 */
static void state_handler_config_mode(void)
{
    static uint32_t config_blink_now = 0;
    static uint32_t config_blink_last = 0;
    // �����ж��Ƿ���Խ���add/minus������ʱ��
    static uint32_t enter_threshold_time = 0;

    config_blink_now = system_tick;
    if (TIME_AFTER(config_blink_now, config_blink_last, get_led_tim())) {
        change_led(LED_TOGGLE);
        config_blink_last = config_blink_now;
    }

    // ��ʼ����������״̬��ʱ�䣬ֻ�ڽ���״̬ʱִ��һ��
    if (!threshold_set) {
        enter_threshold_time = system_tick;
        threshold_set = 1;
    }
    
    if (TIME_AFTER(system_tick, enter_threshold_time, 2000)) {
        // ����ѡ��
        if (HAL_GPIO_ReadPin(GPIOC, KEY1_Pin) == RESET) {
            last_key_time = system_tick;
            current_key_state = KEY_STATE_KEY1_ADD; // ��������ģʽ
        }
        else if (HAL_GPIO_ReadPin(GPIOC, KEY2_Pin) == RESET) {
            last_key_time = system_tick;
            current_key_state = KEY_STATE_KEY2_CONFIG; // �������/����ģʽ
        }
    }
}

/**
 * @brief KEY1����ģʽ��������LED����˸�����ÿһ������50ms
 */
static void state_handler_key1_add(void)
{
    // ��鰴���Ƿ��ͷţ��̰�ȷ�ϣ�
    if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_SET) {
        if (TIME_AFTER(system_tick, last_key_time, 20)) {
            led_tim_set(get_led_tim() + 50);
            printf("LED_TIM + 50 = %lu\n", get_led_tim());
        }
        current_key_state = KEY_STATE_CONFIG_MODE;
    }
}

/**
 * @brief KEY2����ģʽ�����̰�����KEY2����ģʽ��������3s���뱣������ģʽ����
 */
static void state_handler_key2_config(void)
{
    // ��鰴���Ƿ��ͷ�
    if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == SET) {
        // �̰��������ģʽ
        if (TIME_AFTER(system_tick, last_key_time, 20) && !TIME_AFTER(system_tick, last_key_time, 3000)) {
            current_key_state = KEY_STATE_KEY2_MINUS;
            printf("KEY2�̰�,�������ģʽ\n");
        }
        // ������ʱ���뱣������ģʽ
        else {
            current_key_state = KEY_STATE_SAVE_CONFIG;
        }
    }
    // ����Ƿ񳤰�
    else {
        if (TIME_AFTER(system_tick, last_key_time, 3000)) {
            current_key_state = KEY_STATE_SAVE_CONFIG;
        }
    }
}

/**
 * @brief KEY2����ģʽ��������LED����˸�����ÿһ�μ���50ms
 */
static void state_handler_key2_minus(void)
{
    // ȷ���ͷ�ʱ����20 < tick < 3000 ��Χ�ڣ�ִ�м���ģʽ
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
 * @brief ��������ģʽ����
 */
static void state_handler_save_config(void)
{
    if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_SET) {
        change_led(LED_OFF);
        printf("����ɹ������صȴ�����״̬��");
        threshold_set = 0;
        current_key_state = KEY_STATE_WAIT;
    }
}

