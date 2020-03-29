#include "std_key.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_INFO

#define TASK_SIZE 2048
#define TASK_PRI ESP_TASK_MAIN_PRIO + 5

#define MAX_IO 40
#define QUEUE_SIZE 5
#define CHECK_TOTAL_MS 5000
#define CHECK_PEROID_MS 30

static xQueueHandle g_key_queue = NULL;
static xSemaphoreHandle g_loop_run_sem;

enum {
    STATUS_PRESS,
    STATUS_RELEASE,
}status_type;   

typedef struct key_info_t{
    uint32_t shake_time_ms;
    uint32_t time_stamp_ms;
    int last_status;
    uint32_t press_tims_ms;
    int type;
}key_info_t;

static key_info_t *g_info[MAX_IO];

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(g_loop_run_sem, &xHigherPriorityTaskWoken);
}

key_event_t *std_wait_event(int time_ms)
{
    key_event_t *event;
    if(xQueueReceive(g_key_queue, &event, time_ms / portTICK_PERIOD_MS) == pdTRUE)
        return event;
    else
        return NULL;
}

void std_free_key_event(key_event_t *event)
{
    if(event != NULL)
        free(event);
}

static void press_time_handle(int io)
{
    if(g_info[io]->press_tims_ms > g_info[io]->shake_time_ms)
    {
        key_event_t *event = calloc(1, sizeof(key_event_t));
        event->io  = io;
        event->type = g_info[io]->type;
        event->time_hold_ms = g_info[io]->press_tims_ms;
        xQueueSend(g_key_queue, &event, portMAX_DELAY);
    }
}

static void status_judge(int io)
{
    int val = gpio_get_level(io);
    switch( g_info[io]->last_status)
    {
        case STATUS_PRESS:
            if(val == 0)
            {   
                STD_LOGD("press time ++ [%d]", io);
                g_info[io]->press_tims_ms += CHECK_PEROID_MS ;
            }
            else
            {
                STD_LOGD("press_time_handle[%d]", io);
                press_time_handle(io);
                g_info[io]->last_status = STATUS_RELEASE;
            }
            break;

        case STATUS_RELEASE:
            if(val == 0)
            {
                STD_LOGD("press start[%d]", io);
                g_info[io]->last_status = STATUS_PRESS;
                g_info[io]->press_tims_ms = 0;
            }
            else
            {
                //do nothing
            }

            break;
    }
}

static void loop_task(void *argc)
{
    for(;;)
    {
        xSemaphoreTake(g_loop_run_sem, portMAX_DELAY);
        for(int n = 0; n < CHECK_TOTAL_MS/CHECK_PEROID_MS; n++)
        {
            for(int n = 0; n < MAX_IO; n++)
            {
                if(g_info[n] != NULL)
                    status_judge(n);
            }
            vTaskDelay(CHECK_PEROID_MS / portTICK_PERIOD_MS);
        }
    }
}

void std_key_add(int io, key_event_type_t type, int shake_time)
{
    static bool init = false;
    if(init == false)
    {
        g_key_queue = xQueueCreate(QUEUE_SIZE, sizeof(key_event_t *));
        g_loop_run_sem = xSemaphoreCreateBinary();
        xSemaphoreTake(g_loop_run_sem, 0);
        xTaskCreate(loop_task, "key loop task", TASK_SIZE, NULL, TASK_PRI, NULL);
        init = true;

    }

    g_info[io] = (key_info_t *)STD_CALLOC(1, sizeof(key_info_t));
    g_info[io]->shake_time_ms = shake_time;
    g_info[io]->last_status = STATUS_RELEASE;
    g_info[io]->type = type;

    gpio_config_t io_conf;
	//interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = 1ULL<<io;
    //set as input mode    
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 0;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);
    gpio_set_intr_type(io, GPIO_INTR_ANYEDGE);
	    //install gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(io, gpio_isr_handler, (void *)io);
    STD_LOGI("add key[%d] edge[%d]", io, GPIO_INTR_ANYEDGE);
}