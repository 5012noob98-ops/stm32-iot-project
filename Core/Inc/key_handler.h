/**
 * @file key_handler.h
 * @brief ��������ģ��ͷ�ļ�
 */

#ifndef KEY_HANDLER_H
#define KEY_HANDLER_H

#include "main.h"

/**
 * @brief ����״̬ö��
 */
typedef enum {
    KEY_STATE_WAIT,              ///< �ȴ�����״̬
    KEY_STATE_SHORT_PRESS,       ///< �̰�״̬
    KEY_STATE_LONG_PRESS,        ///< ����״̬
    KEY_STATE_DOUBLE_WAIT,       ///< �ȴ�˫��״̬
    KEY_STATE_BLINKING,          ///< ��˸״̬
    KEY_STATE_CONFIG_WAIT,       ///< �ȴ�����״̬
    KEY_STATE_CONFIG_MODE,       ///< ����ģʽ״̬
    KEY_STATE_KEY1_ADD,          ///< KEY1����ģʽ
    KEY_STATE_KEY2_MINUS,        ///< KEY2����ģʽ
    KEY_STATE_KEY2_CONFIG,       ///< KEY2����ģʽ
    KEY_STATE_SAVE_CONFIG        ///< ��������״̬
} KeyState;


/* �ⲿ�������� */
extern volatile uint32_t system_tick;

/* �������� */
void key_init(void);
void key_press_handler(void);
void reset_key_state(void);
KeyState get_key_state(void);

#endif /* KEY_HANDLER_H */

