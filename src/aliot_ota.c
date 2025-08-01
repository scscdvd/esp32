#include "aliot_ota.h"
#include "mywifi.h"
#include "mbedtls/md.h"
#include "esp_ota_ops.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"
#include "esp_crt_bundle.h"
#include "aliot_dm.h"
#include "ILI9488_lvgl.h"

#define ALIOT_URI "iot-06z00aqatwxu3xb.mqtt.iothub.aliyuncs.com"
#define ALIOT_PORT 8883
#define ALIOT_PRODUCTKEY "k0c11gkzMGP"
#define ALIOT_DEVICENAME "mqtt"
#define ALIOT_DeviceSecret "18f55deb42cc92530a26f627a5df7a94"
#define ALIOT_REGION_ID       "cn-shanghai"
mqtt aliot_mqtt;
static bool s_aliot_connected_flg = false;//连接成功标志位
bool isCurrentUpdata = false;//当前是否正在升级
static char* ota_uri;
static ota_finish_callback_t  s_ota_finish_f = NULL;
#define OTA_URI_SIZE 256 
static void ota_finish_callback(int code)
{
    if(!s_aliot_connected_flg)
        return;
    char topic[128] = { 0 };
    snprintf(topic,sizeof(topic),"/ota/device/progress/%s/%s",
        ALIOT_PRODUCTKEY,ALIOT_DEVICENAME);
    ALIOT_DM_DES* dm_des = aliot_malloc_dm(ALIOT_OTA_PROGRESS);
    if(code == 0)   //OTA成功
    {
        aliot_ota_cancel_rollback();
        aliot_set_dm_ota_progress(dm_des,100,"");
    }
    else            //OTA失败
    {
        aliot_set_dm_ota_progress(dm_des,-1,"update fail!");
    }
    aliot_dm_serialize(dm_des);
    ESP_LOGI("ota_finish_callback topic","%s",topic);
    esp_mqtt_client_publish(aliot_mqtt.client,topic,dm_des->dm_js_str,dm_des->data_len,1,0);
    aliot_free_dm(dm_des);
}
/**
 * OTA升级成功后调用此函数，防止程序回滚
 * @param 无
 * @return 无
 */
void aliot_ota_cancel_rollback(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) 
    {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) 
        {
            esp_ota_mark_app_valid_cancel_rollback();
        }
    }
}
void check_memory() {
    ESP_LOGI("MEM", "Free Heap: %d", esp_get_free_heap_size());
    ESP_LOGI("MEM", "DMA Free: %d", heap_caps_get_free_size(MALLOC_CAP_DMA));
    ESP_LOGI("MEM", "PSRAM Free: %d", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}
/*阿里云回调函数*/
void aliot_mqtt_event_handler(void* arg,esp_event_base_t event_base,int32_t event_id,void* event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch (event_id) 
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI("aliot", "ALIOT_MQTT_EVENT_CONNECTED");
            s_aliot_connected_flg = true;
            snprintf(aliot_mqtt.subscribeTopic,sizeof(aliot_mqtt.subscribeTopic), \
                "/ota/device/upgrade/%s/%s",ALIOT_PRODUCTKEY,ALIOT_DEVICENAME);
            ESP_LOGI("aliot","subtopic:%s",aliot_mqtt.subscribeTopic);
            esp_mqtt_client_subscribe(aliot_mqtt.client, aliot_mqtt.subscribeTopic, 0);
            aliot_report_version();
            aliot_ota_cancel_rollback();
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI("aliot", "ALIOT_MQTT_EVENT_DISCONNECTED");
            esp_mqtt_client_reconnect(aliot_mqtt.client); // 断开连接后尝试重连
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI("aliot", "ALIOT_MQTT_EVENT_DATA: topic=%.*s, data=%.*s",
                     event->topic_len, event->topic,
                     event->data_len, event->data);
            if(strstr(event->topic,"/ota/device/upgrade/"))//ota升级请求
            {
                cJSON* ota_js = cJSON_Parse(event->data);
                if(ota_js)
                {
                    cJSON *data_js = cJSON_GetObjectItem(ota_js,"data");
                    if(data_js)
                    {
                        cJSON *url_js = cJSON_GetObjectItem(data_js,"url");//获取uri这个键值对
                        
                        if(url_js)
                        {
                            static char uri[256] = { 0 };
                            strcpy(uri,url_js ->valuestring);
                            ESP_LOGI("ota_uri","%s",uri);
                            /*启动ota*/
                            aliot_config(uri,ota_finish_callback);
                            aliot_ota_start();
                        }
                    }
                    cJSON_Delete(ota_js);
                }
            }
            break;
        default:
            break;
    }
}
/*ota升级回调*/
void ota_event_handler(void* arg,esp_event_base_t event_base,int32_t event_id,void* event_data)
{
    char* TAG = "ota_event_handler";
    if (event_base == ESP_HTTPS_OTA_EVENT) 
    {
            switch (event_id) 
            {
                case ESP_HTTPS_OTA_START:
                    ESP_LOGI(TAG, "OTA started");
                    break;
                case ESP_HTTPS_OTA_CONNECTED:
                    ESP_LOGI(TAG, "Connected to server");
                    break;
                case ESP_HTTPS_OTA_GET_IMG_DESC:
                    ESP_LOGI(TAG, "Reading Image Description");
                    break;
                case ESP_HTTPS_OTA_VERIFY_CHIP_ID:
                    ESP_LOGI(TAG, "Verifying chip id of new image: %d", *(esp_chip_id_t *)event_data);
                    break;
                case ESP_HTTPS_OTA_DECRYPT_CB:
                    ESP_LOGI(TAG, "Callback to decrypt function");
                    break;
                case ESP_HTTPS_OTA_WRITE_FLASH:
                    ESP_LOGD(TAG, "Writing to flash: %d written", *(int *)event_data);
                    break;
                case ESP_HTTPS_OTA_UPDATE_BOOT_PARTITION:
                    ESP_LOGI(TAG, "Boot partition updated. Next Partition: %d", *(esp_partition_subtype_t *)event_data);
                    break;
                case ESP_HTTPS_OTA_FINISH:
                    ESP_LOGI(TAG, "OTA finish");
                    break;
                case ESP_HTTPS_OTA_ABORT:
                    ESP_LOGI(TAG, "OTA abort");
                    break;
            }
        }
}
static esp_err_t _http_client_init_cb(esp_http_client_handle_t http_client)
{
    esp_err_t err = ESP_OK;
    /* Uncomment to add custom headers to HTTP request */
    // err = esp_http_client_set_header(http_client, "Custom-Header", "Value");
    return err;
}

void https_ota_task(void* arg)
{
    ESP_LOGI("ota_task", "Starting Advanced OTA example");
    ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTPS_OTA_EVENT, ESP_EVENT_ANY_ID, &ota_event_handler, NULL));
    esp_err_t ota_finish_err = ESP_OK;
    esp_http_client_config_t config = {
        .url = ota_uri,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 10*1000,
        .keep_alive_enable = true,
    };
    esp_https_ota_config_t ota_config = {
        .http_config = &config,
        .http_client_init_cb = _http_client_init_cb, // Register a callback to be invoked after esp_http_client is initialized
    };
    ota_finish_err = esp_https_ota(&ota_config);
    if(ota_finish_err == ESP_OK)
    {
        ESP_LOGI("ota_task", "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
        if(s_ota_finish_f)
            s_ota_finish_f(0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        lvgl_free();
        esp_restart();
    }
    else
    {
        ESP_LOGE("ota_task", "ESP_HTTPS_OTA upgrade failed 0x%x", ota_finish_err);
        if(s_ota_finish_f)
            s_ota_finish_f(-1);
        vTaskDelete(NULL);
    }

}
static void calc_hmac_sha256(const char* key, const char* msg, char* out_hex)
{
    unsigned char hmac[32];
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, info, 1);
    mbedtls_md_hmac_starts(&ctx, (const unsigned char*)key, strlen(key));
    mbedtls_md_hmac_update(&ctx, (const unsigned char*)msg, strlen(msg));
    mbedtls_md_hmac_finish(&ctx, hmac);
    mbedtls_md_free(&ctx);

    for (int i = 0; i < 32; i++)
        sprintf(out_hex + i * 2, "%02x", hmac[i]);
}

void start_aliot_mqtt(void)
{
    char sign_source[256] = { 0 };
    const char *timestamp = "2524608000000"; // 固定值
    snprintf(aliot_mqtt.clientID, sizeof(aliot_mqtt.clientID),
             "%s|securemode=3,signmethod=hmacsha256,timestamp=%s|",
             ALIOT_DEVICENAME, timestamp);//拼接clientID
    snprintf(aliot_mqtt.username,sizeof(aliot_mqtt.username),"%s&%s", ALIOT_DEVICENAME, ALIOT_PRODUCTKEY);
    snprintf(sign_source, sizeof(sign_source),
             "clientId%sdeviceName%sproductKey%stimestamp%s",
             ALIOT_DEVICENAME, ALIOT_DEVICENAME, ALIOT_PRODUCTKEY, timestamp);

    calc_hmac_sha256(ALIOT_DeviceSecret, sign_source, aliot_mqtt.password);
    snprintf(aliot_mqtt.uri, sizeof(aliot_mqtt.uri), "mqtts://%s.iot-as-mqtt.%s.aliyuncs.com",
             ALIOT_PRODUCTKEY, ALIOT_REGION_ID);
    aliot_mqtt.port = ALIOT_PORT;
    esp_mqtt_client_config_t mqtt_cfg = 
    {
        .broker.address.uri = aliot_mqtt.uri,
        .broker.address.port = aliot_mqtt.port,
        .credentials = 
        {
            .client_id = aliot_mqtt.clientID,
            .username = aliot_mqtt.username,
            .authentication.password = aliot_mqtt.password,
        },
        .broker.verification.certificate = aliyun_root_cert_pem,
    };
     

    aliot_mqtt.client = esp_mqtt_client_init(&mqtt_cfg);
    /*注册回调函数*/
    esp_mqtt_client_register_event(aliot_mqtt.client, MQTT_EVENT_ANY, aliot_mqtt_event_handler, NULL);
    esp_mqtt_client_start(aliot_mqtt.client);
    ESP_LOGI("aliot","mqtt client start");
}

/**
 * @brief 停止MQTT客户端
 */
void aliot_mqtt_stop(void)
{
    if(aliot_mqtt.client != NULL) // 如果mqtt客户端已存在
    {
        esp_mqtt_client_stop(aliot_mqtt.client); // 停止MQTT客户端
        esp_mqtt_client_destroy(aliot_mqtt.client); // 销毁MQTT客户端
        aliot_mqtt.client = NULL; // 清空mqtt客户端
    }
}

const char* get_app_version(void)
{
    static char app_version[32] = {0};
    if(app_version[0] == 0)
    {
        const esp_partition_t *running = esp_ota_get_running_partition();
        esp_app_desc_t running_app_info;
        esp_ota_get_partition_description(running, &running_app_info);
        snprintf(app_version,sizeof(app_version),"%s",running_app_info.version);
    }
    return app_version;
}
/*阿里云上报版本号*/
void aliot_report_version(void)
{
    ALIOT_DM_DES* ver_dm = aliot_malloc_dm(ALIOT_OTA_VERSION);
    if(ver_dm)
    {
        cJSON* pa_js = cJSON_GetObjectItem(ver_dm->dm_js,"params");
        cJSON_AddStringToObject(pa_js,"version",get_app_version());
        aliot_dm_serialize(ver_dm);
        snprintf(aliot_mqtt.publishTopic,sizeof(aliot_mqtt.publishTopic),\
            "/ota/device/inform/%s/%s",ALIOT_PRODUCTKEY,ALIOT_DEVICENAME);
        ESP_LOGI("ver","%s",ver_dm->dm_js_str);
        esp_mqtt_client_publish(aliot_mqtt.client,aliot_mqtt.publishTopic,ver_dm->dm_js_str,ver_dm->data_len,1,0);
        aliot_free_dm(ver_dm);
    }
}
/*获取下载地址*/
esp_err_t aliot_config(char * uri,ota_finish_callback_t cb)
{
    ota_uri = uri;
    s_ota_finish_f = cb;
    return ESP_OK;
}

esp_err_t aliot_ota_start()
{
    if(isCurrentUpdata == true)
        return ESP_FAIL;
    isCurrentUpdata = true;
    xTaskCreatePinnedToCore(https_ota_task,"https_ota_task",8192,NULL,11,NULL,1);
    return ESP_OK;
}
// aliyun_root_cert_pem.h
static const char *aliyun_root_cert_pem = "-----BEGIN CERTIFICATE-----\n"
"MIID3zCCAsegAwIBAgISfiX6mTa5RMUTGSC3rQhnestIMA0GCSqGSIb3DQEBCwUA\n"
"MHcxCzAJBgNVBAYTAkNOMREwDwYDVQQIDAhaaGVqaWFuZzERMA8GA1UEBwwISGFu\n"
"Z3pob3UxEzARBgNVBAoMCkFsaXl1biBJb1QxEDAOBgNVBAsMB1Jvb3QgQ0ExGzAZ\n"
"BgNVBAMMEkFsaXl1biBJb1QgUm9vdCBDQTAgFw0yMzA3MDQwNjM2NThaGA8yMDUz\n"
"MDcwNDA2MzY1OFowdzELMAkGA1UEBhMCQ04xETAPBgNVBAgMCFpoZWppYW5nMREw\n"
"DwYDVQQHDAhIYW5nemhvdTETMBEGA1UECgwKQWxpeXVuIElvVDEQMA4GA1UECwwH\n"
"Um9vdCBDQTEbMBkGA1UEAwwSQWxpeXVuIElvVCBSb290IENBMIIBIjANBgkqhkiG\n"
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEAoK//6vc2oXhnvJD7BVhj6grj7PMlN2N4iNH4\n"
"GBmLmMdkF1z9eQLjksYc4Zid/FX67ypWFtdycOei5ec0X00m53Gvy4zLGBo2uKgi\n"
"T9IxMudmt95bORZbaph4VK82gPNU4ewbiI1q2loRZEHRdyPORTPpvNLHu8DrYBnY\n"
"Vg5feEYLLyhxg5M1UTrT/30RggHpaa0BYIPxwsKyylQ1OskOsyZQeOyPe8t8r2D4\n"
"RBpUGc5ix4j537HYTKSyK3Hv57R7w1NzKtXoOioDOm+YySsz9sTLFajZkUcQci4X\n"
"aedyEeguDLAIUKiYicJhRCZWljVlZActorTgjCY4zRajodThrQIDAQABo2MwYTAO\n"
"BgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUkWHoKi2h\n"
"DlS1/rYpcT/Ue+aKhP8wHwYDVR0jBBgwFoAUkWHoKi2hDlS1/rYpcT/Ue+aKhP8w\n"
"DQYJKoZIhvcNAQELBQADggEBADrrLcBY7gDXN8/0KHvPbGwMrEAJcnF9z4MBxRvt\n"
"rEoRxhlvRZzPi7w/868xbipwwnksZsn0QNIiAZ6XzbwvIFG01ONJET+OzDy6ZqUb\n"
"YmJI09EOe9/Hst8Fac2D14Oyw0+6KTqZW7WWrP2TAgv8/Uox2S05pCWNfJpRZxOv\n"
"Lr4DZmnXBJCMNMY/X7xpcjylq+uCj118PBobfH9Oo+iAJ4YyjOLmX3bflKIn1Oat\n"
"vdJBtXCj3phpfuf56VwKxoxEVR818GqPAHnz9oVvye4sQqBp/2ynrKFxZKUaJtk0\n"
"7UeVbtecwnQTrlcpWM7ACQC0OO0M9+uNjpKIbksv1s11xu0=\n"
"-----END CERTIFICATE-----\n";

