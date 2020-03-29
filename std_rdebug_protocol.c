#include "std_rdebug_protocol.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG
#define RLOG_TASK_SIZE 2048
#define RLOG_TASK_PRI (ESP_TASK_MAIN_PRIO + 3)
#define DEBUG_TASK_SIZE 2048
#define DEBUG_TASK_PRI (ESP_TASK_MAIN_PRIO + 3)
#define TARGET_FUNC_MAX_COUNT 20
 
static bool rlog_en = false;
espnow_peer_t *peer = NULL;

int rdebug_send_debug(char *str)
{
    if(peer == NULL)
    {
        PORT_LOGE("no target mac");
        return -1;
    }
    espnow_send_t *send = build_espnow_send(ESPNOW_DEBUG, peer, (uint8_t *)str, strlen(str) + 1);
    PORT_ERROR_GOTO_FAIL(std_espnow_write(send, true));
    free_espnow_send(send);
    return 0;
    
FAIL:
    return -1;
}

int rdebug_set_mac(uint8_t *mac)
{
    int res = -1;
    char *buf = PORT_CALLOC(250, 1);
    PORT_ASSERT(buf != NULL);
    char *arg = PORT_CALLOC(250, 1);
    PORT_ASSERT(arg != NULL);

    if(peer != NULL)
        std_espnow_del_peer(peer);
    peer = std_espnow_add_peer(mac);
    PORT_ASSERT(peer != NULL);


    char *cmd = "set_mac";
    json_pack_object(buf, "cmd", &cmd);
    espnow_send_t *send = build_espnow_send(ESPNOW_DEBUG, peer, (uint8_t *)buf, strlen(buf) + 1);
    for(int n = 1; n <= 13; n++)
    {
        esp_wifi_set_channel(n, 0);
        if(std_espnow_write(send, true))
        {
            res = 0;
            break;
        }
    }
    free_espnow_send(send);
    PORT_FREE(buf);
    PORT_FREE(arg);
    return res;
}

int rdebug_unset_mac(uint8_t *mac)
{
    int res = -1;
    char *buf = PORT_CALLOC(250, 1);
    PORT_ASSERT(buf != NULL);
    char *arg = PORT_CALLOC(250, 1);
    PORT_ASSERT(arg != NULL);

    if(peer != NULL)
        std_espnow_del_peer(peer);
    peer = std_espnow_add_peer(mac);
    PORT_ASSERT(peer != NULL);


    char *cmd = "unset_mac";
    json_pack_object(buf, "cmd", &cmd);
    espnow_send_t *send = build_espnow_send(ESPNOW_DEBUG, peer, (uint8_t *)buf, strlen(buf) + 1);
    for(int n = 1; n <= 13; n++)
    {
        esp_wifi_set_channel(n, 0);
        if(std_espnow_write(send, true))
        {
            res = 0;
            break;
        }
    }
    free_espnow_send(send);
    PORT_FREE(buf);
    PORT_FREE(arg);
    return res;
}

int rdebug_set_log(int level)
{
    if(peer == NULL)
    {
        PORT_LOGE("no target mac");
        return -1;
    }
    int res = -1;
    char *buf = PORT_CALLOC(250, 1);
    PORT_ASSERT(buf != NULL); 
    char *arg = PORT_CALLOC(250, 1);
    PORT_ASSERT(arg != NULL); 


    char *cmd = "set_log";
    json_pack_object(buf, "cmd", &cmd);
    json_pack_object(arg, "log_level", &level);
    json_pack_object(buf, "arg", &arg);
    
    espnow_send_t *send = build_espnow_send(ESPNOW_DEBUG, peer, (uint8_t *)buf, strlen(buf) + 1);
    PORT_ERROR_GOTO_FAIL(std_espnow_write(send, true));
    free_espnow_send(send);
    res = 0;

FAIL:
    PORT_FREE(arg);
    PORT_FREE(arg);
    return res;
}

int target_print(uint8_t *buf, int len)
{
    if(rlog_en)
    {
        espnow_send_t *send = build_espnow_send(ESPNOW_RLOG, peer, buf, len);
        std_espnow_write(send, false);
        return 0;
    }
    else
        return -1;
}

void target_set_log(uint8_t *mac, char *arg)
{
    int log_level;

    json_parse_object(arg, "log_level", &log_level);

    if(memcmp(mac, peer->addr, 6) != 0)
        return;

    STD_GLOBAL_LOG_LEVEL = log_level;
    PORT_LOGI("new log level is %d", log_level);

}

void target_set_mac(uint8_t *mac, char *arg)
{
    if(!rlog_en)
    {
        if(peer != NULL)
            std_espnow_del_peer(peer);
        peer = std_espnow_add_peer(mac);
    }
    rlog_en = true;

    PORT_LOGI("control mac: "MACSTR, MAC2STR(mac));
}

void target_unset_mac(uint8_t *mac, char *arg)
{
    if(peer != NULL)
        std_espnow_del_peer(peer);

    rlog_en = false;

    PORT_LOGI("cancal remote control");
}

static target_func_t target_func_set_mac = {
    .cmd = "set_mac",
    .func = target_set_mac,
};

static target_func_t target_func_set_log = {
    .cmd = "set_log",
    .func = target_set_log,
};

static target_func_t target_func_unset_mac = {
    .cmd = "unset_mac",
    .func = target_unset_mac,
};

static target_func_t *g_target_func[TARGET_FUNC_MAX_COUNT] = {NULL};

void register_target_func(target_func_t *target_func)
{
    for(int n = 0; n < TARGET_FUNC_MAX_COUNT; n++)
    {
        if(g_target_func[n] == NULL)
        {
            g_target_func[n] = target_func;
            return;
        }
    }

    PORT_END("target func count overflow");
}



static void cmd_handle(uint8_t *mac, char *data)
{

    char *cmd = NULL;
    char *arg = NULL;
    json_parse_object(data, "cmd", &cmd);
    PORT_ASSERT(cmd != NULL);

    json_parse_object(data, "arg", &arg);

    int n = 0;
    for(;;)
    {
        if(g_target_func[n]->cmd == NULL)
        {
            PORT_LOGW("undefined rdebug cmd");
            break;
        }
        if(strcmp(g_target_func[n]->cmd, cmd) == 0)
        {
            if(g_target_func[n]->func != NULL)
                g_target_func[n]->func(mac, arg);
            break;
        }
        n++;
    }

    PORT_FREE(cmd);
    PORT_FREE(arg);
}

static void target_receive_debug_task(void *arg)
{
    espnow_recv_t *recv;
    for(;;)
    {
        recv = std_espnow_read(ESPNOW_DEBUG, portMAX_DELAY);
        if(recv != NULL)
        {
            PORT_LOGD("%s\r\n", (char *)recv->data.data);
            cmd_handle(recv->addr, (char *)recv->data.data);
            free_espnow_recv(recv);
        }
    }  
}

void target_create_receive_debug_task()
{
    register_target_func(&target_func_set_mac);
    register_target_func(&target_func_set_log);
    register_target_func(&target_func_unset_mac);
    xTaskCreate(target_receive_debug_task, "debug task", DEBUG_TASK_SIZE, NULL, DEBUG_TASK_PRI, NULL);
}


#ifdef PRODUCT_TEST


#endif