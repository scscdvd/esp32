#include "uart.h"
#include "driver/uart.h"
#include "driver/gpio.h"

/**
 * @brief UART初始化函数
 * 
 * 该函数用于初始化UART（通用异步收发传输器），设置波特率、数据位、停止位等参数。
 * 
 * @return 无
 */
void uart_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 9600,          // 波特率
        .data_bits = UART_DATA_8_BITS, // 数据位
        .parity = UART_PARITY_DISABLE, // 无奇偶校验
        .stop_bits = UART_STOP_BITS_1, // 停止位
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, // 禁用硬件流控
    };
    
    // 初始化UART1
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, GPIO_NUM_4, GPIO_NUM_5, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, 1024, 1024, 0, NULL, 0));
}