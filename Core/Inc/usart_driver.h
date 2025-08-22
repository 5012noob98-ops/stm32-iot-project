/**
 * @file usart_driver.h
 * @brief UART����ģ��ͷ�ļ�
 */

#ifndef USART_DRIVER_H
#define USART_DRIVER_H

#include "main.h"
#include <stdio.h>

/* �궨�� */
#define RXBUF_SIZE 256

/* �ⲿ�������� */
extern uint8_t rx_buf[RXBUF_SIZE];
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_uart4_rx;

/* �������� */
void USART1_IDLEHandler(void);
void process_command(char *data, uint8_t rlen);
int fputc(int ch, FILE *f);

#endif /* USART_DRIVER_H */

