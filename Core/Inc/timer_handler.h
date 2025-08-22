/**
 * @file timer_handler.h
 * @brief 定时器处理模块头文件
 */

#ifndef TIMER_HANDLER_H
#define TIMER_HANDLER_H

#include "main.h"

/* 外部变量声明 */
extern volatile uint32_t system_tick;
extern volatile uint32_t last_uart_rx_time;

/* 函数声明 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void check_and_report_status(void);

#endif /* TIMER_HANDLER_H */

