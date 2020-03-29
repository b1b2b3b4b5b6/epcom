#include "std_queue.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG
#define PEEK_DELAY_MS 50  
#define QUEUE_COUNT 10

typedef struct queue_str_t{
    QueueHandle_t queue;
    const char *str;
}queue_str_t;

static queue_str_t g_queue_str_list[QUEUE_COUNT] = {{NULL}};

QueueHandle_t std_build_queue(const char *str, int max_queue)
{   
    QueueHandle_t queue;
    queue = xQueueCreate(max_queue, sizeof(queue_data_t *));
    STD_ASSERT(queue != NULL);
    int n = 0;
    for(;;)
    {
        if(g_queue_str_list[n].queue == NULL)
        {
            g_queue_str_list[n].queue = queue;
            g_queue_str_list[n].str = str;
        }
        n++;
        if(n == QUEUE_COUNT)
            STD_END("build queue fail, verflow: %s", str);
    }
    return queue;
}

void std_destroy_queue(QueueHandle_t queue)
{
    STD_ASSERT(queue != NULL);
    vQueueDelete(queue);

    int n = 0;
    for(;;)
    {
        if(g_queue_str_list[n].queue == queue)
        {
            g_queue_str_list[n].queue = NULL;
            g_queue_str_list[n].str = NULL;
        }
        n++;
        if(n == QUEUE_COUNT)
            STD_END("destroy queue fail, no found");
    }


}

int std_queue_send(QueueHandle_t queue, int type, void *data, int block_time_ms)
{
    queue_data_t *queue_data = STD_CALLOC(sizeof(queue_data_t *), 1);
    queue_data->type = type;
    queue_data->data = data;
    if(pdTRUE == xQueueSend(queue, &queue_data, block_time_ms)) 
        return 0;
    else
        return -1;
}


queue_data_t *std_queue_recv(QueueHandle_t queue, int type, int block_time_ms)
{
    queue_data_t *recv = NULL;

    if(type == 0)
    {
        xQueueReceive(queue, &recv, block_time_ms / portTICK_PERIOD_MS);
        return recv;
    }

    for(;;)
    {
        if(xQueuePeek(queue, &recv, block_time_ms / portTICK_PERIOD_MS) == pdTRUE)
        {
            if(recv->type == type)
            {
                STD_ASSERT(xQueueReceive(queue, &recv, 0) == pdTRUE);
                return recv;
            }     
            else
                vTaskDelay(PEEK_DELAY_MS/portTICK_PERIOD_MS);      
        }
        else
            break;

    }
    return NULL;
}