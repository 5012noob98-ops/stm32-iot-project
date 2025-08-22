/**
 * @file cloud_interface.h
 * @brief 云平台接口模块头文件
 */

#ifndef CLOUD_INTERFACE_H
#define CLOUD_INTERFACE_H

#include "main.h"

/* 函数声明 */
void report_device_status(void);
void report_single_param(const char* param_name, int param_value);
void parse_json_str_command(char *json_str);
void report_status(void);

#endif /* CLOUD_INTERFACE_H */

