/**
 * @file usart_driver.c
 * @brief UART����ģ��ʵ��
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

/* �ⲿ���� */
extern volatile uint32_t system_tick;
extern volatile uint32_t last_uart_rx_time;


/**
 * @brief USART1�����жϴ�����
 */
void USART1_IDLEHandler(void)
{
    if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) == SET) {
        __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_IDLE);
        last_uart_rx_time = system_tick;
        
        // 1. ͬ��ֹͣDMA��ȷ�������ȶ�
        HAL_UART_DMAStop(&huart1);
        
        // 2. ��ȡ���ݳ���
        uint8_t rlen = RXBUF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);

        process_command((char *)rx_buf, rlen);
        // 5. ��������DMA����
        HAL_UART_Receive_DMA(&huart1, rx_buf, sizeof(rx_buf));
    }
}

/**
 * @brief ������յ�������
 * @param data ���յ�������
 * @param rlen ���ݳ���
 */
void process_command(char *data, uint8_t rlen)
{
    char cmd[RXBUF_SIZE];
    //���ݽ��յ����ݳ��ȣ�rlen����ȷ���������ݵĳ���cmd����Ĵ�С
    // ȷ�����յ����ݲ��ᳬ��cmd����Ĵ�С
    if(rlen >= sizeof(cmd)) rlen = sizeof(cmd) - 1;
    memcpy(cmd, data, rlen);//�����յ�������data���Ƶ�cmd�У�����Ϊrlen;
    cmd[rlen] = '\0';// ��cmdĩβ��ӽ�������
    
    //ѭ��ȥ���ַ���ĩβ�Ŀհ��ַ�
    char *end = cmd + strlen(cmd) - 1;
    while(end >= cmd && (*end == ' ' || *end == '\r' || *end == '\n' || *end == '\t')) {
        *end = '\0';
        end--;
    }

    //strcmp��ֵ�ȽϺ��� �Ա����� \0 ��β�ַ������ȽϽ����ͬ����0��
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
    //strncmp ��ָ�������ַ����Ƚϣ�����ҵ�BLINKȻ��ʹ��atoi��ȡ��Ч���֣��޸�interval���ı�LED_TIM��
    else if (strncmp(cmd, "BLINK", 5) == 0)
    {
        //atoi:���ַ���ת��Ϊ int ������ֵ������ַ�����ͷ�հ��ַ����ո�\t �ȣ���ʼʶ������ +/-����
        // �ӵ�һ����Ч���ֿ�ʼת��ֱ�������������ַ�ֹͣ
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
 * @brief �ض���printf�����������UART
 * @param ch Ҫ������ַ�
 * @param f �ļ�ָ��
 * @return ������ַ�
 */
int fputc(int ch, FILE *f)
{
    //��ֱ�ӵ�UART���ͣ������ڵ���
    HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, 100);
    return ch;
}

