#include "weather.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "esp_log.h"


struct daily life_dailey[3] = { 0 };
static const char *TAG = "WEATHER";
static const char* url = "http://api.seniverse.com/v3/weather/daily.json?key=SYRYfV1htnRIBQ8fe&location=zhengzhou&language=zh-Hans&unit=c&start=0&days=3";
void parse_weather_json(const char *json) 
{
    cJSON *root = cJSON_Parse(json);
    if (!root) 
    {
        ESP_LOGE(TAG, "Fail to parse JSON");
        return;
    }
    cJSON* result = cJSON_GetObjectItem(root,"results");
    if(!cJSON_IsArray(result))
    {
        ESP_LOGE(TAG, "Fail to get results");
        cJSON_Delete(root);
        return;
    }
    // 取第一个 result 对象
    cJSON *first_result = cJSON_GetArrayItem(result, 0);
    if (!first_result) 
    {
        ESP_LOGE(TAG, "No first result found");
        cJSON_Delete(root);
        return;
    }
    cJSON *daily = cJSON_GetObjectItem(first_result, "daily");
    if (!cJSON_IsArray(daily)) 
    {
        ESP_LOGE(TAG, "'daily' not found");
        cJSON_Delete(root);
        return;
    }
    for (int i = 0; i < 3 && i < cJSON_GetArraySize(daily); i++) 
    {
        cJSON *day = cJSON_GetArrayItem(daily, i);
        const char* date = cJSON_GetObjectItem(day, "date")->valuestring;
        const char* dayt = cJSON_GetObjectItem(day, "text_day")->valuestring;
        const char* nightt = cJSON_GetObjectItem(day, "text_night")->valuestring;
        const char* high = cJSON_GetObjectItem(day, "high")->valuestring;
        const char* low = cJSON_GetObjectItem(day, "low")->valuestring;
        printf("Day %d:\n 日期：%s\n 白天：%s  夜晚：%s\n 最高：%s℃ 最低：%s℃\n",
               i+1, date, dayt, nightt, high, low);
        
        strncpy(life_dailey[i].date,date,sizeof(life_dailey[i].date));
        strncpy(life_dailey[i].text_day,dayt,sizeof(life_dailey[i].text_day));
        printf("date:%s,text_day:%s\r\n",life_dailey[i].date,life_dailey[i].text_day);
    }
    printf("获取完成\n");
    cJSON_Delete(root);
}

void get_weather() 
{
    
    esp_http_client_config_t config = 
    {
        .url = url,
        .transport_type = HTTP_TRANSPORT_OVER_TCP, // 强制走HTTP
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if(client == NULL)
    {
        ESP_LOGE(TAG,"esp_http_client_init failed");
        return;
    }
    esp_http_client_set_method(client,HTTP_METHOD_GET);
    esp_err_t err = esp_http_client_open(client,0);
    if(err == ESP_OK)
    {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Status = %d", status);
        int len = esp_http_client_fetch_headers(client);
        if(len > 0)
        {
            char *buf = malloc(len + 1);
            if (!buf) 
            {
                ESP_LOGE(TAG, "malloc fail");
                esp_http_client_cleanup(client);
                return;
            }
            int r = esp_http_client_read_response(client, buf, len);
            buf[r] = '\0';
            ESP_LOGI(TAG, "Read %d bytes", r);
            parse_weather_json(buf);
            free(buf);
        }
        else
        {
            ESP_LOGE(TAG, "esp_http_client_fetch_headers fail");
        }
        
    }
    else
    {
        ESP_LOGW(TAG,"esp_http_client_open failed");
    }
    esp_http_client_cleanup(client);
}
