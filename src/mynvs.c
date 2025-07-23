#include "mynvs.h"
#include "main.h"
/**
 * @brief NVS初始化函数
 * 
 * 该函数用于初始化NVS（非易失性存储器），如果NVS分区不存在或版本不匹配，则会先擦除NVS分区。
 * 
 * @return 无
 */
void mynvs_init(void)
{
    // NVS初始化
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

esp_err_t mynvs_set_config(Config_t config)
{
    nvs_handle_t handle;
    xSemaphoreTake(xReadWriteSemaphore, portMAX_DELAY);
    esp_err_t err = nvs_open(NVS_PARTITION_NAME, NVS_READWRITE, &handle);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to open NVS partition: %s", esp_err_to_name(err));
        xSemaphoreGive(xReadWriteSemaphore);
        return err;
    }
    err = nvs_set_str(handle, "ssid", (const char*)config.ssid);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to set NVS ssid: %s", esp_err_to_name(err));
        goto exit;
    }
    err = nvs_set_str(handle, "password", (const char*)config.password);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to set NVS password: %s", esp_err_to_name(err));
        goto exit;
    }
    err = nvs_set_u8(handle, "max_connection", config.max_connection);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to set NVS max_connection: %s", esp_err_to_name(err));
        goto exit;
    }

    err = nvs_commit(handle);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to commit NVS changes: %s", esp_err_to_name(err));
        goto exit;
    }
    nvs_close(handle);
    xSemaphoreGive(xReadWriteSemaphore);
    ESP_LOGI("NVS", "Configuration saved successfully");
    return ESP_OK;
exit:
    nvs_close(handle);
    xSemaphoreGive(xReadWriteSemaphore);
    return err;
}

esp_err_t mynvs_get_config(Config_t *config)
{
    nvs_handle_t handle;
    xSemaphoreTake(xReadWriteSemaphore, portMAX_DELAY);
    esp_err_t err = nvs_open(NVS_PARTITION_NAME, NVS_READONLY, &handle);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to open NVS partition: %s", esp_err_to_name(err));
        xSemaphoreGive(xReadWriteSemaphore);
        return err;
    }
    
    size_t ssid_len = sizeof(config->ssid);
    err = nvs_get_str(handle, "ssid", (char*)config->ssid, &ssid_len);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to get NVS ssid: %s", esp_err_to_name(err));
        goto exit;
    }

    size_t password_len = sizeof(config->password);
    err = nvs_get_str(handle, "password", (char*)config->password, &password_len);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to get NVS password: %s", esp_err_to_name(err));
        goto exit;
    }

    err = nvs_get_u8(handle, "max_connection", &config->max_connection);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to get NVS max_connection: %s", esp_err_to_name(err));
        goto exit;
    }
    nvs_close(handle);
    xSemaphoreGive(xReadWriteSemaphore);
    return ESP_OK;
exit:
    nvs_close(handle);
    xSemaphoreGive(xReadWriteSemaphore);
    return err;
}