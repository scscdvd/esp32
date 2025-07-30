#ifndef _WS2812_h
#define _WS2812_h

#include "main.h"

#define WS2812_GPIO_PIN GPIO_NUM_48
#define WS2812_LED_COUNT 1 // WS2812 LED数量
typedef struct 
{
    uint8_t r, g, b;
} rgb_t;
led_strip_handle_t WS2812_init(void);
void WS2812_color_fade(led_strip_handle_t strip, rgb_t from,rgb_t to, int steps, int delay_ms);
#endif // _WS2812_h
