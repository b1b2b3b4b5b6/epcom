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

#include "std_espnow.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_INFO

#define MAX_PACKET 250
#define MAX_SEND_QUEEN   (3)
#define MAX_RECV_QUEEN   (3)
#define TASK_SIZE 3086
#define TASK_PRI (ESP_TASK_MAIN_PRIO + 1)
#define STD_ESPNOW_INTERFACE ESP_IF_WIFI_STA
#define STD_ESPNOW_PMK "pmk6666666666666"
#define STD_ESPNOW_LMK "lmk6666666666666"
#define ESPNOW_CHANNEL 1
#define ENCRYPT_EN false

static bool g_espnow_inited = false;
static int g_send_status = -1;

static QueueHandle_t g_espnow_rcv_queue;
static QueueHandle_t g_espnow_sed_queue;
static SemaphoreHandle_t g_send_rcb;
static espnow_peer_t *g_broadcast_peer;
static uint8_t g_select_type = ESPNOW_NONE;

static SemaphoreHandle_t g_send_lock;

bool std_espnow_get_last_status()
{
    if(g_send_status == ESP_NOW_SEND_SUCCESS)
        return true;
    return false;
}

static void send_lock()
{
    xSemaphoreTake(g_send_lock, portMAX_DELAY);
}

static void send_unlock()
{
    xSemaphoreGive(g_send_lock);
}

espnow_peer_t *std_espnow_add_peer(uint8_t *addr)
{
    PORT_ASSERT(addr != NULL);
    esp_now_peer_info_t *info =  PORT_CALLOC(sizeof(esp_now_peer_info_t), 1);
    memcpy(info->peer_addr, addr, ESP_NOW_ETH_ALEN);

    if(*addr / 2 == 0)
    {
        memcpy(info->lmk, STD_ESPNOW_LMK ,ESP_NOW_KEY_LEN);
        info->encrypt = ENCRYPT_EN;

    }
    else
        info->encrypt = false;
    info->channel = 0;
    info->ifidx = STD_ESPNOW_INTERFACE;
    PORT_ERROR_GOTO_FAIL(esp_now_add_peer(info));
    PORT_FREE(info);
    espnow_peer_t *peer = PORT_CALLOC(sizeof(espnow_peer_t), 1);
    memcpy(peer->addr, addr, ESP_NOW_ETH_ALEN);
    return peer;

FAIL:
    PORT_FREE(info);
    return NULL;
}

int std_espnow_del_peer(espnow_peer_t *target_peer)
{
    PORT_ASSERT(target_peer != NULL);
    PORT_ERROR_GOTO_FAIL(esp_now_del_peer(target_peer->addr));
    PORT_FREE(target_peer);

    return 0;
FAIL:
    return -1;
}

static void espnow_send_cb(const uint8_t *peer_addr, esp_now_send_status_t status)
{
    g_send_status = status;
    xSemaphoreGive(g_send_rcb);
    PORT_LOGM("g_send_status[%d]", status);
}

static void espnow_recv_cb(const uint8_t *src_addr, const uint8_t *src_data, int len)
{
    PORT_LOGV("std esp now receive mac: "MACSTR" len: %d",MAC2STR(src_addr), len);

    switch(*src_data)
    {
        case ESPNOW_RLOG:
            printf("mac["MACSTR"]: %s", MAC2STR(src_addr), src_data + 1);
                return;
            break;
        
        case ESPNOW_DEBUG:
            break;      
        
        default:
            if(src_data[0] != g_select_type)
                return;
            break;
    }


    espnow_recv_t *recv = PORT_CALLOC(sizeof(espnow_recv_t), 1);
    memcpy(recv->addr, src_addr, ESP_NOW_ETH_ALEN);
    wifi_second_chan_t second;
    ESP_ERROR_CHECK(esp_wifi_get_channel(&recv->channel, &second));
    recv->data.len = len -1;
    uint8_t *temp_data = PORT_CALLOC(len - 1, 1);
    memcpy(temp_data, &src_data[1], len - 1);
    recv->data.data = temp_data;
    recv->data.type = src_data[0];

    if(xQueueSend(g_espnow_rcv_queue, &recv, 0) == pdFALSE)
    {
        PORT_LOGE("espnow recv queue overflow");
        free_espnow_recv(recv);
    }
}

static int espnow_write_data(espnow_send_t *send)
{
    uint8_t *addr;
    int res;
    send_lock();
    if(send->peer == NULL)
        addr = g_broadcast_peer->addr;
    else
        addr = send->peer->addr;

    xSemaphoreTake(g_send_rcb, 0);
    res = esp_now_send(addr, send->data.data, send->data.len);
    PORT_ERROR_GOTO_FAIL(res);
    PORT_ASSERT(xSemaphoreTake(g_send_rcb, portMAX_DELAY) == pdTRUE);

    if(g_send_status == ESP_NOW_SEND_SUCCESS)
        res = 0;
    else
        res = -1;
    
FAIL:
    if(res == 0)
        PORT_LOGD("espnow write success, "MACSTR, MAC2STR(addr));
    else
        PORT_LOGE("espnow write fail");

    send_unlock();
    return res;
}

espnow_send_t *build_espnow_send(uint8_t type, espnow_peer_t *peer, uint8_t *data, uint8_t len)
{
    STD_ASSERT(len <= (MAX_PACKET - sizeof(espnow_send_t)));
    espnow_send_t *send = PORT_CALLOC(sizeof(espnow_send_t), 1);
    send->peer = peer;
    send->data.len = len + sizeof(espnow_data_prefix_t);
    send->data.data = PORT_CALLOC(len + sizeof(espnow_data_prefix_t), 1);
    espnow_data_prefix_t *prefix = (espnow_data_prefix_t *)send->data.data;
    prefix->type = type;
    memcpy(&send->data.data[sizeof(espnow_data_prefix_t)], data, len);
    return send;
}

void free_espnow_send(espnow_send_t *send)
{
    if(send == NULL)
        return;
    if(send->data.data == NULL)
    {
        PORT_FREE(send);
        return;
    }
    PORT_FREE(send->data.data);
    PORT_FREE(send);
}

static void espnow_write_task(void *arg)
{
    espnow_send_t *send;
    for(;;)
    {
        if(xQueueReceive(g_espnow_sed_queue, &send, portMAX_DELAY) == pdFALSE)
            continue;

        espnow_write_data(send);
        free_espnow_send(send);
    }
}

int std_espnow_write(espnow_send_t *send, bool block)
{
    if(block)
    {
        return espnow_write_data(send);
    }
    else
    {
        if(xQueueSend(g_espnow_sed_queue, &send, 0) == pdTRUE)
            return 0;
        else
            return -1;
    }    
    return -1;
}


espnow_recv_t *std_espnow_read(int type, int block_time_ms)
{
    espnow_recv_t *recv = NULL;

    if(type == ESPNOW_ALL)
    {
        xQueueReceive(g_espnow_rcv_queue, &recv, block_time_ms / portTICK_PERIOD_MS);
        return recv;
    }

    for(;;)
    {
        if(xQueuePeek(g_espnow_rcv_queue, &recv, block_time_ms / portTICK_PERIOD_MS) == pdTRUE)
        {
            if(recv->data.type == type)
            {
                PORT_ASSERT(xQueueReceive(g_espnow_rcv_queue, &recv, 0) == pdTRUE);
                return recv;
            }     
            else
                vTaskDelay(50/portTICK_PERIOD_MS);      
        }
        else
            break;

    }
    return NULL;
}


int std_espnow_deinit(void)
{
    if (!g_espnow_inited) {
        return ESP_OK;
    }

    ESP_ERROR_CHECK(esp_now_unregister_recv_cb());
    ESP_ERROR_CHECK(esp_now_unregister_send_cb());
    ESP_ERROR_CHECK(esp_now_deinit());
    g_espnow_inited = false;
    return ESP_OK;
}

void std_espnow_select_type(uint8_t type)
{
    g_select_type = type;
}

void std_espnow_empty()
{
    espnow_recv_t *recv;
    for(;;)
    {
        recv = std_espnow_read(ESPNOW_ALL, 100/portTICK_PERIOD_MS);
        if(recv == NULL)
            return;
        free_espnow_recv(recv);
    }
}

int std_espnow_init()
{
    if (g_espnow_inited) {
        return ESP_OK;
    }
    g_send_lock = xSemaphoreCreateBinary();
    xSemaphoreGive(g_send_lock);
    std_wifi_init();
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
    ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)STD_ESPNOW_PMK));

    g_espnow_rcv_queue = xQueueCreate(MAX_RECV_QUEEN, sizeof(espnow_recv_t *));
    g_espnow_sed_queue = xQueueCreate(MAX_SEND_QUEEN, sizeof(espnow_send_t *));
    g_send_rcb = xSemaphoreCreateBinary();
    xSemaphoreTake(g_send_rcb, 0);

    uint8_t addr[6] = {255,255,255,255,255,255};
    g_broadcast_peer = std_espnow_add_peer(addr);

    xTaskCreate(espnow_write_task, "espnow write task", TASK_SIZE, NULL, TASK_PRI, NULL);
    g_espnow_inited = true;
    PORT_LOGI("std esp now init success");
    return ESP_OK;
}


void free_espnow_recv(espnow_recv_t *recv)
{
    if(recv == NULL)
        return;
    if(recv->data.data == NULL)
    {
        PORT_FREE(recv);
        return;
    }

    PORT_FREE(recv->data.data);
    PORT_FREE(recv);
}

#ifdef PRODUCT_TEST

#endif

