/**
 * @file esp8266_driver.h
 * @brief ESP8266驱动模块头文件
 */

#ifndef ESP8266_DRIVER_H
#define ESP8266_DRIVER_H

#include "main.h"
#include <stdbool.h>



/* 宏定义 */
#define RXBUF_SIZE 256

/* AT命令定义 */
#define AT "AT\r\n"
#define ACK_OK "OK"

// WiFi设置
#define AT_WIFI_MODE "AT+CWMODE=3\r\n"
#define AT_SET_PSW "AT+CWJAP=\"1\",\"12345678\"\r\n"

// MQTT设置
#define AT_SET_MQTT_CFG "AT+MQTTUSERCFG=0,1,\"NULL\",\"device1&k22zmFPIrvl\",\"1a6d06524b5dcba3f910fa5d59663950e9d60daf2dc0567cf9fc5f02f4a0f8e8\",0,0,\"\"\r\n"
#define AT_SET_MQTT_CLENTID "AT+MQTTCLIENTID=0,\"k22zmFPIrvl.device1|securemode=2\\,signmethod=hmacsha256\\,timestamp=1755854847860|\"\r\n"
#define AT_SET_MQTT_CONN "AT+MQTTCONN=0,\"iot-06z009xj3ichurh.mqtt.iothub.aliyuncs.com\",1883,1\r\n"

// MQTT订阅主题
#define AT_MQTT_SUB_PROPERTY_SET "AT+MQTTSUB=0,\"/sys/k22zmFPIrvl/device1/thing/service/property/set\",1\r\n"
#define AT_MQTT_SUB_PROPERTY_GET "AT+MQTTSUB=0,\"/sys/k22zmFPIrvl/device1/thing/service/property/get\",1\r\n"
#define AT_MQTT_SUB_PROPERTY_POST_REPLY "AT+MQTTSUB=0,\"/sys/k22zmFPIrvl/device1/thing/event/property/post_reply\",1\r\n"

/* 外部变量声明 */
extern uint8_t rx4_buf[RXBUF_SIZE];
extern uint8_t uart4_wifi_dma_rx_buf[RXBUF_SIZE];
extern bool wifi_rx_state;
extern DMA_HandleTypeDef hdma_uart4_rx;

/* 函数声明 */
void esp8266_init(void);
void send_cmd(uint8_t *cmd);
int send_cmd_with_retry(uint8_t *cmd, uint8_t retries);
void process_esp8266_response(char *data, uint8_t rlen);
void UART4_IDLEHandler(void);

#endif /* ESP8266_DRIVER_H */

