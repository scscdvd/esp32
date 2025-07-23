#ifndef _MYNVS_H_
#define _MYNVS_H_

#include "mywifi.h"



#define NVS_PARTITION_NAME "config"  //NVS分区名称

void mynvs_init(void);
esp_err_t mynvs_set_config(Config_t config);
esp_err_t mynvs_get_config(Config_t *config);

#endif // _MYNVS_H_
