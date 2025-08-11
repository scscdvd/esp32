#ifndef _ALIOT_OTA_H
#define _ALIOT_OTA_H

#include "main.h"


//ota完成回调函数
typedef void(*ota_finish_callback_t)(int code);

void start_aliot_mqtt(void);
void aliot_mqtt_stop(void);
void aliot_mqtt_event_handler(void* arg,esp_event_base_t event_base,int32_t event_id,void* event_data);
const char* get_app_version(void);
void aliot_report_version(void);
esp_err_t aliot_config(char * uri,ota_finish_callback_t cb);
esp_err_t aliot_ota_start();
void aliot_ota_cancel_rollback(void);
void aliot_ntp_init(void);
extern mqtt aliot_mqtt;
static const char *aliyun_root_cert_pem;
#endif