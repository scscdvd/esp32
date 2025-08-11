#include "ui.h"
#include "ui_events.h"
#include <time.h>
#include <stdio.h>
#include "esp_sntp.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "ui_weather.h"
#include "weather.h"
#include "ui_home.h"
#include <string.h>


extern void backlight_set_brightness(int brightness_percent);
// build funtions
void update_time_cb(lv_timer_t *timer)
{
    obj_t *obj = (obj_t *)lv_timer_get_user_data(timer);
    // ESP_LOGI("get time","ntp success");
    // 获取当前时间
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // 格式化时间字符串
    char buf[32];
    if(obj->obj_num == 0)
    {
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    }
    else if(obj->obj_num == 1)
    {
        snprintf(buf, sizeof(buf), "%04d/%02d/%02d",
                timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    }
    // 更新标签文本
    lv_label_set_text(obj->obj, buf);
    
}

void update_weather_cb(lv_timer_t *timer)
{
    ESP_LOGI("weathre","update_weather_cb");
    get_weather();
    lv_tabview_rename_tab(ui_TabView1, 0, life_dailey[0].date); // 第 0 个 tab
    lv_tabview_rename_tab(ui_TabView1, 1, life_dailey[1].date); // 第 0 个 tab
    lv_tabview_rename_tab(ui_TabView1, 2, life_dailey[2].date); // 第 0 个 tab
    if(strcmp(life_dailey[0].text_day,"晴") == 0)
    {
        lv_image_set_src(ui_Image18,&ui_img_sun_png);
        // 0x404040
        lv_obj_set_style_image_recolor(ui_Image18, lv_color_hex(0xFFFF00), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(ui_Label18, "sun");

        lv_image_set_src(ui_Image15,&ui_img_sun_png);
        // 0x404040
        lv_obj_set_style_image_recolor(ui_Image15, lv_color_hex(0xFFFF00), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(ui_Label15, "sun");
        ESP_LOGI("daily0","晴天");
    }
    else if(strcmp(life_dailey[0].text_day,"多云") == 0)
    {
        lv_image_set_src(ui_Image18,&ui_img_cloudy_png);
        // 0x404040
        lv_obj_set_style_image_recolor(ui_Image18, lv_color_hex(0x404040), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(ui_Label18, "cloudy");

        lv_image_set_src(ui_Image15,&ui_img_cloudy_png);
        // 0x404040
        lv_obj_set_style_image_recolor(ui_Image15, lv_color_hex(0x404040), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(ui_Label15, "cloudy");
ESP_LOGI("daily0","多云");
    }
    else if(strcmp(life_dailey[0].text_day,"小雨") == 0)
    {
        lv_image_set_src(ui_Image18,&ui_img_sprinkle_png);
        // 0x404040
        lv_obj_set_style_image_recolor(ui_Image18, lv_color_hex(0x404040), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(ui_Label15, "rainy");

        lv_image_set_src(ui_Image15,&ui_img_cloudy_png);
        // 0x404040
        lv_obj_set_style_image_recolor(ui_Image15, lv_color_hex(0x404040), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(ui_Label15, "cloudy");
        ESP_LOGI("daily0","小雨");
    }

    if(strcmp(life_dailey[1].text_day,"晴天") == 0)
    {
        lv_image_set_src(ui_Image16,&ui_img_sun_png);
        // 0x404040
        lv_obj_set_style_image_recolor(ui_Image16, lv_color_hex(0xFFFF00), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(ui_Label16, "sun");
ESP_LOGI("daily1","晴天");
    }
    else if(strcmp(life_dailey[1].text_day,"多云") == 0)
    {
        lv_image_set_src(ui_Image16,&ui_img_cloudy_png);
        // 0x404040
        lv_obj_set_style_image_recolor(ui_Image16, lv_color_hex(0x404040), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(ui_Label16, "cloudy");
ESP_LOGI("daily1","多云");
    }
    else if(strcmp(life_dailey[1].text_day,"小雨") == 0)
    {
        lv_image_set_src(ui_Image16,&ui_img_sprinkle_png);
        // 0x404040
        lv_obj_set_style_image_recolor(ui_Image16, lv_color_hex(0x404040), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(ui_Label16, "rainy");
        ESP_LOGI("daily1","小雨");
    }

    if(strcmp(life_dailey[2].text_day,"晴天") == 0)
    {
        lv_image_set_src(ui_Image17,&ui_img_sun_png);
        // 0x404040
        lv_obj_set_style_image_recolor(ui_Image17, lv_color_hex(0xFFFF00), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(ui_Label17, "sun");
        ESP_LOGI("daily2","晴天");
    }
    else if(strcmp(life_dailey[2].text_day,"多云") == 0)
    {
        lv_image_set_src(ui_Image17,&ui_img_cloudy_png);
        // 0x404040
        lv_obj_set_style_image_recolor(ui_Image17, lv_color_hex(0x404040), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(ui_Label17, "cloudy");
ESP_LOGI("daily2","多云");
    }
    else if(strcmp(life_dailey[2].text_day,"小雨") == 0)
    {
        lv_image_set_src(ui_Image17,&ui_img_sprinkle_png);
        // 0x404040
        lv_obj_set_style_image_recolor(ui_Image17, lv_color_hex(0x404040), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(ui_Label17, "rainy");
        ESP_LOGI("daily2","小雨");
    }
    
}

void system_runtime_cb(lv_timer_t *timer)
{
    lv_obj_t *obj = (lv_obj_t *)lv_timer_get_user_data(timer);
    uint64_t uptime_us = esp_timer_get_time();  // 获取系统运行时间，单位：微秒
    uint64_t uptime_ms = uptime_us / 1000;     // 转换为毫秒
    int hour = uptime_ms/1000/60/60;
    int minutes = (uptime_ms / 1000 / 60) % 60;
    int seconds = (uptime_ms / 1000) % 60;
    char buf[32] = { 0 };
    snprintf(buf,sizeof(buf),"%02d:%02d:%02d",hour,minutes,seconds);
    lv_label_set_text(obj,buf);
}
void brightness_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);  // 获取事件源对象
    int value = lv_slider_get_value(obj);     // 获取滑块的当前值
    if(value == 0)
    {
        _ui_slider_set_property(obj,_UI_SLIDER_PROPERTY_VALUE,50);
        backlight_set_brightness(100);
    }   
    else
    {
        _ui_slider_set_property(obj,_UI_SLIDER_PROPERTY_VALUE,value);
        backlight_set_brightness(value);
    }
        // 打印滑块值
    printf("Slider Value: %d\n", value);
    
}

void wifionOff_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);  // 获取事件源对象
    
    bool state = lv_obj_get_state(obj) & LV_STATE_CHECKED; 

// 打印开关状态
    if (state) 
    {
        printf("Switch is ON\n");
        esp_wifi_start();
    } 
    else 
    {
        printf("Switch is OFF\n");
        // 关闭 WiFi
        // wifi_disable();
        
        esp_wifi_stop();
        
    }
    
}