/**
 * @file cloud_interface.h
 * @brief ��ƽ̨�ӿ�ģ��ͷ�ļ�
 */

#ifndef CLOUD_INTERFACE_H
#define CLOUD_INTERFACE_H

#include "main.h"

/* �������� */
void report_device_status(void);
void report_single_param(const char* param_name, int param_value);
void parse_json_str_command(char *json_str);
void report_status(void);

#endif /* CLOUD_INTERFACE_H */

