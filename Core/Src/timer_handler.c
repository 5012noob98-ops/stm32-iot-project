/**
 * @file timer_handler.c
 * @brief ��ʱ������ģ��ʵ��
 */

#include "timer_handler.h"
#include "tim.h"
#include "cloud_interface.h"
#include <stdio.h>

/* �ⲿ���� */
extern TIM_HandleTypeDef htim6;

/* ˽�б��� */
volatile uint32_t system_tick = 0;
volatile uint32_t last_uart_rx_time = 0;

#define TIME_AFTER(now,old,interval) ((uint32_t)((now)-(old)) >= (interval))

/**
 * @brief ��ʱ�����ڻص�����
 * @param htim ��ʱ�����
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim6) {
        system_tick++;
    }
}

/**
 * @brief ��鲢��ʱ�ϱ�״̬
 */
void check_and_report_status(void)
{
    static uint32_t last_report_time = 0;
    // ÿ60���ϱ�һ��״̬��ԭ��������60000ms��
    if (TIME_AFTER(system_tick, last_report_time, 60000)) {
        report_device_status();
        last_report_time = system_tick;
    }
}

