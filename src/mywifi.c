#include "mywifi.h"
#include "main.h"
#include "mqtt_client.h"

static const char *TAG = "wifi";
mqtt mqttclient =
{
    .deviceID = DEVICE_ID,
    .accessKey = ACCESS_KEY,
    .port = MQTT_PORT,
    .productID = PRODUCT_ID
}; // MQTT客户端句柄
/** 事件回调函数
 * @param arg   用户传递的参数
 * @param event_base    事件类别
 * @param event_id      事件ID
 * @param event_data    事件携带的数据
 * @return 无
*/
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    char buffer[1024] = { 0 }; // 接收缓冲区
    switch (event_id) 
    {
        case MQTT_EVENT_CONNECTED:
            snprintf(mqttclient.subscribeTopic, sizeof(mqttclient.subscribeTopic), "$sys/%s/%s/cmd/request/+", mqttclient.productID, mqttclient.deviceID); // 设置订阅主题
            esp_mqtt_client_subscribe(mqttclient.client, mqttclient.subscribeTopic, 0);
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            esp_mqtt_client_reconnect(mqttclient.client); // 断开连接后尝试重连
            
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA: topic=%.*s, data=%.*s",
                     event->topic_len, event->topic,
                     event->data_len, event->data);
            memset(buffer, 0, sizeof(buffer)); // 清空接收缓冲区
            strncpy(buffer,event->data_len,event->data); // 复制数据到缓冲区
            xQueueSend(dataAnalysisQueue, buffer, 0);//发送到解析数据线程
            snprintf(mqttclient.publishTopic, sizeof(mqttclient.publishTopic), "$sys/%s/%s/dp/post/json", mqttclient.productID, mqttclient.deviceID); // 构建发布主题
            esp_mqtt_client_publish(mqttclient.client, mqttclient.publishTopic, "{\"id\":10}", 0, 0, 0);
            break;
        default:
            break;
    }
}
/** 事件回调函数
 * @param arg   用户传递的参数
 * @param event_base    事件类别
 * @param event_id      事件ID
 * @param event_data    事件携带的数据
 * @return 无
*/
static void ap_event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data)
{
    if(event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
            case WIFI_EVENT_AP_START:      //WIFI以AP模式启动后触发此事件
                ESP_LOGI(TAG, "AP started");
                /*保存自己的IP地址*/
                ap_getlocal_IP(); //获取AP模式下的本地IP地址
                ESP_LOGI(TAG, "AP local IP: %s", wifiConfigInfo.deviceIP);
                #if !MQTTEnabled
                xTaskCreate(ap_tcpserver_thread, "ap_tcpserver_thread", 4096, arg, 16, NULL); //创建TCP服务器线程
                #endif
                break;
            case WIFI_EVENT_AP_STACONNECTED:  //有客户端连接到AP时触发此事件
                ESP_LOGI(TAG, "Client connected to AP");
                #if MQTTEnabled
                mqtt_start(); //启动mqtt客户端
                #endif
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:   //有客户端从AP断开连接时触发此事件
                ESP_LOGI(TAG, "Client disconnected from AP");
                #if MQTTEnabled
                esp_mqtt_client_unregister_event(mqttclient.client, MQTT_EVENT_ANY, mqtt_event_handler); // 注销事件处理函数
                mqtt_stop(); // 停止MQTT客户端
                #endif
                break;
            default:
                break;
        }
    }
}

/** 事件回调函数
 * @param arg   用户传递的参数
 * @param event_base    事件类别
 * @param event_id      事件ID
 * @param event_data    事件携带的数据
 * @return 无
*/
static void sta_event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data)
{   
    if(event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
            case WIFI_EVENT_STA_START:      //WIFI以STA模式启动后触发此事件
                esp_wifi_connect();         //启动WIFI连接
                break;
            case WIFI_EVENT_STA_CONNECTED:  //WIFI连上路由器后，触发此事件
                ESP_LOGI(TAG, "connected to AP");
                
                break;
            case WIFI_EVENT_STA_DISCONNECTED:   //WIFI从路由器断开连接后触发此事件
                #if MQTTEnabled
                esp_mqtt_client_unregister_event(mqttclient.client, MQTT_EVENT_ANY, mqtt_event_handler); // 注销事件处理函数
                mqtt_stop(); // 停止MQTT客户端
                #endif
                esp_wifi_connect();             //继续重连
                ESP_LOGI(TAG,"connect to the AP fail,retry now");
                break;
            default:
                break;
        }
    }
    if(event_base == IP_EVENT)                  //IP相关事件
    {
        switch(event_id)
        {
            case IP_EVENT_STA_GOT_IP:           //只有获取到路由器分配的IP，才认为是连上了路由器
                ESP_LOGI(TAG,"get ip address");
                /*保存自己的IP地址*/
                sta_getlocal_IP(); //获取STA模式下的本地IP地址
                ESP_LOGI(TAG, "STA local IP: %s", wifiConfigInfo.deviceIP);
                #if MQTTEnabled
                mqtt_start();//启动mqtt客户端
                #endif
                break;
        }
    }
}
/** 静态IP设置函数
 * @param config   要配置的IP信息
 * @param netif    对应的网络接口
 * @return 无
*/
static void static_ip_set(Config_t config, esp_netif_t *netif)
{
    esp_netif_ip_info_t ip_info;
    // 停止 DHCP server
    esp_netif_dhcps_stop(netif);
    // 设置静态 IP 地址
    ip4addr_aton(config.deviceIP, &ip_info.ip);      // IP
    ip4addr_aton(config.deviceNetMask, &ip_info.netmask); // 子网掩码
    ip4addr_aton(config.deviceGateway, &ip_info.gw);      // 网关
    // 设置 IP 信息
    esp_netif_set_ip_info(netif, &ip_info);
}

/** WIFI初始化函数
 * @param config   WIFI配置参数
 * @return ESP_OK表示成功，其他表示失败
*/
esp_err_t wifi_init(Config_t config)
{   
    wifi_config_t wifi_config;  //WIFI配置结构体
    esp_netif_t *netif;  //网络接口对象
    ESP_ERROR_CHECK(esp_netif_init());  //用于初始化tcpip协议栈
    ESP_ERROR_CHECK(esp_event_loop_create_default());       //创建一个默认系统事件调度循环，之后可以注册回调函数来处理系统的一些事件
    

    //初始化WIFI
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)); 
    memset(&wifi_config, 0, sizeof(wifi_config));  //清空配置
    if(config.mode == WIFI_STA)  //如果是STA模式
    {
        netif = esp_netif_create_default_wifi_sta();    //使用默认配置创建STA对象
        if(config.dhcpEnable == 0) //如果不使用DHCP
        {
            ESP_LOGI(TAG, "DHCP is disabled, setting static IP");
            static_ip_set(config, netif);  //设置静态IP地址
        }
        // 注册 STA 模式相关事件
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, sta_event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, sta_event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, sta_event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, sta_event_handler, NULL));
        //WIFI配置
        memcpy(wifi_config.sta.ssid, config.ssid, sizeof(config.ssid));  //复制SSID
        memcpy(wifi_config.sta.password, config.password, sizeof(config.password));  //复制
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;  //设置加密方式为WPA2_PSK  
        wifi_config.sta.pmf_cfg.capable = true;  //设置PMF能力
        wifi_config.sta.pmf_cfg.required = false; //不强制要求PMF
        //启动WIFI
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );         //设置工作模式为STA
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );   //设置wifi配置
    }
    else if(config.mode == WIFI_AP)  //如果是AP模式
    {
        //WIFI配置
        netif = esp_netif_create_default_wifi_ap();
        if(config.dhcpEnable == 0) //如果不使用DHCP
        {
            ESP_LOGI(TAG, "DHCP is disabled, setting static IP");
            static_ip_set(config, netif);  //设置静态IP地址
        }
        // 注册 AP 模式相关事件
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_START, ap_event_handler, &config));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, ap_event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, ap_event_handler, NULL));
        //AP模式的WIFI配置
        if(strlen((const char *)config.password) < 8)
        {
            ESP_LOGE(TAG, "AP password length must be at least 8");
            memset(config.password, 0, sizeof(config.password));  //如果密码长度小于8，清空密码
            strcpy((char *)config.password, "12345678");  //如果密码长度小于8，设置默认密码
        }
        memcpy(wifi_config.ap.ssid, config.ssid, sizeof(config.ssid));  //复制SSID
        memcpy(wifi_config.ap.password, config.password, sizeof(config.password));  //复制密码
        wifi_config.ap.max_connection = config.max_connection;  //设置最大连接数
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;  //设置加密方式为WPA2_PSK

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP) );         //设置工作模式为AP
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config) );   //设置wifi配置
    }
   
    ESP_ERROR_CHECK(esp_wifi_start() );                         //启动WIFI
    
    ESP_LOGI(TAG, "wifi_init finished.");
    return ESP_OK;
}
/** AP模式下的tcp服务器线程
 * @param arg   用户传递的参数，包含设备端口号等配置信息
 * @return 无
*/
void ap_tcpserver_thread(void *arg)
{
    unsigned short port = ((Config_t*)arg)->device_port;  //获取监听端口号
    fd_set read_fds;
    int server_fd, client_fd, max_fd, activity;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    server_fd = network_tcp_init(TCP_SERVER, port);  //初始化TCP服务器
    if(server_fd < 0) 
    {
        ESP_LOGE("ap_tcpserver_thread", "Failed to create TCP server socket");
        vTaskDelete(NULL);  // 删除当前任务
        return;
    }
    ESP_LOGI("ap_tcpserver_thread", "TCP server started on port %d", port);
    max_fd = server_fd;  //设置最大文件描述符为服务器套接字
    FD_ZERO(&read_fds);
    FD_SET(server_fd, &read_fds);
    while(1)
    {
        activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);  //等待客户端连接
        if ((activity < 0) && (errno != EINTR)) 
        {
            ESP_LOGE(TAG, "select error");
            continue;
        }
         // 处理新连接
        if (FD_ISSET(server_fd, &read_fds)) 
        {
            client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
            if (client_fd < 0) 
            {
                ESP_LOGE(TAG, "accept failed");
                continue;
            }
            /*连接上客户端之后开始接收数据*/
            BaseType_t err = xTaskCreate(ap_recv_thread, "ap_recv_thread", 2048, (void *)client_fd, 17, NULL); //创建接收线程
            if(err != pdPASS) 
            {
                ESP_LOGE(TAG, "Failed to create receive task");
                close(client_fd);  //关闭客户端套接字
                continue;
            }
            ESP_LOGI(TAG, "New connection: socket fd=%d", client_fd);

        }

    }
}
/** AP模式下的接收线程
 * @param arg   用户传递的参数，包含客户端套接字
 * @return 无
*/
void ap_recv_thread(void *arg)
{
    int client_fd = *(int*)arg;  //获取客户端套接字
    char buffer[1024] = { 0 };  //接收缓冲区
    int bytes_received = 0;  //接收到的字节数
    while(1)
    {
        bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);  //接收数据
        if (bytes_received < 0) 
        {
            ESP_LOGE(TAG, "recv failed: %s", strerror(errno));
            memset(buffer, 0, sizeof(buffer));  //清空缓冲区
            continue;
        }
        else if (bytes_received == 0) 
        {
            ESP_LOGI(TAG, "Client disconnected");
            break;  //客户端断开连接
        }
        buffer[bytes_received] = '\0';  //确保字符串结束
        /*接收到数据之后需要解析数据*/
        BaseType_t err = xQueueSend(dataAnalysisQueue, buffer, 0); //将接收到的数据发送到解析数据队列
        if(err != pdPASS) 
        {
            ESP_LOGE(TAG, "Failed to send data to UART queue");
        }
        ESP_LOGI(TAG, "Received data: %s", buffer);
        memset(buffer, 0, sizeof(buffer));  //清空缓冲区
    }
    close(client_fd);  //关闭客户端套接字
    vTaskDelete(NULL);  // 删除当前任务
}
/** 网络TCP初始化函数
 * @param mode   网络模式
 * @param port   监听端口
 * @return 成功返回套接字描述符，失败返回-1
*/
int network_tcp_init(network_mode mode,unsigned short port)
{
    int sockfd;
    struct sockaddr_in addr;
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sockfd < 0) 
    {
        ESP_LOGE("TCP", "Failed to create socket: %s", strerror(errno));
        return -1;
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&opt, sizeof(opt));
    setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&opt, sizeof(opt));//端口复用
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);  // 端口号
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    listen(sockfd, 5);  // 设置监听队列长度为5
    return sockfd;  // 返回套接字描述符
}
/**
 * @brief 获取STA模式下的本地IP地址
 */
void sta_getlocal_IP(void)
{
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif) 
    {
        esp_netif_get_ip_info(netif, &ip_info);
        ESP_LOGI(TAG, "Local IP: %s", ip4addr_ntoa(&ip_info.ip));
        memset(wifiConfigInfo.deviceIP, 0, sizeof(wifiConfigInfo.deviceIP)); // 清空IP地址字符串
        memset(wifiConfigInfo.deviceNetMask, 0, sizeof(wifiConfigInfo.deviceNetMask)); // 清空子网掩码字符串
        memset(wifiConfigInfo.deviceGateway, 0, sizeof(wifiConfigInfo.deviceGateway)); // 清空网关字符串
        // 将IP地址、子网掩码和网关转换为字符串并保存
        strcpy(wifiConfigInfo.deviceIP, ip4addr_ntoa(&ip_info.ip));
        strcpy(wifiConfigInfo.deviceNetMask, ip4addr_ntoa(&ip_info.netmask));
        strcpy(wifiConfigInfo.deviceGateway, ip4addr_ntoa(&ip_info.gw));
        
    } 
    else 
    {
        ESP_LOGE(TAG, "Failed to get netif handle for STA");
    }
}
/**
 * @brief 获取AP模式下的本地IP地址
 */
void ap_getlocal_IP(void)
{
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (netif) 
    {
        esp_netif_get_ip_info(netif, &ip_info);
        ESP_LOGI(TAG, "Local IP: %s", ip4addr_ntoa(&ip_info.ip));
        memset(wifiConfigInfo.deviceIP, 0, sizeof(wifiConfigInfo.deviceIP)); // 清空IP地址字符串
        strcpy(wifiConfigInfo.deviceIP, ip4addr_ntoa(&ip_info.ip));
    } 
    else 
    {
        ESP_LOGE(TAG, "Failed to get netif handle for AP");
    }
}
/**
 * @brief 启动MQTT客户端
 */
void mqtt_start(void)
{
    if(wifiConfigInfo.mode == WIFI_AP) // 如果是AP模式
    {
        ESP_LOGI(TAG, "MQTT started in AP mode");
        
        sprintf(mqttclient.uri, "mqtt://%s:%d", MQTT_BROKER, mqttclient.port); // 构建MQTT Broker URI
    }
    else if(wifiConfigInfo.mode == WIFI_STA) // 如果是STA模式
    {
        ESP_LOGI(TAG, "MQTT started in STA mode");
        sprintf(mqttclient.uri, "mqtt://%s:%d", MQTT_BROKER, mqttclient.port); // 构建MQTT Broker URI
    }
    esp_mqtt_client_config_t mqtt_cfg = 
    {
        .broker.address.uri = mqttclient.uri , // MQTT Broker URI
        .credentials = 
        {
            .username = mqttclient.productID,
            .client_id = mqttclient.deviceID,
            .authentication.password = mqttclient.accessKey,
        }
    };
    mqttclient.client = esp_mqtt_client_init(&mqtt_cfg);
    /*注册回调函数*/
    esp_mqtt_client_register_event(mqttclient.client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);

    esp_mqtt_client_start(mqttclient.client);  // 启动MQTT客户端
}
/**
 * @brief 停止MQTT客户端
 */
void mqtt_stop(void)
{
    if(mqttclient.client != NULL) // 如果mqtt客户端已存在
    {
        esp_mqtt_client_stop(mqttclient.client); // 停止MQTT客户端
        esp_mqtt_client_destroy(mqttclient.client); // 销毁MQTT客户端
        mqttclient.client = NULL; // 清空mqtt客户端
    }
}