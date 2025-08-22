/**
 * @file usart_driver.c
 * @brief UART驱动模块实现
 */

#include "usart_driver.h"
#include "usart.h"
#include "dma.h"
#include "led_control.h"
#include "key_handler.h"
#include "cloud_interface.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* 外部变量 */
extern volatile uint32_t system_tick;
extern volatile uint32_t last_uart_rx_time;


/**
 * @brief USART1空闲中断处理函数
 */
void USART1_IDLEHandler(void)
{
    if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) == SET) {
        __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_IDLE);
        last_uart_rx_time = system_tick;
        
        // 1. 同步停止DMA，确保数据稳定
        HAL_UART_DMAStop(&huart1);
        
        // 2. 获取数据长度
        uint8_t rlen = RXBUF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);

        process_command((char *)rx_buf, rlen);
        // 5. 重新启动DMA接收
        HAL_UART_Receive_DMA(&huart1, rx_buf, sizeof(rx_buf));
    }
}

/**
 * @brief 处理接收到的命令
 * @param data 接收到的数据
 * @param rlen 数据长度
 */
void process_command(char *data, uint8_t rlen)
{
    char cmd[RXBUF_SIZE];
    //根据接收到数据长度，rlen，来确定接收数据的长度cmd数组的大小
    // 确保接收的数据不会超过cmd数组的大小
    if(rlen >= sizeof(cmd)) rlen = sizeof(cmd) - 1;
    memcpy(cmd, data, rlen);//将接收到的数据data复制到cmd中，长度为rlen;
    cmd[rlen] = '\0';// 在cmd末尾添加结束符。
    
    //循环去除字符串末尾的空白字符
    char *end = cmd + strlen(cmd) - 1;
    while(end >= cmd && (*end == ' ' || *end == '\r' || *end == '\n' || *end == '\t')) {
        *end = '\0';
        end--;
    }

    //strcmp数值比较函数 对比两个 \0 结尾字符串，比较结果相同返回0；
    if (strcmp(cmd, "LED ON") == 0)
    {
        change_led(LED_ON);
        reset_key_state();
    }
    else if (strcmp(cmd, "LED OFF") == 0)
    {
        change_led(LED_OFF);
        reset_key_state();
    }
    //strncmp 以指定长度字符串比较，如果找到BLINK然后使用atoi提取有效数字，修改interval来改变LED_TIM；
    else if (strncmp(cmd, "BLINK", 5) == 0)
    {
        //atoi:将字符串转换为 int 类型数值，会从字符串开头空白字符（空格、\t 等）开始识别数字 +/-符号
        // 从第一个有效数字开始转换直到遇到非数字字符停止
        uint32_t interval = atoi(cmd + 5);
        led_tim_set(interval);
        printf("BLINK interval set to %lu ms\r\n", get_led_tim());
    }
    else if (strcmp(cmd, "GET CONFIG") == 0)
    {
        printf("Current Config: LED_TIM = %lu ms,LED_State = %d\r\n", get_led_tim(), read_current_led_state());
    }  
    else{
        printf("Unknown command: %s\r\n", cmd);
    }
}

/**
 * @brief 重定向printf函数的输出到UART
 * @param ch 要输出的字符
 * @param f 文件指针
 * @return 输出的字符
 */
int fputc(int ch, FILE *f)
{
    //简单直接的UART发送，可用于调试
    HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, 100);
    return ch;
}

