/**
 * @file esp8266_driver.c
 * @brief ESP8266驱动模块实现
 */

#include "esp8266_driver.h"
#include "usart.h"
#include "dma.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"


/* 外部变量 */
extern volatile uint32_t system_tick;
extern DMA_HandleTypeDef hdma_usart1_rx;

/* 私有变量 */


/**
 * @brief 发送AT命令
 * @param cmd 要发送的命令
 */
void send_cmd(uint8_t *cmd)
{
    printf("发送指令:----> %s\r\n", cmd);
    HAL_UART_Transmit_DMA(&huart4, cmd, strlen((const char *)cmd));
    HAL_Delay(5);
}

/**
 * @brief ESP8266初始化，包括WiFi连接和MQTT连接
 */
void esp8266_init(void)
{
    printf("wifi初始化开始\r\n");

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

    // 订阅相关主题（用于接收命令）
    send_cmd((uint8_t *)AT_MQTT_SUB_PROPERTY_SET);
    HAL_Delay(5000);

    // 订阅获取属性主题
    send_cmd((uint8_t *)AT_MQTT_SUB_PROPERTY_GET);
    HAL_Delay(1000);

    printf("wifi初始化完成\r\n");
}

/**
 * @brief 带重试机制的命令发送函数
 * @param cmd 要发送的命令
 * @param retries 重试次数
 * @return 0表示成功，-1表示失败
 */
int send_cmd_with_retry(uint8_t *cmd, uint8_t retries)
{
    for (uint8_t i = 0; i < retries; i++) {
        printf("重试次数 %d: ", i+1);
        send_cmd(cmd);
        HAL_Delay(3000);
        if (wifi_rx_state) {
            wifi_rx_state = false;
            if (strstr((char*)uart4_wifi_dma_rx_buf, "+MQTTCONNECTED") || 
                strstr((char*)uart4_wifi_dma_rx_buf, "+MQTTSUB") ||
                strstr((char*)uart4_wifi_dma_rx_buf, "SEND OK") ||
                strstr((char*)uart4_wifi_dma_rx_buf, "OK")) {
                memset(uart4_wifi_dma_rx_buf, 0, sizeof(uart4_wifi_dma_rx_buf));
                return 0; // 成功
            }
        }
    }
    memset(uart4_wifi_dma_rx_buf, 0, sizeof(uart4_wifi_dma_rx_buf));
    return -1; // 失败
}

/**
 * @brief 处理ESP8266响应数据
 * @param data 接收到的数据
 * @param rlen 数据长度
 */
void process_esp8266_response(char *data, uint8_t rlen)
{
    // 数据清理
    char clean_data[RXBUF_SIZE + 1];
    memcpy(clean_data, data, rlen);
    clean_data[rlen] = '\0';
    
    printf("ESP8266 响应数据接收到: %s\r\n", clean_data);

    // 检查WiFi连接状态
    if (strstr(clean_data, "WIFI CONNECTED") || strstr(clean_data, "WIFI GOT IP")) {
        printf("WIFI连接成功\r\n");
    }
    // 检查MQTT连接响应
    else if (strstr(clean_data, "+MQTTCONNECTED")) {
        printf("MQTT连接成功\r\n");
    }
    // 检查AT命令响应
    else if (strstr(clean_data, "OK")) {
        printf("AT指令交互成功\r\n");
    } else if (strstr(clean_data, "ERROR")) {
        if (strstr(clean_data, "+MQTTCONNECTED") || strstr(clean_data, "+MQTTSUB")) {
            printf("AT指令返回ERROR,但连接已成功\r\n");
        } else {
            printf("AT指令交互失败\r\n");
        }
    }
    // 检查MQTT接收信息
    else if (strstr(clean_data, "+MQTTRCV") != NULL || strstr(clean_data, "+MQTTSUBRECV") != NULL) {
        // 获取MQTT信息内容
        char *payload_start = strchr(clean_data, '{');
        if (payload_start != NULL) {
            // 找到JSON结束位置
            char *json_end = strrchr(payload_start, '}');
            if (json_end != NULL) {
                *(json_end + 1) = '\0'; // 截断字符串
                
                printf("MQTT Payload : %s \r\n", payload_start);
                // 解析JSON命令（这里需要外部函数）
                parse_json_str_command(payload_start);
            }
        }
    }
    // MQTT发送成功响应
    else if (strstr(clean_data, "SEND OK")) {
        printf("MQTT信息发送成功\r\n");
    } else if (strstr(clean_data, "+MQTTPUB:")) {
        printf("MQTT发布响应\r\n");
    } else {
        printf("没有对应的数据处理，接收到原始数据: %s\r\n", clean_data);
    }
    
    // 清理缓冲区
    memset(uart4_wifi_dma_rx_buf, 0, sizeof(uart4_wifi_dma_rx_buf));
}

/**
 * @brief UART4空闲中断处理函数
 */
void UART4_IDLEHandler(void)
{
    if (__HAL_UART_GET_FLAG(&huart4, UART_FLAG_IDLE) == SET) {
        __HAL_UART_CLEAR_FLAG(&huart4, UART_FLAG_IDLE);
        
        // 1. 同步停止DMA，确保数据稳定
        HAL_UART_DMAStop(&huart4);
        
        // 2. 获取数据长度
        uint8_t rlen = RXBUF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_uart4_rx);

        wifi_rx_state = true;
        memcpy(uart4_wifi_dma_rx_buf, rx4_buf, rlen);
        process_esp8266_response((char *)uart4_wifi_dma_rx_buf, rlen);
        
        // 5. 重新启动DMA接收
        HAL_UART_Receive_DMA(&huart4, rx4_buf, sizeof(rx4_buf));
    }
}

