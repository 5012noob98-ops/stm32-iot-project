/**
 * @file timer_handler.c
 * @brief 定时器处理模块实现
 */

#include "timer_handler.h"
#include "tim.h"
#include "cloud_interface.h"
#include <stdio.h>

/* 外部变量 */
extern TIM_HandleTypeDef htim6;

/* 私有变量 */
volatile uint32_t system_tick = 0;
volatile uint32_t last_uart_rx_time = 0;

#define TIME_AFTER(now,old,interval) ((uint32_t)((now)-(old)) >= (interval))

/**
 * @brief 定时器周期回调函数
 * @param htim 定时器句柄
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim6) {
        system_tick++;
    }
}

/**
 * @brief 检查并定时上报状态
 */
void check_and_report_status(void)
{
    static uint32_t last_report_time = 0;
    // 每60秒上报一次状态（原代码中是60000ms）
    if (TIME_AFTER(system_tick, last_report_time, 60000)) {
        report_device_status();
        last_report_time = system_tick;
    }
}

