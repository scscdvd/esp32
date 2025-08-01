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
    ESP_LOGI("NVS", "NVS initialized successfully");
}
/**
 * @brief 设置WIFI配置到NVS
 * 该函数用于将WIFI配置保存到NVS中。
 * @param config   要保存的WIFI配置
 * @return ESP_OK表示成功，其他表示失败
 */
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
/**
 * @brief 从NVS获取WIFI配置
 * 该函数用于从NVS中读取WIFI配置。
 * @param config   用于存储读取的WIFI配置
 * @return ESP_OK表示成功，其他表示失败
 */
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
    uint16_t tmp;
    err = nvs_get_u16(handle, "mode", &tmp);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to get NVS mode: %s", esp_err_to_name(err));
        goto exit;
    }
    config->mode = tmp;
    size_t ssid_len = sizeof(config->ssid);
    err = nvs_get_str(handle, "ssid", (char*)config->ssid, &ssid_len);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to get NVS ssid: %s", esp_err_to_name(err));
        goto exit;
    }
    size_t deviceIP_len = sizeof(config->deviceIP);
    
    size_t password_len = sizeof(config->password);
    err = nvs_get_str(handle, "password", (char*)config->password, &password_len);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to get NVS password: %s", esp_err_to_name(err));
        goto exit;
    }
    
    
    size_t deviceNetMask_len = sizeof(config->deviceNetMask);
    err = nvs_get_str(handle, "deviceNetMask", (char*)config->deviceNetMask, &deviceNetMask_len);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to get NVS deviceNetMask: %s", esp_err_to_name(err));
        goto exit;
    }

    size_t deviceGateway_len = sizeof(config->deviceGateway);
    err = nvs_get_str(handle, "deviceGateway", (char*)config->deviceGateway, &deviceGateway_len);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to get NVS deviceGateway: %s", esp_err_to_name(err));
        goto exit;
    }

    
    err = nvs_get_u16(handle, "max_connection", &config->max_connection);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to get NVS max_connection: %s", esp_err_to_name(err));
        goto exit;
    }

    err = nvs_get_u16(handle, "dhcpEnable", &config->dhcpEnable);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to get NVS dhcpEnable: %s", esp_err_to_name(err));
        goto exit;
    }
    
    err = nvs_get_u16(handle, "device_port", &config->device_port);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to get NVS device_port: %s", esp_err_to_name(err));
        goto exit;
    }
    err = nvs_get_str(handle, "deviceIP", (char*)config->deviceIP, &deviceIP_len);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to get NVS deviceIP: %s", esp_err_to_name(err));
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

esp_err_t nvs_save_lcd_calibration_param(long int min_x,long int max_x,long int min_y,long int max_y)
{
    nvs_handle_t handle;
    xSemaphoreTake(xReadWriteSemaphore, portMAX_DELAY);
    esp_err_t err = nvs_open("calibration_param", NVS_READWRITE, &handle);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to open NVS partition: %s", esp_err_to_name(err));
        xSemaphoreGive(xReadWriteSemaphore);
        return err;
    }
    err = nvs_set_i32(handle,"min_x",min_x);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to save NVS min_x: %s", esp_err_to_name(err));
        goto err;
    }
    err = nvs_set_i32(handle,"max_x",max_x);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to save NVS max_x: %s", esp_err_to_name(err));
        goto err;
    }
    err = nvs_set_i32(handle,"min_y",min_y);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to save NVS min_y: %s", esp_err_to_name(err));
        goto err;
    }
    
    err = nvs_set_i32(handle,"max_y",max_y);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to save NVS max_y: %s", esp_err_to_name(err));
        goto err;
    }
    err = nvs_set_u8(handle,"isSave",1);//校准参数设置成功
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to save NVS isSave: %s", esp_err_to_name(err));
        goto err;
    }
    nvs_commit(handle);
    nvs_close(handle);
    xSemaphoreGive(xReadWriteSemaphore);
    return ESP_OK;
err:
    nvs_close(handle);
    xSemaphoreGive(xReadWriteSemaphore);
    return ESP_FAIL;
}

uint8_t nvs_get_lcd_calibration_param(long int *min_x,long int *max_x,long int *min_y,long int *max_y)
{
    uint8_t isSave = 0;
    nvs_handle_t handle;
    xSemaphoreTake(xReadWriteSemaphore, portMAX_DELAY);
    esp_err_t err = nvs_open("calibration_param", NVS_READONLY, &handle);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to open NVS partition: %s", esp_err_to_name(err));
        xSemaphoreGive(xReadWriteSemaphore);
        return isSave;
    }
    
    err = nvs_get_i32(handle,"min_x",min_x);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to get NVS min_x: %s", esp_err_to_name(err));
        goto err;
    }
    err = nvs_get_i32(handle,"max_x",max_x);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to get NVS max_x: %s", esp_err_to_name(err));
        goto err;
    }
    err = nvs_get_i32(handle,"min_y",min_y);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to get NVS min_y: %s", esp_err_to_name(err));
        goto err;
    }
    err = nvs_get_i32(handle,"max_y",max_y);
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to get NVS max_y: %s", esp_err_to_name(err));
        goto err;
    }
    err = nvs_get_u8(handle,"isSave",&isSave);//校准参数设置成功
    if(err != ESP_OK) 
    {
        ESP_LOGE("NVS", "Failed to get NVS isSave: %s", esp_err_to_name(err));
        goto err;
    }
    nvs_commit(handle);
    nvs_close(handle);
    xSemaphoreGive(xReadWriteSemaphore);
    return isSave;
err:
    nvs_close(handle);
    xSemaphoreGive(xReadWriteSemaphore);
    return isSave;//失败返回0
}