#include "uart.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "data_analysis.h"

/**
 * @brief UART初始化函数
 * 
 * 该函数用于初始化UART（通用异步收发传输器），设置波特率、数据位、停止位等参数。
 * 
 * @return 无
 */
void uart_init(void)
{
    const uart_config_t uart_config = 
    {
        .baud_rate = 9600,          // 波特率
        .data_bits = UART_DATA_8_BITS, // 数据位
        .parity = UART_PARITY_DISABLE, // 无奇偶校验
        .stop_bits = UART_STOP_BITS_1, // 停止位
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, // 禁用硬件流控
    };
    
    // 初始化UART1
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, GPIO_NUM_4, GPIO_NUM_5, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, 1024, 1024, 10, &uartQueue, 0));
    ESP_LOGI("UART", "UART initialized successfully");
}

/**
 * @brief UART线程
 * @param arg   用户传递的参数
 * @return 无
 */
void uart_thread(void *arg)
{
    ESP_LOGI("UART", "UART thread started");
    uart_event_t event;
    BaseType_t err;
    char uartData[1024] = { 0 };
    bool isJson = false;
    char buffer[1024] = { 0 };
    int buffer_pos = 0;
    while(1)
    {
        err = xQueueReceive(uartQueue, &event, portMAX_DELAY); //从队列中接收数据
        if(err == pdPASS) 
        {
            switch(event.type) 
            {
                case UART_DATA: // 接收到数据
                    ESP_LOGI("UART", "Received data from UART");
                    int len = uart_read_bytes(UART_NUM_1, uartData,event.size, 0); // 读取UART数据
                    if(len > 0) 
                    {
                        for(int i = 0; i < len; i++)
                        {
                            buffer[buffer_pos++] = uartData[i];
                            if(uartData[i] == '}')//结尾
                            {
                                buffer[buffer_pos >= 1024 ? 1024 : buffer_pos] = '\0';
                                ESP_LOGI("UART", "len : %d, Data: %s", buffer_pos, buffer);
                                isJson = is_valid_json((const char*)buffer);
                                if(isJson == true)//json数据来获取配置参数
                                {
                                    /*wifi参数配置*/
                                    ESP_LOGI("uart","是Json数据,配置参数");
                                    bool ret = Param_setgetconfig(buffer);
                                    if(ret == false)
                                    {
                                        ESP_LOGI("config","设置获取参数失败");
                                    }
                                    else
                                    {
                                        ESP_LOGI("config","设置获取参数成功");
                                        
                                    }
                                }
                                else
                                {
                                    ESP_LOGI("uart","不是Json数据");
                                    /*其他数据*/
                                }
                                buffer_pos = 0;
                                memset(buffer,0,sizeof(buffer));
                            }
                        }
                    }
                    break;
                case UART_FIFO_OVF:
                    ESP_LOGW("UART", "HW FIFO Overflow");
                    uart_flush_input(UART_NUM_1);     // 清掉 FIFO
                    xQueueReset(uartQueue);          // 清掉事件队列
                    break;

                case UART_BUFFER_FULL:
                    ESP_LOGW("UART", "Ring Buffer Full");
                    uart_flush_input(UART_NUM_1);     // 清掉 FIFO
                    xQueueReset(uartQueue);          // 清掉事件队列
                    break;
                default:
                    ESP_LOGW("UART", "Unhandled UART event type: %d", event.type);
                    uart_flush_input(UART_NUM_1);     // 清掉 FIFO
                    xQueueReset(uartQueue);          // 清掉事件队列
                    break;
            }
        }
        else 
        {
            ESP_LOGE("UART", "Failed to receive data from queue");
        }
        memset(uartData, 0, sizeof(uartData)); // 清空缓冲区
    }
    vTaskDelete(NULL); // 删除当前任务
}

