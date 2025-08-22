/**
 * @file led_control.h
 * @brief LED����ģ��ͷ�ļ�
 */

#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include "main.h"
#include "gpio.h"

/**
 * @brief LED״̬ö��
 */
typedef enum {
    LED_OFF,        ///< LED�ر�
    LED1_ON,        ///< LED1����
    LED2_ON,        ///< LED2����
    LED_ON,         ///< ����LED����
    LED_TOGGLE      ///< LED��ת״̬
} LedState;

/* �������� */
void led_init(void);
void change_led(LedState led_state);
void led_tim_set(uint32_t new_val);
uint32_t get_led_tim(void);
LedState read_current_led_state(void);

#endif /* LED_CONTROL_H */

