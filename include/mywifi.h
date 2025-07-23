#ifndef _MYWIFI_H_
#define _MYWIFI_H_
#include "main.h"


typedef enum 
{
    WIFI_STA = 0,  //STA接口
    WIFI_AP,       //AP接口
} wifiMode_t;

typedef struct 
{
    wifiMode_t mode;  //WIFI工作模式
    uint8_t ssid[32];     //WIFI SSID
    uint8_t password[64]; //WIFI密码
    uint8_t max_connection; //最大连接数
    unsigned short device_port; //设备端口
    char deviceIP[32]; //设备IP地址
    char deviceNetMask[32]; //设备子网掩码
    char deviceGateway[32]; //设备网关
    uint8_t dhcpEnable; //是否使用DHCP
} Config_t;
typedef struct
{
    /* data */
    esp_mqtt_client_handle_t client;
    char uri[128]; // MQTT Broker URI
    char deviceID[64]; // 设备ID
    char accessKey[128]; // 访问密钥
    unsigned short port; // MQTT端口
    char publishTopic[256]; // 发布主题
    char subscribeTopic[256]; // 订阅主题
    char productID[64]; // 产品ID
} mqtt;

typedef enum 
{
    TCP_SERVER = 0, //TCP服务器
    TCP_CLIENT,     //TCP客户端
} network_mode;

//WIFI初始化
esp_err_t wifi_init(Config_t config);
int network_tcp_init(network_mode mode,unsigned short port);
void ap_getlocal_IP(void);
void sta_getlocal_IP(void);
void mqtt_start(void);
void mqtt_stop(void);
/*线程*/
void ap_tcpserver_thread(void *arg);
void ap_recv_thread(void *arg);


extern Config_t wifiConfigInfo; // WIFI配置结构体
#endif
