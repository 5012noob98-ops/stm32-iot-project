/**
 * @file timer_handler.h
 * @brief ��ʱ������ģ��ͷ�ļ�
 */

#ifndef TIMER_HANDLER_H
#define TIMER_HANDLER_H

#include "main.h"

/* �ⲿ�������� */
extern volatile uint32_t system_tick;
extern volatile uint32_t last_uart_rx_time;

/* �������� */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void check_and_report_status(void);

#endif /* TIMER_HANDLER_H */

