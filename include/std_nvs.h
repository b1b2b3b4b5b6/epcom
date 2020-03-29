#ifndef STD_NVS_H
#define STD_NVS_H

#include "nvs.h"
#include "nvs_flash.h"
#include "std_port_common.h"

void std_nvs_init();
esp_err_t std_nvs_erase(const char *key);
ssize_t std_nvs_save(const char *key,  const void *value, size_t length);
ssize_t std_nvs_load(const char *key, void *value, size_t length);
bool std_nvs_is_exist(const char *key);

#endif