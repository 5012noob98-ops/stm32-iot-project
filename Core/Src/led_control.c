/**
 * @file led_control.c
 * @brief LED����ģ��ʵ��
 */

#include "led_control.h"
#include "gpio.h"
#include <stdint.h>

/* ˽�б��� */
static uint32_t LED_TIM = 300;      //LED��˸ʱ����
static LedState led_state = LED_OFF;//LED״̬

/**
 * @brief LED��ʼ������
 */
void led_init(void)
{
    // ��ʼ��ʱ�ر�����LED
    change_led(LED_OFF);
}

/**
 * @brief �ı�LED״̬
 * @param led_state Ҫ���õ�LED״̬
 */
void change_led(LedState led_state)
{
    // �����ڲ�״̬��¼
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
 * @brief ����LED��˸ʱ����
 * @param new_val �µ�ʱ����ֵ(����)
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
 * @brief ��ȡLED��˸ʱ����
 * @return ��ǰLED��˸ʱ����
 */
uint32_t get_led_tim(void)
{
    return LED_TIM;
}

/**
 * @brief ��ȡ��ǰʵ��LED״̬
 * @return ��ǰLED״̬
 */
LedState read_current_led_state(void)
{
    uint8_t led1_state = HAL_GPIO_ReadPin(LED1_GPIO_Port, LED1_Pin);
    uint8_t led2_state = HAL_GPIO_ReadPin(LED2_GPIO_Port, LED2_Pin);
    
    // ע�⣺STM32��GPIO_PIN_RESET��ʾ������GPIO_PIN_SET��ʾϨ��
    if (led1_state == GPIO_PIN_RESET && led2_state == GPIO_PIN_RESET) {
        return LED_ON;      // ����LED����
    } else if (led1_state == GPIO_PIN_RESET) {
        return LED1_ON;     // ֻ��LED1��
    } else if (led2_state == GPIO_PIN_RESET) {
        return LED2_ON;     // ֻ��LED2��
    } else {
        return LED_OFF;     // ����LED����
    }
}
