#include "weather.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "esp_log.h"

// static const char aliyun_root_ca[] =
// "-----BEGIN CERTIFICATE-----\n"
// "MIIFUTCCBDmgAwIBAgIQB5g2A63jmQghnKAMJ7yKbDANBgkqhkiG9w0BAQsFADBh\n"
// "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
// "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n"
// "QTAeFw0yMDA3MTYxMjI1MjdaFw0yMzA1MzEyMzU5NTlaMFkxCzAJBgNVBAYTAlVT\n"
// "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxMzAxBgNVBAMTKlJhcGlkU1NMIFRMUyBE\n"
// "ViBSU0EgTWl4ZWQgU0hBMjU2IDIwMjAgQ0EtMTCCASIwDQYJKoZIhvcNAQEBBQAD\n"
// "ggEPADCCAQoCggEBANpuQ1VVmXvZlaJmxGVYotAMFzoApohbJAeNpzN+49LbgkrM\n"
// "Lv2tblII8H43vN7UFumxV7lJdPwLP22qa0sV9cwCr6QZoGEobda+4pufG0aSfHQC\n"
// "QhulaqKpPcYYOPjTwgqJA84AFYj8l/IeQ8n01VyCurMIHA478ts2G6GGtEx0ucnE\n"
// "fV2QHUL64EC2yh7ybboo5v8nFWV4lx/xcfxoxkFTVnAIRgHrH2vUdOiV9slOix3z\n"
// "5KPs2rK2bbach8Sh5GSkgp2HRoS/my0tCq1vjyLJeP0aNwPd3rk5O8LiffLev9j+\n"
// "UKZo0tt0VvTLkdGmSN4h1mVY6DnGfOwp1C5SK0MCAwEAAaOCAgswggIHMB0GA1Ud\n"
// "DgQWBBSkjeW+fHnkcCNtLik0rSNY3PUxfzAfBgNVHSMEGDAWgBQD3lA1VtFMu2bw\n"
// "o+IbG8OXsj3RVTAOBgNVHQ8BAf8EBAMCAYYwHQYDVR0lBBYwFAYIKwYBBQUHAwEG\n"
// "CCsGAQUFBwMCMBIGA1UdEwEB/wQIMAYBAf8CAQAwNAYIKwYBBQUHAQEEKDAmMCQG\n"
// "CCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdpY2VydC5jb20wewYDVR0fBHQwcjA3\n"
// "oDWgM4YxaHR0cDovL2NybDMuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0R2xvYmFsUm9v\n"
// "dENBLmNybDA3oDWgM4YxaHR0cDovL2NybDQuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0\n"
// "yPM8qvlKGxc5T5eHVzV6jpjpyzl6VEKpaxH6gdGVpQVgjkOR9yY9XAUlFnzlOCpq\n"
// "sm7r2ZUKpDfrhUnVzX2nSM15XSj48rVBBAnGJWkLPijlACd3sWFMVUiKRz1C5PZy\n"
// "el2l7J/W4d99KFLSYgoy5GDmARpwLc//fXfkr40nMY8ibCmxCsjXQTe0fJbtrrLL\n"
// "yWQlk9VDV296EI/kQOJNLVEkJ54P\n"
// "-----END CERTIFICATE-----\n";

static const char *TAG = "WEATHER";

void parse_weather_json(const char *json) 
{
    cJSON *root = cJSON_Parse(json);
    if (!root) 
    {
        ESP_LOGE(TAG, "Fail to parse JSON");
        return;
    }
    cJSON *daily = cJSON_GetObjectItem(root, "daily");
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
    }
    printf("获取完成\n");
    cJSON_Delete(root);
}

void get_weather() 
{
    esp_http_client_config_t config = 
    {
        .url = "https://api.seniverse.com/v3/weather/daily.json?key=SYRYfV1htnRIBQ8fe&location=zhengzhou&language=zh-Hans&unit=c&start=0&days=3",
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        // .cert_pem = (const char*)aliyun_root_ca,
        .disable_auto_redirect = false
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Accept-Encoding", "identity");
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) 
    {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Status = %d", status);
        int len = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "Content length = %d", len);
        if (len > 4096) 
        {
            ESP_LOGE(TAG, "Content too large");
            esp_http_client_cleanup(client);
            return;
        }
        char *buf = malloc(len + 1);
        if (!buf) 
        {
            ESP_LOGE(TAG, "malloc fail");
            esp_http_client_cleanup(client);
            return;
        }
        int r = esp_http_client_read(client, buf, len);
        buf[r] = '\0';
        ESP_LOGI(TAG, "Read %d bytes", r);
        parse_weather_json(buf);
        free(buf);
    } 
    else 
    {
        ESP_LOGW(TAG, "GET failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}
