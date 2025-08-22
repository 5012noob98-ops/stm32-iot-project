/**
 * @file cloud_interface.c
 * @brief ��ƽ̨�ӿ�ģ��ʵ��
 */

#include "cloud_interface.h"
#include "usart.h"
#include "gpio.h"
#include "led_control.h"
#include "key_handler.h"
#include "esp8266_driver.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* �ⲿ�������� */
extern volatile uint32_t system_tick;
extern uint8_t uart4_wifi_dma_rx_buf[];
extern DMA_HandleTypeDef hdma_usart1_rx;

/* ˽�б��� */
static volatile uint32_t message_id = 0;

/**
 * @brief �����豸״̬����ƽ̨
 */
void report_device_status(void)
{
    // ��ȡLED״̬
    int led1_state = !HAL_GPIO_ReadPin(LED1_GPIO_Port, LED1_Pin);
    int led2_state = !HAL_GPIO_ReadPin(LED2_GPIO_Port, LED2_Pin);
    uint8_t LED = (led1_state || led2_state) ? 1 : 0;

    message_id++;

    printf("��ʼ׼���ϱ��豸״̬...\r\n");

    // ����ϱ�����
    report_single_param("LED", LED);
    report_single_param("LED1", led1_state);
    report_single_param("LED2", led2_state);
    report_single_param("Blink", get_led_tim());

    printf("���в����ϱ����\r\n");
}

/**
 * @brief �ϱ�������������ƽ̨
 * @param param_name ��������
 * @param param_value ����ֵ
 */
void report_single_param(const char* param_name, int param_value)
{
    char publish_cmd[512];

    sprintf(publish_cmd,
            "AT+MQTTPUB=0,\"/sys/k22zmFPIrvl/device1/thing/event/property/post\"," \
            "\"{\\\"params\\\":{\\\"%s\\\":%d}}\",1,0\r\n",
            param_name, param_value);

    printf("�ϱ����� %s = %d\r\n", param_name, param_value);
    printf("����ָ��:----> %s", publish_cmd);
    
    // �������ESP8266
    send_cmd((uint8_t *)publish_cmd);
    HAL_Delay(2000);
}

/**
 * @brief ����JSON��ʽ������
 * @param json_str ָ��JSON�ַ�����ָ��
 */
void parse_json_str_command(char *json_str)
{
    // 1. ����JSON�ַ���
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        printf("Error: Invalid JSON - %s\r\n", cJSON_GetErrorPtr());
        return;
    }

    // ���ȼ���Ƿ��ǰ����ƵĿ��������ʽ
    cJSON *method = cJSON_GetObjectItem(root, "method");
    if (method && cJSON_IsString(method)) {
        // �����������
        if (strcmp(method->valuestring, "thing.service.property.set") == 0) {
            cJSON *params = cJSON_GetObjectItem(root, "params");
            if (params) {
                cJSON *LED = cJSON_GetObjectItem(params, "LED");
                if (LED && cJSON_IsNumber(LED)) {
                    if (LED->valueint == 1) {
                        change_led(LED_ON);
                        printf("Aliyun Control: Turn ON all LEDs\r\n");
                    } else {
                        change_led(LED_OFF);
                        printf("Aliyun Control: Turn OFF all LEDs\r\n");
                    }
                    reset_key_state();
                }

                cJSON *LED1 = cJSON_GetObjectItem(params, "LED1");
                if (LED1 && cJSON_IsNumber(LED1)) {
                    if (LED1->valueint == 1) {
                        change_led(LED1_ON);
                        printf("Aliyun Control: Turn ON LED1\r\n");
                    } else {
                        // ��Ҫ��ȷ�����߼������LED2�ǿ����ģ�ֻ�ر�LED1������LED2����
                        if(HAL_GPIO_ReadPin(LED2_GPIO_Port, LED2_Pin) == GPIO_PIN_RESET) {
                            change_led(LED2_ON);  // ֻ�ر�LED1������LED2����
                        } else {
                            change_led(LED_OFF);  // LED2Ҳ�رգ���ȫ���ر�
                        }
                        printf("Aliyun Control: Turn OFF LED\r\n");
                    }
                    reset_key_state();
                }

                cJSON *LED2 = cJSON_GetObjectItem(params, "LED2");
                if (LED2 && cJSON_IsNumber(LED2)) {
                    if (LED2->valueint == 1) {
                        change_led(LED2_ON);
                        printf("Aliyun Control: Turn ON LED2\r\n");
                    } else {
                        // ��Ҫ��ȷ�����߼������LED1�ǿ����ģ�ֻ�ر�LED2������LED1����
                        if(HAL_GPIO_ReadPin(LED1_GPIO_Port, LED1_Pin) == GPIO_PIN_RESET) {
                            change_led(LED1_ON);  // ֻ�ر�LED2������LED1����
                        } else {
                            change_led(LED_OFF);  // LED1Ҳ�رգ���ȫ���ر�
                        }
                        printf("Aliyun Control: Turn OFF LED\r\n");
                    }
                    reset_key_state();
                }

                // ������˸Ƶ��
                cJSON *Blink = cJSON_GetObjectItem(params, "Blink");
                if (Blink && cJSON_IsNumber(Blink)) {
                    led_tim_set((uint32_t)Blink->valueint);
                    printf("Cloud Control: Blink interval set to %lu ms\r\n", get_led_tim());
                }
            }
        } else if (strcmp(method->valuestring, "thing.service.property.get") == 0) {
            printf("�յ�������ȡ���׼���ϱ��豸״̬\r\n");
            report_device_status(); // �����ϱ�״̬
        }
        cJSON_Delete(root);
        return;
    }

    // 2. ��ȡ "cmd" �ֶΣ��ɸ�ʽ��
    cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
    if (!cmd || !cJSON_IsString(cmd)) {
        cJSON_Delete(root);
        printf("Error: Missing or invalid 'cmd' field\r\n");
        return;
    }

    // 3. ���� "cmd" ִ�в�ͬ����
    if (strcmp(cmd->valuestring, "led") == 0) {
        cJSON *id = cJSON_GetObjectItem(root, "id");
        cJSON *op = cJSON_GetObjectItem(root, "op");
        if (id && cJSON_IsNumber(id) && op && cJSON_IsString(op)) {
            if (strcmp(op->valuestring, "on") == 0) {
                if (id->valueint == 1) {
                    change_led(LED1_ON);
                    printf("JSON : LED1 ON\r\n");
                } else if (id->valueint == 2) {
                    change_led(LED2_ON);
                    printf("JSON : LED2 ON\r\n");
                } else {
                    change_led(LED_ON);
                    printf("JSON : LED ON\r\n");
                }
                reset_key_state();
            } else if(strcmp(op->valuestring, "off") == 0) {
                change_led(LED_OFF);
                printf("JSON : LED OFF\r\n");
                reset_key_state();
            }
        }
    } else if(strcmp(cmd->valuestring, "blink") == 0) {
        cJSON *interval = cJSON_GetObjectItem(root, "interval");
        if (interval && cJSON_IsNumber(interval)) {
            led_tim_set((uint32_t)interval->valueint);
            printf("JSON:BLINK interval set to %lu\r\n", get_led_tim());
        }
    } else if(strcmp(cmd->valuestring, "report") == 0) {
        // �յ��ϱ����׼���ϱ�״̬
        report_device_status(); // ״̬�仯�����ϱ����ƶ�
    } else {
        printf("�Ҳ�����Ӧ��JSON��ʽ: %s\r\n", cmd->valuestring);
    }

    // 4. �ͷ��ڴ�
    cJSON_Delete(root);
}

/**
 * @brief ���ɰ���JSON��ʽ��״̬����
 */
void report_status(void)
{
    // 1. ����һ��JSON����
    cJSON *root = cJSON_CreateObject();
    if (!root) return;

    // 2. ���״̬��Ϣ
    cJSON_AddNumberToObject(root, "led1", !HAL_GPIO_ReadPin(LED1_GPIO_Port, LED1_Pin));
    cJSON_AddNumberToObject(root, "led2", !HAL_GPIO_ReadPin(LED2_GPIO_Port, LED2_Pin));
    cJSON_AddNumberToObject(root, "interval", get_led_tim());
    cJSON_AddStringToObject(root, "state", read_current_led_state() == LED_OFF ? "off" : "on");

    // 3. ��JSON�����ӡΪ�ַ�����ʽ

    char *status_string = cJSON_Print(root);
    if (status_string) {
        printf("%s\r\n", status_string);
        // �ͷ�cJSON_Print������ڴ�
        cJSON_free(status_string);
    }

    // 4. �ͷ�JSON�����ڴ�
    cJSON_Delete(root);
}

