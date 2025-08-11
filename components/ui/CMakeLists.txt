set(SOURCES screens/ui_home.c
    screens/ui_setting.c
    screens/ui_home1.c
    screens/ui_brightness.c
    screens/ui_systemInfo.c
    screens/ui_weather.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    images/ui_img_back_png.c
    images/ui_img_bizhi_png.c
    images/ui_img_connect_wifi_png.c
    images/ui_img_home_png.c
    images/ui_img_setting_png.c
    images/ui_img_brightness_png.c
    images/ui_img_systemInfo_png.c
    images/ui_img_sprinkle_png.c
    images/ui_img_sun_png.c
    images/ui_img_weather_png.c
    images/ui_img_cloudy_png.c
    ui_events.c

    )

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS .
                 ../../include
                 screens
                 ../../managed_components/lvgl__lvgl
                 ../../main
     REQUIRES driver esp_wifi esp_timer 
)

