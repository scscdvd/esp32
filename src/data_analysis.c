#include "data_analysis.h"
#include "mynvs.h"
#include "driver/uart.h"
#include "esp_system.h"
#include "ILI9488_lvgl.h"
bool is_valid_json(const char *data)
{
    if (data == NULL || strlen(data) == 0)
        return false;

    cJSON *json = cJSON_Parse(data);
    if (json == NULL) 
    {
        return false;
    } 
    else 
    {
         // 只接受 JSON object，过滤掉数字、字符串、true/false/null
        if (!cJSON_IsObject(json) && !cJSON_IsArray(json)) 
        {
            cJSON_Delete(json);
            return false;
        }
        cJSON_Delete(json);  // 释放内存
        return true;
    }
}
static bool mynvs_setWifiInfo(cJSON* root)
{
    cJSON *item = NULL;
    nvs_handle_t handle;
    xSemaphoreTake(xReadWriteSemaphore, portMAX_DELAY);
    esp_err_t err = nvs_open(NVS_PARTITION_NAME, NVS_READWRITE, &handle);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to open NVS partition: %s", esp_err_to_name(err));
        xSemaphoreGive(xReadWriteSemaphore);
        return false;
    }
    cJSON_ArrayForEach(item,root)
    {
        const char* key = item->string;//获取配置参数的键
        ESP_LOGI("key","%s",key);
        if(cJSON_IsString(item))
        {
            ESP_LOGI("value","%s",item->valuestring);
            err = nvs_set_str(handle,key,item->valuestring);
        }  
        else if(cJSON_IsNumber(item))
        {
            err = nvs_set_u16(handle,key,item->valuedouble);
            ESP_LOGI("value","%lf",item->valuedouble);
        }
        if(err != ESP_OK)
            goto exit; 
    }
    err = nvs_commit(handle); //  必须调用
    if(err != ESP_OK)
        goto exit;
    nvs_close(handle);
    xSemaphoreGive(xReadWriteSemaphore);
    return true;
exit:
    nvs_close(handle);
    xSemaphoreGive(xReadWriteSemaphore);
    return false;
}
char *create_json_string()
{
    Config_t config;
    esp_err_t err = mynvs_get_config(&config);
    if(err != ESP_OK)
    {
        ESP_LOGI("get config","获取参数失败");
        return NULL;
    }
    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    } 

    cJSON_AddStringToObject(root, "config", "get");
    cJSON_AddNumberToObject(root, "mode", config.mode);
    cJSON_AddStringToObject(root, "ssid", (char*)config.ssid);
    cJSON_AddStringToObject(root, "password", (char*)config.password);
    cJSON_AddNumberToObject(root, "max_connection", config.max_connection);
    cJSON_AddNumberToObject(root, "device_port", config.device_port);
    cJSON_AddStringToObject(root, "deviceIP", (char*)config.deviceIP);
    cJSON_AddStringToObject(root, "deviceNetMask", (char*)config.deviceNetMask);
    cJSON_AddStringToObject(root, "deviceGateway", (char*)config.deviceGateway);
    cJSON_AddNumberToObject(root, "dhcpEnable", config.dhcpEnable);
    

    char *json_str = cJSON_PrintUnformatted(root);  // 转换为字符串
    cJSON_Delete(root);  // 删除 JSON 对象，释放内存（json_str仍然可用）

    return json_str; // 注意：这个字符串需要手动 free()
} 
bool Param_setgetconfig(char *data)
{
    
    cJSON* Json = cJSON_Parse(data);
    if(Json == NULL)
    {
        return false;
    }
     cJSON *item = cJSON_GetObjectItem(Json, "config");
     if(item == NULL)
     {
        ESP_LOGI("set config","没有这个配置");
        goto exit;
     }
     if(strcmp(item->valuestring,"set") == 0)//设置参数
     {
        ESP_LOGI("set","设置参数");
        bool ret = mynvs_setWifiInfo(Json);
        if(ret == false)
        {
            uart_write_bytes(UART_NUM_1, (const char *)"{\"config\":\"set\",\"ACK\":\"NO\"}", strlen((const char *)"{\"config\":\"set\",\"ACK\":\"NO\"}")); // 发送数据到UART设备
            goto exit; 
        }
        else
        {
            uart_write_bytes(UART_NUM_1, (const char *)"{\"config\":\"set\",\"ACK\":\"OK\"}", strlen((const char *)"{\"config\":\"set\",\"ACK\":\"OK\"}")); // 发送数据到UART设备
            cJSON_Delete(Json);
            lvgl_free();
            esp_restart();
        }
            
     }
     else if(strcmp(item->valuestring,"get") == 0)//配置参数
     {
        ESP_LOGI("get","获取参数");
        char* ret = create_json_string();
        if(ret == NULL)
        {
            uart_write_bytes(UART_NUM_1, (const char *)"{\"config\":\"set\",\"ACK\":\"NO\"}", strlen((const char *)"{\"config\":\"set\",\"ACK\":\"NO\"}")); // 发送数据到UART设备
            goto exit;
        }
        else
        {
            uart_write_bytes(UART_NUM_1,(const char*)ret,strlen(ret));
            free(ret);
        }
        
     }
    cJSON_Delete(Json);
    return true;
exit:
    cJSON_Delete(Json);
    return false;
}
