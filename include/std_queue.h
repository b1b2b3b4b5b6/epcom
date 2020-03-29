#ifndef STD_QUEUE_H
#define STD_QUEUE_H

#include "std_port_common.h"

typedef struct queue_data_t{
    int type;
    void *data;
}queue_data_t;

QueueHandle_t std_build_queue(const char *str, int max_queue);
void std_destroy_queue(QueueHandle_t queue);
int std_queue_send(QueueHandle_t queue, int type, void *data, int block_time_ms);
queue_data_t *std_queue_recv(QueueHandle_t queue, int type, int block_time_ms);

#endif