#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"
#include <string.h> 
#include <stdlib.h>
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi_types.h"
#include "freertos/semphr.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "esp_netif.h"
#include "mqtt_client.h"
#include "mywifi.h"
#include <arpa/inet.h>  

#define MQTTEnabled 1 //MQTT功能开关

#define MQTT_BROKER "mqtt://broker-cn.emqx.io" // 替换为实际的MQTT Broker地址
#define MQTT_PORT 1883  // 或8883 for SSL

#define MQTT_USERNAME "esp32_v1"
#define MQTT_CLIENT_ID "mqttx_bb521919"  // 请替换为你的实际设备ID
#define MQTT_PASSWORD "123456789"  

#define MQTT_PUBLISH_TOPIC "topic/pub" // 发布主题
#define MQTT_SUBSCRIBE_TOPIC "topic/sub" // 订阅主题

/*消息队列*/
#define networkToUartQueueLen 4 //网络到UART的队列长度
#define networkToUartQueueItemSize 1024 //网络到UART的队列项大小

#define dataAnalysisQueueLen 4 //数据分析队列长度
#define dataAnalysisQueueItemSize 1024 //数据分析队列项大小

#define gpioQueueLen 4 //GPIO队列长度
#define gpioQueueItemSize 1 //GPIO队列项大小

extern SemaphoreHandle_t xReadWriteSemaphore; // 读写NVS信号量
extern QueueHandle_t networkToUartQueue; // 网络到UART的队列
extern QueueHandle_t dataAnalysisQueue; // 数据分析队列
extern QueueHandle_t gpioQueue; // GPIO队列

void uart_thread(void *arg);
void analysis_data_thread(void *arg);
void gpio_thread(void *arg);
#endif
