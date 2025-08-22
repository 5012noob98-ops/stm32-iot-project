/**
 * @file cloud_interface.c
 * @brief 云平台接口模块实现
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

/* 外部变量声明 */
extern volatile uint32_t system_tick;
extern uint8_t uart4_wifi_dma_rx_buf[];
extern DMA_HandleTypeDef hdma_usart1_rx;

/* 私有变量 */
static volatile uint32_t message_id = 0;

/**
 * @brief 报告设备状态到云平台
 */
void report_device_status(void)
{
    // 获取LED状态
    int led1_state = !HAL_GPIO_ReadPin(LED1_GPIO_Port, LED1_Pin);
    int led2_state = !HAL_GPIO_ReadPin(LED2_GPIO_Port, LED2_Pin);
    uint8_t LED = (led1_state || led2_state) ? 1 : 0;

    message_id++;

    printf("开始准备上报设备状态...\r\n");

    // 逐个上报参数
    report_single_param("LED", LED);
    report_single_param("LED1", led1_state);
    report_single_param("LED2", led2_state);
    report_single_param("Blink", get_led_tim());

    printf("所有参数上报完成\r\n");
}

/**
 * @brief 上报单个参数到云平台
 * @param param_name 参数名称
 * @param param_value 参数值
 */
void report_single_param(const char* param_name, int param_value)
{
    char publish_cmd[512];

    sprintf(publish_cmd,
            "AT+MQTTPUB=0,\"/sys/k22zmFPIrvl/device1/thing/event/property/post\"," \
            "\"{\\\"params\\\":{\\\"%s\\\":%d}}\",1,0\r\n",
            param_name, param_value);

    printf("上报参数 %s = %d\r\n", param_name, param_value);
    printf("发送指令:----> %s", publish_cmd);
    
    // 发送命令到ESP8266
    send_cmd((uint8_t *)publish_cmd);
    HAL_Delay(2000);
}

/**
 * @brief 解析JSON格式的命令
 * @param json_str 指向JSON字符串的指针
 */
void parse_json_str_command(char *json_str)
{
    // 1. 解析JSON字符串
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        printf("Error: Invalid JSON - %s\r\n", cJSON_GetErrorPtr());
        return;
    }

    // 首先检查是否是阿里云的控制命令格式
    cJSON *method = cJSON_GetObjectItem(root, "method");
    if (method && cJSON_IsString(method)) {
        // 处理控制命令
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
                        // 需要正确处理逻辑：如果LED2是开启的，只关闭LED1，保持LED2开启
                        if(HAL_GPIO_ReadPin(LED2_GPIO_Port, LED2_Pin) == GPIO_PIN_RESET) {
                            change_led(LED2_ON);  // 只关闭LED1，保持LED2开启
                        } else {
                            change_led(LED_OFF);  // LED2也关闭，则全部关闭
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
                        // 需要正确处理逻辑：如果LED1是开启的，只关闭LED2，保持LED1开启
                        if(HAL_GPIO_ReadPin(LED1_GPIO_Port, LED1_Pin) == GPIO_PIN_RESET) {
                            change_led(LED1_ON);  // 只关闭LED2，保持LED1开启
                        } else {
                            change_led(LED_OFF);  // LED1也关闭，则全部关闭
                        }
                        printf("Aliyun Control: Turn OFF LED\r\n");
                    }
                    reset_key_state();
                }

                // 处理闪烁频率
                cJSON *Blink = cJSON_GetObjectItem(params, "Blink");
                if (Blink && cJSON_IsNumber(Blink)) {
                    led_tim_set((uint32_t)Blink->valueint);
                    printf("Cloud Control: Blink interval set to %lu ms\r\n", get_led_tim());
                }
            }
        } else if (strcmp(method->valuestring, "thing.service.property.get") == 0) {
            printf("收到主动获取命令，准备上报设备状态\r\n");
            report_device_status(); // 主动上报状态
        }
        cJSON_Delete(root);
        return;
    }

    // 2. 获取 "cmd" 字段（旧格式）
    cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
    if (!cmd || !cJSON_IsString(cmd)) {
        cJSON_Delete(root);
        printf("Error: Missing or invalid 'cmd' field\r\n");
        return;
    }

    // 3. 根据 "cmd" 执行不同操作
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
        // 收到上报命令，准备上报状态
        report_device_status(); // 状态变化主动上报到云端
    } else {
        printf("找不到对应的JSON格式: %s\r\n", cmd->valuestring);
    }

    // 4. 释放内存
    cJSON_Delete(root);
}

/**
 * @brief 生成包含JSON格式的状态报告
 */
void report_status(void)
{
    // 1. 创建一个JSON对象
    cJSON *root = cJSON_CreateObject();
    if (!root) return;

    // 2. 添加状态信息
    cJSON_AddNumberToObject(root, "led1", !HAL_GPIO_ReadPin(LED1_GPIO_Port, LED1_Pin));
    cJSON_AddNumberToObject(root, "led2", !HAL_GPIO_ReadPin(LED2_GPIO_Port, LED2_Pin));
    cJSON_AddNumberToObject(root, "interval", get_led_tim());
    cJSON_AddStringToObject(root, "state", read_current_led_state() == LED_OFF ? "off" : "on");

    // 3. 将JSON对象打印为字符串形式

    char *status_string = cJSON_Print(root);
    if (status_string) {
        printf("%s\r\n", status_string);
        // 释放cJSON_Print分配的内存
        cJSON_free(status_string);
    }

    // 4. 释放JSON对象内存
    cJSON_Delete(root);
}

