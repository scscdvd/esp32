#include "io_control.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define INPUT1_EVENT (1ULL << 0)
#define INPUT2_EVENT (1ULL << 1)
/**
 * @brief GPIO中断处理函数
 * @param arg   用户传递的参数
 * @return 无
 */
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t gpio_num = (uint8_t)(uintptr_t)arg; // 获取GPIO编号
    uint32_t value = 0;
    switch(gpio_num)
    {
        case INPUT1:value |= INPUT1_EVENT;break;
        case INPUT2:value |= INPUT2_EVENT;break;
    }
    xTaskNotifyFromISR(gpio_task_handle,value, eSetBits,&xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) //立即切换任务
    {
        portYIELD_FROM_ISR();
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
        .pin_bit_mask = 1ULL << OUTPUT1 | 1ULL << OUTPUT2, // 设置GPIO引脚
        .mode = GPIO_MODE_OUTPUT, // 设置为输出模式
        .pull_up_en = GPIO_PULLUP_DISABLE, // 启用上拉电阻
        .pull_down_en = GPIO_PULLDOWN_ENABLE, // 禁用下拉电阻
        .intr_type = GPIO_INTR_DISABLE, // 禁用中断
     };
     gpio_config_t io_input_conf = 
     {
        .pin_bit_mask = 1ULL << INPUT1 | 1ULL << INPUT2, // 设置GPIO引脚
        .mode = GPIO_MODE_INPUT, // 设置为输入模式
        .pull_up_en = GPIO_PULLUP_DISABLE, // 启用上拉电阻
        .pull_down_en = GPIO_PULLDOWN_ENABLE, // 禁用下拉电阻
        .intr_type = GPIO_INTR_ANYEDGE, // 启用中断
     };
    gpio_config(&io_output_conf); // 应用GPIO配置
    gpio_config(&io_input_conf); // 应用GPIO配置
    gpio_install_isr_service(0);  // 安装 ISR 服务
      // 为每个输入 GPIO 添加中断处理器
    gpio_isr_handler_add(INPUT1, gpio_isr_handler, (void*)INPUT1);
    gpio_isr_handler_add(INPUT2, gpio_isr_handler, (void*)INPUT2);

    gpio_set_level(OUTPUT1, 0);
    gpio_set_level(OUTPUT2, 0);
    ESP_LOGI("IOControl", "IO control initialized successfully");
}


/**
 * @brief GPIO线程
 * @param arg   用户传递的参数
 * @return 无
 */
void gpio_thread(void *arg)
{
    ESP_LOGI("GPIO", "GPIO thread started");
    gpio_task_handle = xTaskGetCurrentTaskHandle();
    uint8_t gpio_num = 0;
    uint8_t level = 0;
    while(1)
    {
        uint32_t events = 0;
        if (xTaskNotifyWait(0, ULONG_MAX,&events,portMAX_DELAY) == pdTRUE)
        {
            vTaskDelay(pdMS_TO_TICKS(10)); // 延时10毫秒，避免过快处理
            if(events & INPUT1_EVENT)//INPUT1
            {
                gpio_num = INPUT1;
                level = gpio_get_level(gpio_num); // 获取GPIO电平状态
                if(level == 1) 
                {
                    ESP_LOGI("GPIO", "%d is HIGH",INPUT1);
                    // 执行相应操作
                } 
                else 
                {
                    ESP_LOGI("GPIO", "%d is LOW",INPUT1);
                    // 执行相应操作
                }
            }
            if((events & INPUT2_EVENT) && (gpio_get_level(INPUT1) == 0))//INPUT2
            {
                gpio_num = INPUT2;
                level = gpio_get_level(gpio_num); // 获取GPIO电平状态
                if(level == 1) 
                {
                    ESP_LOGI("GPIO", "%d is HIGH",INPUT2);
                    // 执行相应操作
                } 
                else 
                {
                    ESP_LOGI("GPIO", "%d is LOW",INPUT2);
                    // 执行相应操作
                }
            }
        }
    }
    vTaskDelete(NULL); // 删除当前任务
}