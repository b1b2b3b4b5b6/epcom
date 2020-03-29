/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "std_nvs.h"

#define STD_SPACE_NAME "mcb"
#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG

void std_nvs_init()
{
    static bool init_flag = false;

    if (!init_flag) {
        esp_err_t ret = nvs_flash_init();
        PORT_ASSERT(ret == 0);
        init_flag = true;
    }
}

esp_err_t std_nvs_erase(const char *key)
{


    esp_err_t ret    = ESP_OK;
    nvs_handle handle = 0;

    ret = nvs_open(STD_SPACE_NAME, NVS_READWRITE, &handle);
    PORT_ASSERT(ret == 0);

    if(key == NULL)
        ret = nvs_erase_all(handle);
    else
        ret = nvs_erase_key(handle, key);

    if(ret != ESP_OK)
    {
        PORT_LOGD("nvs error: %s", esp_err_to_name(ret));
        return -1;
    }

    nvs_commit(handle);

    return ESP_OK;
}

ssize_t std_nvs_save(const char *key,  const void *value, size_t length)
{
    PORT_ASSERT(key != NULL);
    PORT_ASSERT(value != NULL);
    PORT_ASSERT(length > 0);

    esp_err_t ret     = ESP_OK;
    nvs_handle handle = 0;

    ret = nvs_open(STD_SPACE_NAME, NVS_READWRITE, &handle);
    PORT_ASSERT(ret == 0);

    ret = nvs_set_blob(handle, key, value, length);
    nvs_commit(handle);
    nvs_close(handle);

    if(ret != ESP_OK)
    {
        PORT_LOGD("nvs error: %s", esp_err_to_name(ret));
        return -1;
    }

    return 0;
}

ssize_t std_nvs_load(const char *key, void *value, size_t length)
{
    PORT_ASSERT(key);
    PORT_ASSERT(value);
    PORT_ASSERT(length > 0);

    esp_err_t ret     = ESP_OK;
    nvs_handle handle = 0;

    ret = nvs_open(STD_SPACE_NAME, NVS_READWRITE, &handle);
    PORT_ASSERT(ret == 0);

    ret = nvs_get_blob(handle, key, value, &length);

    nvs_close(handle);

    if(ret != ESP_OK)
    {
        PORT_LOGD("nvs error: %s", esp_err_to_name(ret));
        return -1;
    }   
        
    return 0;
}

bool std_nvs_is_exist(const char *key)
{
    PORT_ASSERT(key != NULL);

    esp_err_t ret     = ESP_OK;
    nvs_handle handle = 0;
    size_t length     = 0;

    ret = nvs_open(STD_SPACE_NAME, NVS_READWRITE, &handle);
    
    PORT_ASSERT(ret == 0);

    ret = nvs_get_blob(handle, key, NULL, &length);

    nvs_close(handle);

    return ret == ESP_OK ? true : false;
}
