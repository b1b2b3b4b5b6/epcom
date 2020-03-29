#include "std_reboot.h"
#include "std_nvs.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG
#define KEY_REBOOT "std_reboot"
#define TASK_PRI (ESP_TASK_MAIN_PRIO + 1)

typedef struct reboot_info_t {
    int times;
}reboot_info_t;


static void reboot_check_task(void *arg)
{
    reboot_info_t reboot_info;
    if(std_nvs_is_exist(KEY_REBOOT))
    {
        std_nvs_load(KEY_REBOOT, &reboot_info, sizeof(reboot_info_t));
        reboot_info.times++;
        STD_LOGW("reboot times: %d", reboot_info.times);
        std_nvs_save(KEY_REBOOT, &reboot_info, sizeof(reboot_info_t));  
        switch(reboot_info.times)
        {
            case 0:
                break;
            case 1:
                break;
            case 2:
                break;
            default:
                STD_LOGW("reboot times to clean all config");
                std_reset(0);
                break;
        }
        vTaskDelay(5000/portTICK_PERIOD_MS);
        reboot_info.times = 0;
        std_nvs_save(KEY_REBOOT, &reboot_info, sizeof(reboot_info_t)); 
    }
    else
    {
        reboot_info.times = 0;
        std_nvs_save(KEY_REBOOT, &reboot_info, sizeof(reboot_info_t));
    }
    
    vTaskDelete(NULL);
}

void std_reset(int time_ms)
{
    STD_LOGW("systemctl will reset after ms[%d]", time_ms);
    vTaskDelay(time_ms/portTICK_PERIOD_MS);
    std_nvs_erase(NULL);
    esp_restart();  
}

void std_reboot(int time_ms)
{
    STD_LOGW("systemctl will restart after ms[%d]", time_ms);
    vTaskDelay(time_ms/portTICK_PERIOD_MS);
    esp_restart();
}

void std_reboot_init()
{
    xTaskCreate(reboot_check_task, "reboot check", 2048, NULL, TASK_PRI, NULL);
}