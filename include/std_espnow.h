

#ifndef STD_ESPNOW_H
#define STD_ESPNOW_H

#include "std_port_common.h"
#include "esp_now.h"
#include "std_wifi.h"

typedef enum espnow_data_type_t{
    ESPNOW_ALL = 0,
    ESPNOW_CONFIG = 1,
    ESPNOW_DEBUG,
    ESPNOW_NORMAL,
    ESPNOW_RLOG,
    ESPNOW_NONE,
}espnow_data_type_t;

typedef struct espnow_data_prefix_t {
    uint8_t type;
}espnow_data_prefix_t;

typedef struct espnow_data_t{
    uint8_t *data;
    uint8_t type;
    int len;
}espnow_data_t;

typedef struct espnow_peer_t{
    uint8_t addr[ESP_NOW_ETH_ALEN];
}espnow_peer_t;

typedef struct espnow_send_t{
    espnow_peer_t *peer;
    espnow_data_t data;
}espnow_send_t;

typedef struct espnow_recv_t{
    uint8_t addr[ESP_NOW_ETH_ALEN];
    uint8_t channel;
    espnow_data_t data;
}espnow_recv_t;

typedef struct espnow_sedack_t{
    uint8_t id;
    uint8_t res;
}espnow_sedack_t;



#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */


espnow_peer_t *std_espnow_add_peer(uint8_t *addr);
int std_espnow_del_peer(espnow_peer_t *target_peer);

espnow_recv_t *std_espnow_read(int type, int block_time_ms);
int std_espnow_write(espnow_send_t *send, bool block);

int std_espnow_deinit(void);
int std_espnow_init();

void std_espnow_select_type(uint8_t type);
void std_espnow_empty();

espnow_send_t *build_espnow_send(uint8_t type, espnow_peer_t *peer, uint8_t *data, uint8_t len);
void free_espnow_send(espnow_send_t *send);

void free_espnow_recv(espnow_recv_t *recv);

bool std_espnow_get_last_status();


#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __MDF_ESPNOW_H__ */
