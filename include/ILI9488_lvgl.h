#ifndef _ILI9488_LVGL_H
#define _ILI9488_LVGL_H

#include "main.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "hal/lcd_types.h"

void ili9488_lvgl_init(void);
void lvgl_free() ;
void backlight_set_brightness(int brightness_percent);
#endif // !_ILI9488_LVGL_H
