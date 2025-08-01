#include "main.h"
#include "mywifi.h"
#include "mynvs.h"
#include "uart.h"   
#include "io_control.h"
#include "WS2812.h"
#include "data_analysis.h"
#include "aliot_ota.h"
#include "ILI9488_lvgl.h"

SemaphoreHandle_t xReadWriteSemaphore;//读写nvs信号量
QueueHandle_t uartQueue; // UART接收队列
QueueHandle_t dataAnalysisQueue; // 数据分析队列
static led_strip_handle_t strip = NULL;
Config_t wifiConfigInfo = 
    {
        .mode = WIFI_STA,  //WIFI工作模式
        .ssid = "CMCC-C67C",     //WIFI SSID
        .password = "3.141592653",   //WIFI密码
        .deviceIP = "192.168.1.100", //设备IP地址
        .deviceNetMask = "255.255.255.0", //设备子网掩码
        .deviceGateway = "192.168.1.1", //设备网关
        .device_port = 9547,      //设备端口
        .max_connection = 1,       //最大连接数
        .dhcpEnable = 1            //是否使用DHCP
    };//wifi配置结构体
static void print_config(const Config_t config)
{
    ESP_LOGI("CONFIG", "mode           : %d", config.mode);
    ESP_LOGI("CONFIG", "ssid           : %s", config.ssid);
    ESP_LOGI("CONFIG", "password       : %s", config.password);
    ESP_LOGI("CONFIG", "deviceIP       : %s", config.deviceIP);
    ESP_LOGI("CONFIG", "deviceNetMask  : %s", config.deviceNetMask);
    ESP_LOGI("CONFIG", "deviceGateway  : %s", config.deviceGateway);
    ESP_LOGI("CONFIG", "device_port    : %d", config.device_port);
    ESP_LOGI("CONFIG", "max_connection : %d", config.max_connection);
    ESP_LOGI("CONFIG", "dhcpEnable     : %d", config.dhcpEnable);
}
TaskHandle_t lvgl_task_handle = NULL;
TaskHandle_t gpio_task_handle = NULL;
/**
 * @brief 应用程序入口函数
 * @return 无
 */
void app_main(void)
{
    printf("Starting application...\n");
    ESP_LOGI("app_main","application version %s",get_app_version());
    // esp_log_level_set("*", ESP_LOG_NONE);  // 关闭所有模块的日志i
    xReadWriteSemaphore = xSemaphoreCreateBinary();//nvs读写信号量
    assert(xReadWriteSemaphore);
    xSemaphoreGive(xReadWriteSemaphore);//先释放一次
    dataAnalysisQueue = xQueueCreate(dataAnalysisQueueLen, dataAnalysisQueueItemSize); // 创建数据分析队列
    //NVS初始化（WIFI底层驱动有用到NVS，所以这里要初始化）
    mynvs_init();
    esp_err_t ret = mynvs_get_config(&wifiConfigInfo); //从NVS中读取WIFI配置
    if (ret != ESP_OK) 
    {
        ESP_LOGW("app_main", "Failed to get WiFi config from NVS");
    }
    else
    {
        ESP_LOGI("app_main", "WiFi config loaded successfully from NVS");
        print_config(wifiConfigInfo);
    }
    //wifi工作模式初始化
    wifi_init(wifiConfigInfo);
    uart_init(); //UART初始化
    io_control_init(); //IO控制初始化
    
    // 初始化 RMT TX
    strip = WS2812_init();
    assert(strip);
    led_strip_set_pixel(strip, 0, 255, 0, 0);

    led_strip_refresh(strip);
    ESP_LOGI("WS2812", "Red LED should be on");
    BaseType_t err = xTaskCreate(analysis_data_thread, "analysis_data_thread", 4096, NULL, 7, NULL); //创建数据分析线程
    if(err != pdPASS) 
    {
        ESP_LOGE("app_main", "Failed to create analysis_data_thread");
    }  
    else
    {
        ESP_LOGI("app_main", "analysis_data_thread created successfully");
    } 
    err = xTaskCreatePinnedToCore(uart_thread, "uart_thread", 5120, NULL, 6, NULL, 1); //创建接收线程
    if(err != pdPASS) 
    {
        ESP_LOGE("app_main", "Failed to create uart_thread");
    }  
    else
    {
        ESP_LOGI("app_main", "uart_thread created successfully");
    }
     err = xTaskCreatePinnedToCore(gpio_thread, "gpio_thread", 4096, NULL, 8, &gpio_task_handle, 1); //创建GPIO线程
    if(err != pdPASS) 
    {
        ESP_LOGE("app_main", "Failed to create gpio_thread");
    }
    else
    {
        ESP_LOGI("app_main", "gpio_thread created successfully");
    }
    err = xTaskCreatePinnedToCore(heartLed_thread, "heartLed_thread", 4096, NULL, 9, NULL, 1); //创建心跳LED线程
    if(err != pdPASS) 
    {
        ESP_LOGE("app_main", "Failed to create heartLed_thread");
    }  
    else
    {
        ESP_LOGI("app_main", "heartLed_thread created successfully");
    }
    err = xTaskCreatePinnedToCore(lvgl_thread, "lvgl_thread", 10240,NULL, 5,  &lvgl_task_handle, 1); //创建lvgl线程
    if(err != pdPASS) 
    {
        ESP_LOGE("app_main", "Failed to create lvgl_thread");
    }  
    else
    {
        ESP_LOGI("app_main", "lvgl_thread created successfully");
    }
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
* @brief 心跳LED线程
 * @param arg   用户传递的参数
 * @return 无
 * 
 * 该线程用于控制心跳LED灯的颜色变化，模拟心跳效果。
 * 使用WS2812 LED灯带进行颜色渐变显示。
*/
void heartLed_thread(void *arg)
{
    ESP_LOGI("HeartLed", "Heart LED thread started");
    rgb_t red = {255, 0, 0};
    rgb_t green = {0, 255, 0};
    rgb_t blue = {0, 0, 255};
    
    while(1) 
    {
        WS2812_color_fade(strip, red, green, 50, 20);   // 红 -> 绿
        WS2812_color_fade(strip, green, blue, 50, 20);  // 绿 -> 蓝
        WS2812_color_fade(strip, blue, red, 50, 20);    // 蓝 -> 红
    }
    vTaskDelete(NULL);
}

/**
 * @brief 数据分析线程
 * @param arg   用户传递的参数
 * @return 无
 */
void analysis_data_thread(void *arg)
{
    ESP_LOGI("DataAnalysis", "Data analysis thread started");
    uint8_t dataBuffer[1024] = { 0 };
    BaseType_t err;
    while(1)
    {
        err = xQueueReceive(dataAnalysisQueue, dataBuffer, portMAX_DELAY); //从数据分析队列中接收数据
        if(err == pdPASS) 
        {
            // 处理接收到的数据
            ESP_LOGI("DataAnalysis", "Received data: %s", dataBuffer);
            /*数据解析*/
            uart_write_bytes(UART_NUM_1, (const char *)dataBuffer, strlen((const char *)dataBuffer)); // 发送数据到UART设备
        }
        else 
        {
            ESP_LOGE("DataAnalysis", "Failed to receive data from queue");
        }
        memset(dataBuffer, 0, sizeof(dataBuffer)); // 清空缓冲区
    }
    vTaskDelete(NULL); // 删除当前任务
}
