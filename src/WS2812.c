#include "WS2812.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

led_strip_handle_t WS2812_init(void)
{
    led_strip_handle_t strip = NULL;

    led_strip_config_t strip_config = 
    {
        .strip_gpio_num = WS2812_GPIO_PIN,
        .max_leds = WS2812_LED_COUNT,
        .flags.invert_out = false,
        .led_model = LED_MODEL_WS2812,  // 使用WS2812模型
    };

    led_strip_rmt_config_t rmt_config = 
    {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };

    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &strip);
    if (ret != ESP_OK) 
    {
        ESP_LOGE("WS2812", "Failed to initialize LED strip");
        return NULL;
    }
    led_strip_clear(strip);
    return strip;
}

void WS2812_color_fade(led_strip_handle_t strip, rgb_t from,rgb_t to, int steps, int delay_ms)
{
    for (int i = 0; i < steps; i++) 
    {
        uint8_t r = from.r + ((to.r - from.r) * i) / steps;
        uint8_t g = from.g + ((to.g - from.g) * i) / steps;
        uint8_t b = from.b + ((to.b - from.b) * i) / steps;

        led_strip_set_pixel(strip, 0, r, g, b);
        led_strip_refresh(strip);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}
