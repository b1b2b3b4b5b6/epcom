#ifndef STD_OTA_H
#define STD_OTA_H

//thread not safe

#include "std_port_common.h"
#include "std_nvs.h"
esp_partition_t *std_ota_update_partition();
int std_ota_check_update();
void std_ota_set_url(char *url);
void std_ota_init(char *url);
int std_ota_http_image();
int std_ota_reboot(int time_ms);
char *std_ota_version();
char *std_ota_update_version();
int std_ota_update();
char *std_ota_upstream_version();
int std_ota_check_version(char *version);
int std_ota_update_len();
int std_ota_end(int bin_len, char *version);
#endif