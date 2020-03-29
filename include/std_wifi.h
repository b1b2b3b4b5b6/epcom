#ifndef STD_WIFI_H
#define STD_WIFI_H

#include "std_port_common.h"
#include "esp_mesh_internal.h"
#include "std_nvs.h"

#define DEFAULT_CHANNEL 1
#define NOROUTER "NOROUTER"

extern const uint8_t WIFI_MESH_MULTICAST_ADDR[];
extern const uint8_t WIFI_MESH_BROADCAST_ADDR[];
extern const uint8_t WIFI_MESH_ROOT_TODS_ADDR[];
extern const uint8_t WIFI_MESH_SELF_NODE_ADDR[];

void std_wifi_init();
void std_wifi_restore();
void std_wifi_disconnect();
void std_wifi_lock();
void std_wifi_unlock();
uint8_t *std_wifi_get_stamac();
char *std_wifi_get_stamac_str();
bool std_wifi_is_self(mesh_addr_t *mac);
void std_wifi_connect(char *ssid, char *passwd);
bool std_wifi_is_connect();
int std_wifi_await_connect(int time_s);
void std_wifi_store_config();

void std_wifi_set_ip(char *ip);
char *std_wifi_get_ip();

#endif