#include "std_wifi.h"


#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG
#define KEY_WIFI "key_wifi"

const uint8_t WIFI_MESH_MULTICAST_ADDR[] = {0x01, 0x00, 0x5E, 0x00, 0x00, 0x00};
const uint8_t WIFI_MESH_BROADCAST_ADDR[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
const uint8_t WIFI_MESH_ROOT_TODS_ADDR[] = {0x00, 0x00, 0xc0, 0xa8, 0x00, 0x00};
const uint8_t WIFI_MESH_SELF_NODE_ADDR[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static bool g_wifi_init = false;
xSemaphoreHandle g_wifi_lock = NULL;
static uint8_t sta_mac[6] = {0};
static char sta_mac_str[30] = {0};
static bool g_is_connect = false;

void std_wifi_lock()
{
    xSemaphoreTake(g_wifi_lock, portMAX_DELAY);
}

void std_wifi_unlock()
{
    xSemaphoreGive(g_wifi_lock);
}

void std_wifi_init()
{
    if(g_wifi_init == true)
        return;
    tcpip_adapter_init();
    //tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA); //可选，不影响
    //tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP); //可选，不影响
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_event_loop_init(NULL, NULL);
    esp_event_loop_set_cb(NULL, NULL);
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_mesh_set_6m_rate(false));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(DEFAULT_CHANNEL,0));
    esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
    mac2str(sta_mac, sta_mac_str);
    PORT_LOGI("sta mac: %s", sta_mac_str);
    g_wifi_init = true;
    if(g_wifi_lock == NULL)
        g_wifi_lock = xSemaphoreCreateMutex(); 
}


void std_wifi_restore()
{
    STD_ASSERT(g_wifi_init != false);
    std_wifi_disconnect();
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_mesh_set_6m_rate(false));
    ESP_ERROR_CHECK(esp_wifi_set_channel(DEFAULT_CHANNEL,0));
    
}

uint8_t *std_wifi_get_stamac()
{
    return sta_mac;
}

char *std_wifi_get_stamac_str()
{
    return sta_mac_str;
}

bool std_wifi_is_self(mesh_addr_t *mac)
{
    if(memcmp(mac,WIFI_MESH_SELF_NODE_ADDR, 6) == 0)
        return true;

    if(memcmp(mac, sta_mac, 6) == 0)
        return true;
    return false;
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    char ip_str[20] = {0};
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            
            sprintf(ip_str, IPSTR, IP2STR(&event->event_info.got_ip.ip_info.ip));
            std_wifi_set_ip(ip_str);
            PORT_LOGI("station got ip");
            g_is_connect = true;
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            /* This is a workaround as ESP32 WiFi libs don't currently
               auto-reassociate. */
            PORT_LOGI("station disconnect, try to connect");
            esp_wifi_connect();
            g_is_connect = false;
            break;
        case SYSTEM_EVENT_STA_LOST_IP:
            esp_wifi_disconnect();
            esp_wifi_connect();
            break;
        default:
            break;
    }
    return ESP_OK;
}

void std_wifi_connect(char *ssid, char *passwd)
{
    wifi_config_t wifi_config = {0};
    if(ssid == NULL && passwd == NULL)
    {
        if(std_nvs_is_exist(KEY_WIFI))
            std_nvs_load(KEY_WIFI, &wifi_config, sizeof(wifi_config));
        else
            return;
    }
        
    else
    {
        strcpy((char *)wifi_config.sta.ssid, ssid);
        strcpy((char *)wifi_config.sta.password, passwd);
    }

    esp_event_loop_set_cb(wifi_event_handler, NULL);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    esp_wifi_connect();

    std_nvs_save(KEY_WIFI, &wifi_config, sizeof(wifi_config));

}

bool std_wifi_is_connect()
{
    return g_is_connect;
}

int std_wifi_await_connect(int time_s)
{
    STD_LOGI("wait wifi connect");
    if(time_s == 0)
    {
        for(;;)
        {
            if(std_wifi_is_connect())
                return 0;
            vTaskDelay(1000/ portTICK_PERIOD_MS);
        }
    }

    for(int n = 0; n < time_s; n++)
    {
        if(std_wifi_is_connect())
            return 0;
        vTaskDelay(1000/ portTICK_PERIOD_MS);
    }
    return -1;
}

void std_wifi_disconnect()
{
    esp_event_loop_set_cb(NULL, NULL);
    esp_wifi_disconnect();
    g_is_connect = false;
}

static char g_ip[20] = "0.0.0.0";

char *std_wifi_get_ip()
{
    return g_ip;
}

void std_wifi_set_ip(char *ip)
{
    STD_ASSERT(ip);
    memcpy(g_ip, ip, strlen(ip) + 1);
}   