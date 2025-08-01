#ifndef _MYNVS_H_
#define _MYNVS_H_

#include "mywifi.h"



#define NVS_PARTITION_NAME "config"  //NVS分区名称

void mynvs_init(void);
esp_err_t mynvs_set_config(Config_t config);
esp_err_t mynvs_get_config(Config_t *config);
esp_err_t nvs_save_lcd_calibration_param(long int min_x,long int max_x,long int min_y,long int max_y);
uint8_t nvs_get_lcd_calibration_param(long int *min_x,long int *max_x,long int *min_y,long int *max_y);
#endif // _MYNVS_H_
