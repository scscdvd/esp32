#include "io_control.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define GPIO_OUTPUT_PIN  ((1ULL<<GPIO_NUM_14) | (1ULL<<GPIO_NUM_13)) // 定义GPIO输出引脚
#define GPIO_INPUT_PIN   ((1ULL<<GPIO_NUM_12) | (1ULL<<GPIO_NUM_15)) // 定义GPIO输入引脚
/**
 * @brief GPIO中断处理函数
 * @param arg   用户传递的参数
 * @return 无
 */
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    uint8_t level = gpio_get_level(gpio_num); // 获取GPIO电平状态
    if(gpio_num == GPIO_NUM_12)
    {
        // 处理 GPIO_NUM_12 的中断
    }
    else if(gpio_num == GPIO_NUM_15)
    {
        // 处理 GPIO_NUM_15 的中断
    }
}
/*
    * @brief 初始化IO控制
    * 该函数配置GPIO引脚为输入输出模式，并安装中断服务。
    * GPIO_NUM_14 和 GPIO_NUM_13 设置为输出引脚，GPIO_NUM_12 和 GPIO_NUM_15 设置为输入引脚。
    
*/
void io_control_init(void)
{
     gpio_config_t io_output_conf = 
     {
        .pin_bit_mask = GPIO_OUTPUT_PIN, // 设置GPIO引脚
        .mode = GPIO_MODE_INPUT_OUTPUT, // 设置为输出模式
        .pull_up_en = GPIO_PULLUP_DISABLE, // 禁用上拉电阻
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // 禁用下拉电阻
        .intr_type = GPIO_INTR_DISABLE, // 禁用中断
     };
     gpio_config_t io_input_conf = 
     {
        .pin_bit_mask = GPIO_INPUT_PIN, // 设置GPIO引脚
        .mode = GPIO_MODE_INPUT, // 设置为输入模式
        .pull_up_en = GPIO_PULLUP_DISABLE, // 禁用上拉电阻
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // 禁用下拉电阻
        .intr_type = GPIO_INTR_ANYEDGE, // 启用中断
     };
    gpio_config(&io_output_conf); // 应用GPIO配置
    gpio_config(&io_input_conf); // 应用GPIO配置
    gpio_install_isr_service(0);  // 安装 ISR 服务
      // 为每个输入 GPIO 添加中断处理器
    gpio_isr_handler_add(GPIO_NUM_12, gpio_isr_handler, (void*)GPIO_NUM_12);
    gpio_isr_handler_add(GPIO_NUM_15, gpio_isr_handler, (void*)GPIO_NUM_15);

    gpio_set_level(GPIO_NUM_14, 0);
    gpio_set_level(GPIO_NUM_13, 0);
}


