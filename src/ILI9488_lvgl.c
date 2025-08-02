#include "ILI9488_lvgl.h"
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <driver/spi_master.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_ili9488.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <lvgl.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "mynvs.h"
#include "ui_home.h"
static const char* TAG = "ili9488";

#define LCD_H_RES   320
#define LCD_V_RES   480
#define LCD_SPI_NUM         SPI2_HOST
#define LCD_PIXEL_CLK_HZ    (40 * 1000 * 1000)
#define LCD_CMD_BITS        8
#define LCD_PARAM_BITS      8
#define LCD_COLOR_SPACE     ESP_LCD_COLOR_SPACE_RGB
#define LCD_BITS_PER_PIXEL  18
#define LCD_MAX_TRANSFER_SIZE 32768
#define LCD_QUEUE_LEN 10
#define LVGL_BUFFER_HEIGHT 50
#define LVGL_BUFFER_SIZE (LCD_H_RES * LVGL_BUFFER_HEIGHT)
#define LVGL_UPDATE_PERIOD_MS 5

// Pin definitions
#define LCD_BACKLIGHT_PIN GPIO_NUM_6
#define LCD_MOSI         GPIO_NUM_11
#define LCD_MISO         GPIO_NUM_13
#define LCD_CS           GPIO_NUM_10
#define LCD_CLK          GPIO_NUM_12
#define LCD_DC            GPIO_NUM_9
#define LCD_RST           GPIO_NUM_8

#define TOUCH_CS          GPIO_NUM_10
#define TOUCH_CLK         GPIO_NUM_12
#define TOUCH_DIN         GPIO_NUM_11
#define TOUCH_DO          GPIO_NUM_13
#define TOUCH_IRQ         GPIO_NUM_7

#define BACKLIGHT_FREQ    5000
#define LEDC_SPEED_MODE   LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL      LEDC_CHANNEL_0
#define LEDC_TIMER        LEDC_TIMER_1

// Touch calibration points on screen
static const lv_point_t calib_points_screen[4] = {
    {30, 30},                           // top-left
    {LCD_H_RES - 30, 30},               // top-right
    {LCD_H_RES - 30, LCD_V_RES - 30},  // bottom-right
    {30, LCD_V_RES - 30},               // bottom-left
};

static long int raw_min_x, raw_max_x, raw_min_y, raw_max_y;
static int16_t raw_x[4], raw_y[4];

static SemaphoreHandle_t spi_mutex = NULL;
static spi_device_handle_t touch_spi = NULL;

static esp_lcd_panel_io_handle_t lcd_io_handle = NULL;
static esp_lcd_panel_handle_t lcd_handle = NULL;

static lv_display_t *lv_display = NULL;
static lv_color_t *lv_buf_1 = NULL;
static lv_color_t *lv_buf_2 = NULL;

static lv_indev_t *touch_indev = NULL;

extern TaskHandle_t lvgl_task_handle;

// ======= SPI 初始化 =======
static void ili9488_spi_init(void)
{
    ESP_LOGI(TAG, "Initializing SPI bus");
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = LCD_MOSI,
        .miso_io_num = LCD_MISO,
        .sclk_io_num = LCD_CLK,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = LCD_MAX_TRANSFER_SIZE,
        .flags = SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_MISO | SPICOMMON_BUSFLAG_MOSI | SPICOMMON_BUSFLAG_MASTER,
        .intr_flags = ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_NUM, &bus_cfg, SPI_DMA_CH_AUTO));
}

// ======= 背光控制 =======
static void backlight_init(void)
{
    ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_SPEED_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = BACKLIGHT_FREQ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t channel_cfg = {
        .gpio_num = LCD_BACKLIGHT_PIN,
        .speed_mode = LEDC_SPEED_MODE,
        .channel = LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
        .flags = {0},
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_cfg));
}

static void backlight_set_brightness(int brightness_percent)
{
    if (brightness_percent > 100) brightness_percent = 100;
    if (brightness_percent < 0) brightness_percent = 0;

    uint32_t duty = (brightness_percent * ((1 << 10) - 1)) / 100;  // 10-bit duty
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_SPEED_MODE, LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_SPEED_MODE, LEDC_CHANNEL));
}

// ======= 触摸读取 =======
static bool touch_raw_read(int16_t *x, int16_t *y)
{
    if (!spi_mutex || !touch_spi) return false;

    uint8_t tx[3], rx[3];
    spi_transaction_t t = {.length = 8 * 3, .tx_buffer = tx, .rx_buffer = rx};

    xSemaphoreTake(spi_mutex, portMAX_DELAY);

    // 读取Y轴
    tx[0] = 0x90;
    spi_device_polling_transmit(touch_spi, &t);
    int16_t y_raw = ((rx[1] << 8) | rx[2]) >> 3;

    // 读取X轴
    tx[0] = 0xD0;
    spi_device_polling_transmit(touch_spi, &t);
    int16_t x_raw = ((rx[1] << 8) | rx[2]) >> 3;

    xSemaphoreGive(spi_mutex);

    *x = x_raw;
    *y = y_raw;
    return true;
}

// ======= 触摸校准 =======
static void touch_calibration_run(void)
{
    lv_obj_clean(lv_scr_act());
    ESP_LOGI(TAG, "Starting touch calibration");

    for (int i = 0; i < 4; i++) {
        lv_obj_clean(lv_scr_act());

        lv_obj_t *dot = lv_btn_create(lv_scr_act());
        lv_obj_set_size(dot, 16, 16);
        lv_obj_set_style_radius(dot, 8, 0);
        lv_obj_set_style_bg_color(dot, lv_color_hex(0xFF0000), 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_align(dot, LV_ALIGN_TOP_LEFT, calib_points_screen[i].x - 8, calib_points_screen[i].y - 8);

        bool pressed = false;
        int32_t x_sum = 0, y_sum = 0;
        int sample_cnt = 0;

        while (!pressed) {
            lv_timer_handler();
            vTaskDelay(pdMS_TO_TICKS(20));
            if (gpio_get_level(TOUCH_IRQ) == 0) {
                int16_t x, y;
                if (touch_raw_read(&x, &y)) {
                    x_sum += x;
                    y_sum += y;
                    sample_cnt++;
                }
            } else if (sample_cnt > 0) {
                raw_x[i] = x_sum / sample_cnt;
                raw_y[i] = y_sum / sample_cnt;
                pressed = true;
                ESP_LOGI(TAG, "Calibration point %d: raw_x=%d raw_y=%d", i, raw_x[i], raw_y[i]);
                vTaskDelay(pdMS_TO_TICKS(1200));
            }
        }
    }

    raw_min_x = raw_x[0] < raw_x[3] ? raw_x[0] : raw_x[3];
    raw_max_x = raw_x[1] > raw_x[2] ? raw_x[1] : raw_x[2];
    raw_min_y = raw_y[0] < raw_y[1] ? raw_y[0] : raw_y[1];
    raw_max_y = raw_y[2] > raw_y[3] ? raw_y[2] : raw_y[3];

    printf("\n=== Touch Calibration Result ===\n");
    printf("#define TOUCH_RAW_X_MIN   %ld\n", raw_min_x);
    printf("#define TOUCH_RAW_X_MAX   %ld\n", raw_max_x);
    printf("#define TOUCH_RAW_Y_MIN   %ld\n", raw_min_y);
    printf("#define TOUCH_RAW_Y_MAX   %ld\n", raw_max_y);
    printf("===============================\n\n");
    /*保存校准参数*/
    nvs_save_lcd_calibration_param(raw_min_x,raw_max_x,raw_min_y,raw_max_y);

    lv_obj_clean(lv_scr_act());
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Touch calibration completed!");
    lv_obj_center(label);
    ESP_LOGI("touch","校准完成");
}

// ======= LVGL触摸读取回调 =======
static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    if (!spi_mutex || !touch_spi) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    int16_t x_raw = 0, y_raw = 0;

    xSemaphoreTake(spi_mutex, portMAX_DELAY);

    uint8_t tx[3] = {0}, rx[3] = {0};
    spi_transaction_t t = {
        .length = 8 * 3,
        .tx_buffer = tx,
        .rx_buffer = rx
    };

    tx[0] = 0x90;  // Y read command
    if (spi_device_polling_transmit(touch_spi, &t) == ESP_OK) {
        y_raw = ((rx[1] << 8) | rx[2]) >> 3;
    }

    tx[0] = 0xD0;  // X read command
    if (spi_device_polling_transmit(touch_spi, &t) == ESP_OK) {
        x_raw = ((rx[1] << 8) | rx[2]) >> 3;
    }

    xSemaphoreGive(spi_mutex);

    int x_range = raw_max_x - raw_min_x;
    int y_range = raw_max_y - raw_min_y;
    if (x_range == 0) x_range = 1;
    if (y_range == 0) y_range = 1;

    int x = (x_raw - raw_min_x) * (LCD_H_RES - 1) / x_range;
    int y = (y_raw - raw_min_y) * (LCD_V_RES - 1) / y_range;

    if (x < 0) x = 0;
    else if (x >= LCD_H_RES) x = LCD_H_RES - 1;

    if (y < 0) y = 0;
    else if (y >= LCD_V_RES) y = LCD_V_RES - 1;

    bool touched = (gpio_get_level(TOUCH_IRQ) == 0);

    data->point.x = x;
    data->point.y = y;
    data->state = touched ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}
// ======= 触摸初始化 =======
static void touch_init(void)
{
    ESP_LOGI(TAG, "Initializing touch");

    spi_mutex = xSemaphoreCreateMutex();

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 2 * 1000 * 1000,  // XPT2046 max speed
        .mode = 0,
        .spics_io_num = TOUCH_CS,
        .queue_size = 1,
        // .flags = SPI_DEVICE_HALFDUPLEX,
    };

    ESP_ERROR_CHECK(spi_bus_add_device(LCD_SPI_NUM, &devcfg, &touch_spi));

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << TOUCH_IRQ,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, touch_read_cb);
}

// ======= LVGL显示刷新回调 =======
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
    esp_lcd_panel_draw_bitmap(panel,
                             area->x1, area->y1,
                             area->x2 + 1, area->y2 + 1,
                             (void *)px_map);
}

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                   esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

// ======= 显示驱动初始化 =======
static void display_init(void)
{
    if (!lv_display) {
        ESP_LOGE(TAG, "lv_display not initialized");
        return;
    }

    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = LCD_DC,
        .cs_gpio_num = LCD_CS,
        .pclk_hz = LCD_PIXEL_CLK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = LCD_QUEUE_LEN,
        .on_color_trans_done = notify_lvgl_flush_ready,
        .user_ctx = lv_display,
        .flags = {
            .dc_low_on_data = 0,
            .octal_mode = 0,
        },
    };

    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = LCD_RST,
        .color_space = LCD_COLOR_SPACE,
        .bits_per_pixel = LCD_BITS_PER_PIXEL,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_NUM, &io_cfg, &lcd_io_handle));
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9488(lcd_io_handle, &panel_cfg, LVGL_BUFFER_SIZE, &lcd_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(lcd_handle, false));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(lcd_handle, false));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(lcd_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(lcd_handle, 0, 0));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcd_handle, true));

    lv_display_set_user_data(lv_display, lcd_handle);
}

// ======= LVGL 初始化 =======
static void IRAM_ATTR lvgl_tick_cb(void *param)
{
    lv_tick_inc(LVGL_UPDATE_PERIOD_MS);
}

static void lvgl_init(void)
{
    ESP_LOGI(TAG, "Initializing LVGL");

    lv_init();

    size_t buf_size = LVGL_BUFFER_SIZE * sizeof(lv_color_t);
    lv_buf_1 = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    if (!lv_buf_1) {
        ESP_LOGE(TAG, "Failed to allocate lv_buf_1");
        return;
    }

    lv_buf_2 = NULL; // 不使用双缓冲，简化

    lv_display = lv_display_create(LCD_H_RES, LCD_V_RES);
    if (!lv_display) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        free(lv_buf_1);
        return;
    }

    lv_display_set_flush_cb(lv_display, lvgl_flush_cb);
    lv_display_set_buffers(lv_display, lv_buf_1, lv_buf_2, LVGL_BUFFER_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_color_format(lv_display, LV_COLOR_FORMAT_RGB888);

    const esp_timer_create_args_t timer_args = {
        .callback = &lvgl_tick_cb,
        .name = "lvgl_tick"
    };

    esp_timer_handle_t timer;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, LVGL_UPDATE_PERIOD_MS * 1000));

    touch_init();
}

// ======= 总初始化 =======
void ili9488_lvgl_init(void)
{
    backlight_init();
    backlight_set_brightness(0);

    ili9488_spi_init();

    lvgl_init();
    display_init();

    backlight_set_brightness(75);
}


// ======= 释放资源 =======
void lvgl_free(void)
{
    if (lv_buf_1) {
        free(lv_buf_1);
        lv_buf_1 = NULL;
    }
    if (lv_buf_2) {
        free(lv_buf_2);
        lv_buf_2 = NULL;
    }
    if (touch_spi) {
        spi_bus_remove_device(touch_spi);
        touch_spi = NULL;
    }
    if (spi_mutex) {
        vSemaphoreDelete(spi_mutex);
        spi_mutex = NULL;
    }
    gpio_isr_handler_remove(TOUCH_IRQ);

    if (lv_display) {
        lv_display_delete(lv_display);
        lv_display = NULL;
    }
}

void lvgl_thread(void *arg)
{
    ESP_LOGI(TAG,"lvgl_thread start");
    lvgl_task_handle = xTaskGetCurrentTaskHandle();
    ili9488_lvgl_init();
    if(!nvs_get_lcd_calibration_param(&raw_min_x,&raw_max_x,&raw_min_y,&raw_max_y))//如果没有校准过就校准
        touch_calibration_run();
    // 创建按钮
    lv_obj_t * btn = lv_button_create(lv_screen_active());
    lv_obj_set_size(btn, 100, 50);
    lv_obj_center(btn);

    // 添加按钮标签
    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, "Click Me!");
    while(1) 
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_task_handler();
    }
}