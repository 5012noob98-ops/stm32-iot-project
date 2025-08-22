/**
 * @file esp8266_driver.c
 * @brief ESP8266����ģ��ʵ��
 */

#include "esp8266_driver.h"
#include "usart.h"
#include "dma.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"


/* �ⲿ���� */
extern volatile uint32_t system_tick;
extern DMA_HandleTypeDef hdma_usart1_rx;

/* ˽�б��� */


/**
 * @brief ����AT����
 * @param cmd Ҫ���͵�����
 */
void send_cmd(uint8_t *cmd)
{
    printf("����ָ��:----> %s\r\n", cmd);
    HAL_UART_Transmit_DMA(&huart4, cmd, strlen((const char *)cmd));
    HAL_Delay(5);
}

/**
 * @brief ESP8266��ʼ��������WiFi���Ӻ�MQTT����
 */
void esp8266_init(void)
{
    printf("wifi��ʼ����ʼ\r\n");

    send_cmd((uint8_t *)AT);
    HAL_Delay(5000);

    send_cmd((uint8_t *)AT_WIFI_MODE);
    HAL_Delay(5000);

    send_cmd((uint8_t *)AT_SET_PSW);
    HAL_Delay(5000);

    send_cmd((uint8_t *)AT_SET_MQTT_CFG);
    HAL_Delay(5000);

    send_cmd((uint8_t *)AT_SET_MQTT_CLENTID);
    HAL_Delay(5000);

    send_cmd((uint8_t *)AT_SET_MQTT_CONN);
    HAL_Delay(5000);

    // ����������⣨���ڽ������
    send_cmd((uint8_t *)AT_MQTT_SUB_PROPERTY_SET);
    HAL_Delay(5000);

    // ���Ļ�ȡ��������
    send_cmd((uint8_t *)AT_MQTT_SUB_PROPERTY_GET);
    HAL_Delay(1000);

    printf("wifi��ʼ�����\r\n");
}

/**
 * @brief �����Ի��Ƶ�����ͺ���
 * @param cmd Ҫ���͵�����
 * @param retries ���Դ���
 * @return 0��ʾ�ɹ���-1��ʾʧ��
 */
int send_cmd_with_retry(uint8_t *cmd, uint8_t retries)
{
    for (uint8_t i = 0; i < retries; i++) {
        printf("���Դ��� %d: ", i+1);
        send_cmd(cmd);
        HAL_Delay(3000);
        if (wifi_rx_state) {
            wifi_rx_state = false;
            if (strstr((char*)uart4_wifi_dma_rx_buf, "+MQTTCONNECTED") || 
                strstr((char*)uart4_wifi_dma_rx_buf, "+MQTTSUB") ||
                strstr((char*)uart4_wifi_dma_rx_buf, "SEND OK") ||
                strstr((char*)uart4_wifi_dma_rx_buf, "OK")) {
                memset(uart4_wifi_dma_rx_buf, 0, sizeof(uart4_wifi_dma_rx_buf));
                return 0; // �ɹ�
            }
        }
    }
    memset(uart4_wifi_dma_rx_buf, 0, sizeof(uart4_wifi_dma_rx_buf));
    return -1; // ʧ��
}

/**
 * @brief ����ESP8266��Ӧ����
 * @param data ���յ�������
 * @param rlen ���ݳ���
 */
void process_esp8266_response(char *data, uint8_t rlen)
{
    // ��������
    char clean_data[RXBUF_SIZE + 1];
    memcpy(clean_data, data, rlen);
    clean_data[rlen] = '\0';
    
    printf("ESP8266 ��Ӧ���ݽ��յ�: %s\r\n", clean_data);

    // ���WiFi����״̬
    if (strstr(clean_data, "WIFI CONNECTED") || strstr(clean_data, "WIFI GOT IP")) {
        printf("WIFI���ӳɹ�\r\n");
    }
    // ���MQTT������Ӧ
    else if (strstr(clean_data, "+MQTTCONNECTED")) {
        printf("MQTT���ӳɹ�\r\n");
    }
    // ���AT������Ӧ
    else if (strstr(clean_data, "OK")) {
        printf("ATָ����ɹ�\r\n");
    } else if (strstr(clean_data, "ERROR")) {
        if (strstr(clean_data, "+MQTTCONNECTED") || strstr(clean_data, "+MQTTSUB")) {
            printf("ATָ���ERROR,�������ѳɹ�\r\n");
        } else {
            printf("ATָ���ʧ��\r\n");
        }
    }
    // ���MQTT������Ϣ
    else if (strstr(clean_data, "+MQTTRCV") != NULL || strstr(clean_data, "+MQTTSUBRECV") != NULL) {
        // ��ȡMQTT��Ϣ����
        char *payload_start = strchr(clean_data, '{');
        if (payload_start != NULL) {
            // �ҵ�JSON����λ��
            char *json_end = strrchr(payload_start, '}');
            if (json_end != NULL) {
                *(json_end + 1) = '\0'; // �ض��ַ���
                
                printf("MQTT Payload : %s \r\n", payload_start);
                // ����JSON���������Ҫ�ⲿ������
                parse_json_str_command(payload_start);
            }
        }
    }
    // MQTT���ͳɹ���Ӧ
    else if (strstr(clean_data, "SEND OK")) {
        printf("MQTT��Ϣ���ͳɹ�\r\n");
    } else if (strstr(clean_data, "+MQTTPUB:")) {
        printf("MQTT������Ӧ\r\n");
    } else {
        printf("û�ж�Ӧ�����ݴ������յ�ԭʼ����: %s\r\n", clean_data);
    }
    
    // ��������
    memset(uart4_wifi_dma_rx_buf, 0, sizeof(uart4_wifi_dma_rx_buf));
}

/**
 * @brief UART4�����жϴ�����
 */
void UART4_IDLEHandler(void)
{
    if (__HAL_UART_GET_FLAG(&huart4, UART_FLAG_IDLE) == SET) {
        __HAL_UART_CLEAR_FLAG(&huart4, UART_FLAG_IDLE);
        
        // 1. ͬ��ֹͣDMA��ȷ�������ȶ�
        HAL_UART_DMAStop(&huart4);
        
        // 2. ��ȡ���ݳ���
        uint8_t rlen = RXBUF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_uart4_rx);

        wifi_rx_state = true;
        memcpy(uart4_wifi_dma_rx_buf, rx4_buf, rlen);
        process_esp8266_response((char *)uart4_wifi_dma_rx_buf, rlen);
        
        // 5. ��������DMA����
        HAL_UART_Receive_DMA(&huart4, rx4_buf, sizeof(rx4_buf));
    }
}

