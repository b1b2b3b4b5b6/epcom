#include "std_ota.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG
#define BUFFSIZE 1024
#define KEY_OTA_INFO "ota_info"

#define DEFAULT_SERVER "http://lumochen.cn:3000"
#define DEFAULT_NAME "app.bin"
#define DEFAULT_URL DEFAULT_SERVER"/"DEFAULT_NAME

static char ota_buf[BUFFSIZE + 1] = { 0 };
static char g_ota_url[50] = {0};
static const esp_partition_t *running;
static const esp_partition_t *update_partition;

typedef struct ota_info_t{
    char  version[10];
    int bin_len;
}ota_info_t;

ota_info_t g_ota_info = {0};


esp_partition_t *std_ota_update_partition()
{
    return (esp_partition_t *)update_partition;
}

char *std_ota_version()
{
    static char version[30];
    esp_app_desc_t running_app_info;
    ESP_ERROR_CHECK(esp_ota_get_partition_description(running, &running_app_info));
    memset(version, 0, 30);
    strcpy(version, running_app_info.version);
    return version;
}

char *std_ota_update_version()
{
    return g_ota_info.version;
}

int std_ota_update_len()
{
    return g_ota_info.bin_len;
}

void std_ota_set_url(char *url)
{
    if(url == NULL)
    {
        sprintf(g_ota_url, "%s", DEFAULT_URL);
        g_ota_url[strlen(DEFAULT_URL)] = 0;
        return;
    }

    if(strstr(url, "http") == NULL)
    {
        sprintf(g_ota_url, "http://%s", url);
        g_ota_url[strlen(url) + strlen("http://")] = 0;
    }  
    else
    {
        sprintf(g_ota_url, "%s", url);
        g_ota_url[strlen(url)] = 0;
    }
}

static char *get_ota_url()
{
        return g_ota_url;
}

static void http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

int std_ota_reboot(int time_ms)
{
    int err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) 
    {
        STD_LOGE("esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        return -1;
    }
    std_nvs_erase(KEY_OTA_INFO);
    STD_LOGW("ota set next start for ota");
    if(time_ms >= 0)
    {
        STD_LOGW("system restat after [%d]ms", time_ms);
        vTaskDelay(time_ms / portTICK_PERIOD_MS);
        esp_restart();
    }
    return 0;
}

int std_ota_check_update()
{
    if (esp_ota_set_boot_partition(update_partition) != ESP_OK) 
        return -1;

    ESP_ERROR_CHECK(esp_ota_set_boot_partition(running));
    return 0;
}


int std_ota_http_image()
{
    esp_err_t res;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;


    STD_LOGI("Starting OTA...");

    char *new_version = std_ota_upstream_version();
    if(new_version == NULL)
        return -1;

    esp_http_client_config_t config = {
        .url = get_ota_url(),
        .cert_pem = (char *)NULL,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if(client == NULL)
    {
        STD_LOGE("can not connect http server");
        return -1;
    }
        
    res = esp_http_client_open(client, 0);
    if (res != ESP_OK) {
        esp_http_client_cleanup(client);
        return -1;
    }

    esp_http_client_fetch_headers(client);

    int binary_file_length = 0;
    ESP_ERROR_CHECK(esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle));
    while (1) {
        int data_read = esp_http_client_read(client, ota_buf, BUFFSIZE);
        if (data_read < 0) {
            STD_LOGE("Error: SSL data read error");
            binary_file_length = -1;
        }

        if(data_read == 0)
        {       
            STD_LOGI("Connection closed,all data received");
            STD_LOGI("Total Write binary data length : %d", binary_file_length);
            break;
        } 

        if(data_read > 0)
        {
            ESP_ERROR_CHECK(esp_ota_write(update_handle, (const void *)ota_buf, data_read));
            binary_file_length += data_read;
        }
    }
    res = esp_ota_end(update_handle);
    STD_ERROR_GOTO_FAIL(res);

    if(binary_file_length <= 0)
    {
        res = -1;
        STD_LOGE("binary length error");
        goto FAIL;       
    }

    STD_LOGI("ota image write success!");
    res = std_ota_end(binary_file_length, new_version);

FAIL:
    http_cleanup(client);
    if(res == 0)
        STD_LOGD("http image success");
    else
        STD_LOGE("http image fail");
    return res;
}

int std_ota_end(int bin_len, char *version)
{
    if(esp_ota_set_boot_partition(update_partition) != 0)
        return -1;
    ESP_ERROR_CHECK(esp_ota_set_boot_partition(running));
    memset(&g_ota_info, 0, sizeof(g_ota_info));
    g_ota_info.bin_len = bin_len;
    strcpy(g_ota_info.version, version);
    std_nvs_save(KEY_OTA_INFO, &g_ota_info, sizeof(g_ota_info));
    return 0;
}


int std_ota_update()
{
    char *version = std_ota_upstream_version();
    int res = std_ota_check_version(version);

    if(res == 0)
    {
        std_ota_http_image();
        return std_ota_reboot(0);
    }
    return -1;
}

void std_ota_init(char *url)
{
    std_ota_set_url(url);
    
    static bool init = false;
    if(init)
        return;
    init = true;
    
    running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) != ESP_OK) 
        STD_END("partition error");

    STD_LOGI("Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);    

    STD_LOGI("app name:         %s", running_app_info.project_name);
    STD_LOGI("app version:      %s", running_app_info.version);
    STD_LOGI("app time:         %s", running_app_info.time);
    STD_LOGI("app date:         %s", running_app_info.date);
    STD_LOGI("idf version:      %s", running_app_info.idf_ver);

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    if (configured != running) {
        STD_LOGW("Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        STD_LOGW("(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }

    update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);

    if(std_nvs_is_exist(KEY_OTA_INFO))
    {
        std_nvs_load(KEY_OTA_INFO, &g_ota_info, sizeof(g_ota_info));
    }
    else
    {
        memset(&g_ota_info, 0, sizeof(g_ota_info));
        std_nvs_save(KEY_OTA_INFO, &g_ota_info, sizeof(g_ota_info));
    }
}

char *std_ota_upstream_version()
{
    static char version[20];
    memset(version, 0, 20);
    esp_err_t err;
    esp_http_client_config_t config = {
        .url = get_ota_url(),
        .cert_pem = (char *)NULL,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    STD_ASSERT(client != NULL);

    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        STD_LOGE("Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return NULL;
    }

    esp_http_client_fetch_headers(client);
    int data_read = esp_http_client_read(client, ota_buf, 1024);

    if (data_read < sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
        STD_LOGE("ota bin header read fail");
        http_cleanup(client);
        return NULL;
    } 

    esp_app_desc_t new_app_info;
    memcpy(&new_app_info, &ota_buf[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
    if(strcmp("app", new_app_info.project_name) != 0)
    {
        STD_LOGE("ota bin name illegal");
        http_cleanup(client);
        return NULL;            
    }
    strcpy(version, new_app_info.version);   
    http_cleanup(client);
    return version;
}

int std_ota_check_version(char *version)
{
    if(version == NULL)
        return -1;
    if(strcmp(version, std_ota_version()) == 0)
    {
        STD_LOGW("same version[%s]", version);
        return -1;
    }

    STD_LOGI("new version[%s]", version);
    return 0;
}



#ifdef PRODUCT_TEST

#endif