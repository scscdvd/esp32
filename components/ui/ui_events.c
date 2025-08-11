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