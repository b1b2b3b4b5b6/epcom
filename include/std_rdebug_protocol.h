#ifndef STD_RDEBUG_PROTOCOL
#define STD_RDEBUG_PROTOCOL

#include "std_port_common.h"
#include "std_espnow.h"
#include "std_json.h"

typedef struct target_func_t{
    char *cmd;
    void (*func)(uint8_t *mac, char *data);
}target_func_t;

int rdebug_set_mac(uint8_t *mac);
int rdebug_set_log(int level);
int rdebug_unset_mac(uint8_t *mac);
int target_print(uint8_t *buf, int len);
void register_target_func(target_func_t *target_func);
void target_create_receive_debug_task();
void target_create_receive_rlog_task();
int rdebug_send_debug(char *str);

#endif