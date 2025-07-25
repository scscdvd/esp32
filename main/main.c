#include "main.h"
#include "mywifi.h"
#include "mynvs.h"
#include "uart.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "io_control.h"
SemaphoreHandle_t xReadWriteSemaphore;//读写nvs信号量
QueueHandle_t networkToUartQueue; // 网络到UART的队列
QueueHandle_t dataAnalysisQueue; // 数据分析队列
QueueHandle_t gpioQueue; // GPIO队列

Config_t wifiConfigInfo = 
    {
        .mode = WIFI_STA,  //WIFI工作模式
        .ssid = "fang",     //WIFI SSID
        .password = "12345678",   //WIFI密码
        .deviceIP = "192.168.1.100", //设备IP地址
        .deviceNetMask = "255.255.255.0", //设备子网掩码
        .deviceGateway = "192.168.1.1", //设备网关
        .device_port = 9547,      //设备端口
        .max_connection = 1,       //最大连接数
        .dhcpEnable = 1            //是否使用DHCP
    };//wifi配置结构体
/**
 * @brief 应用程序入口函数
 * @return 无
 */
void app_main(void)
{

    // esp_log_level_set("*", ESP_LOG_NONE);  // 关闭所有模块的日志
    xReadWriteSemaphore = xSemaphoreCreateBinary();//nvs读写信号量
    networkToUartQueue = xQueueCreate(networkToUartQueueLen, networkToUartQueueItemSize); // 创建队列
    dataAnalysisQueue = xQueueCreate(dataAnalysisQueueLen, dataAnalysisQueueItemSize); // 创建数据分析队列
    gpioQueue = xQueueCreate(gpioQueueLen, gpioQueueItemSize); // 创建GPIO队列
    //NVS初始化（WIFI底层驱动有用到NVS，所以这里要初始化）
    mynvs_init();
    mynvs_get_config(&wifiConfigInfo); //从NVS中读取WIFI配置 
    
    //wifi工作模式初始化
    wifi_init(wifiConfigInfo);
    uart_init(); //UART初始化
    io_control_init(); //IO控制初始化
    printf("WIFI mode: %d\n", wifiConfigInfo.mode);
    BaseType_t err = xTaskCreate(analysis_data_thread, "analysis_data_thread", 2048, NULL, 14, NULL); //创建数据分析线程
    if(err != pdPASS) 
    {
        ESP_LOGE("app_main", "Failed to create analysis_data_thread");
    }   
    err = xTaskCreatePinnedToCore(uart_thread, "uart_thread", 2048, NULL, 13, NULL, 1); //创建接收线程
    if(err != pdPASS) 
    {
        ESP_LOGE("app_main", "Failed to create uart_thread");
    }  
    err = xTaskCreatePinnedToCore(gpio_thread, "gpio_thread", 2048, NULL, 18, NULL, 1); //创建GPIO线程
    if(err != pdPASS) 
    {
        ESP_LOGE("app_main", "Failed to create gpio_thread");
    }
    vTaskStartScheduler(); // 启动调度器

    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
/**
 * @brief GPIO线程
 * @param arg   用户传递的参数
 * @return 无
 */
void gpio_thread(void *arg)
{
    uint8_t gpio_num = 0;
    uint8_t level = 0;
    uint8_t first_level = 0;
    while(1)
    {
        if(xQueueReceive(gpioQueue, &gpio_num, portMAX_DELAY) == pdTRUE) //从GPIO队列中接收GPIO数据
        {
            vTaskDelay(pdMS_TO_TICKS(10)); // 延时10毫秒，避免过快处理
            level = gpio_get_level(gpio_num); // 获取GPIO电平状态

            ESP_LOGI("GPIO", "GPIO %d level: %d", gpio_num, level);
            switch(gpio_num) 
            {
                case GPIO_NUM_12:
                    if(level == 1) 
                    {
                        ESP_LOGI("GPIO", "GPIO 12 is HIGH");
                        // 执行相应操作
                    } 
                    else 
                    {
                        ESP_LOGI("GPIO", "GPIO 12 is LOW");
                        // 执行相应操作
                    }
                    break;
                case GPIO_NUM_15:
                    first_level = gpio_get_level(GPIO_NUM_12); // 获取GPIO_NUM_12的初始电平状态
                    if(first_level == 1)
                        break; // 如果GPIO_NUM_12为高电平，则不处理GPIO_NUM_15
                    if(level == 1) 
                    {
                        ESP_LOGI("GPIO", "GPIO 15 is HIGH");
                        // 执行相应操作
                    } 
                    else 
                    {
                        ESP_LOGI("GPIO", "GPIO 15 is LOW");
                        // 执行相应操作
                    }
                    break;
                default:
                    ESP_LOGW("GPIO", "Unhandled GPIO: %d", gpio_num);
            }
        }
    }
    vTaskDelete(NULL); // 删除当前任务
}
/**
 * @brief 数据分析线程
 * @param arg   用户传递的参数
 * @return 无
 */
void analysis_data_thread(void *arg)
{
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

            xQueueSend(networkToUartQueue, dataBuffer, 0); //将数据发送到UART队列
        }
        else 
        {
            ESP_LOGE("DataAnalysis", "Failed to receive data from queue");
        }
        memset(dataBuffer, 0, sizeof(dataBuffer)); // 清空缓冲区
    }
    vTaskDelete(NULL); // 删除当前任务
}
/**
 * @brief UART线程
 * @param arg   用户传递的参数
 * @return 无
 */
void uart_thread(void *arg)
{
    uint8_t uartData[1024] = { 0 };
    BaseType_t err;
    while(1)
    {
        err = xQueueReceive(networkToUartQueue, uartData, portMAX_DELAY); //从队列中接收数据
        if(err == pdPASS) 
        {
            // 处理接收到的数据
            ESP_LOGI("UART", "Received data: %s", uartData);
            // 发送数据到UART设备
            uart_write_bytes(UART_NUM_1, (const char *)uartData, strlen((const char *)uartData)); // 发送数据到UART设备
        }
        else 
        {
            ESP_LOGE("UART", "Failed to receive data from queue");
        }
        memset(uartData, 0, sizeof(uartData)); // 清空缓冲区
    }
    vTaskDelete(NULL); // 删除当前任务
}