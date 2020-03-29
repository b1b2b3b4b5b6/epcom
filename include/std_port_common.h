
#ifndef STD_PORT_COMMON_H
#define STD_PORT_COMMON_H

#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "esp_mesh.h"

#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"

#include "esp_log.h"
#include "esp_event_loop.h"
#include "esp_partition.h"
#include "esp_system.h"

#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "netdb.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "sys/param.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_heap_caps.h"

#include "std_common.h"
#include "std_json.h"
#include "std_type.h"

#define PORT_LOGM(format, ...) do{\
    if (STD_LOCAL_LOG_LEVEL >= STD_LOG_MARK)\
        printf("\033[35m [%d] %s | line: %d | "format"\033[0m\r\n", esp_log_timestamp(), strrchr(__FILE__, '/') + 1, __LINE__, ##__VA_ARGS__);\
}while(0) 

#define PORT_LOGE(format, ...) do{\
    if (STD_LOCAL_LOG_LEVEL >= STD_LOG_ERROR)\
        printf("\033[31m [%d] %s | line: %d | "format"\033[0m\r\n", esp_log_timestamp(), strrchr(__FILE__, '/') + 1, __LINE__, ##__VA_ARGS__);\
}while(0) 

#define PORT_LOGW(format, ...) do{\
    if (STD_LOCAL_LOG_LEVEL >= STD_LOG_WARN)\
        printf("\033[33m [%d] %s | line: %d | "format"\033[0m\r\n", esp_log_timestamp(), strrchr(__FILE__, '/') + 1, __LINE__, ##__VA_ARGS__);\
}while(0) 

#define PORT_LOGI(format, ...) do{\
    if (STD_LOCAL_LOG_LEVEL >= STD_LOG_INFO)\
        printf("\033[32m [%d] %s | line: %d | "format"\033[0m\r\n", esp_log_timestamp(), strrchr(__FILE__, '/') + 1, __LINE__, ##__VA_ARGS__);\
}while(0) 

#define PORT_LOGD(format, ...) do{\
    if (STD_LOCAL_LOG_LEVEL >= STD_LOG_DEBUG)\
        printf("\033[37m [%d] %s | line: %d | "format"\033[0m\r\n", esp_log_timestamp(), strrchr(__FILE__, '/') + 1, __LINE__ ,##__VA_ARGS__);\
}while(0) 

#define PORT_LOGV(format, ...) do{\
    if (STD_LOCAL_LOG_LEVEL >= STD_LOG_VERBOSE)\
        printf("\033[37m [%d] %s | line: %d | "format"\033[0m\r\n", esp_log_timestamp(), strrchr(__FILE__, '/') + 1, __LINE__ ,##__VA_ARGS__);\
}while(0)


#define PORT_END(format, ...) do{\
	PORT_LOGE(format, ##__VA_ARGS__);\
	vTaskDelay(portMAX_DELAY);\
}while (0)

#define PORT_ASSERT(con) do{\
    if(!(con)){\
            PORT_END("assert fail");\
        }\
    }while(0)

#define PORT_ERROR_GOTO_FAIL(value) do { \
        int _value = value;\
        if (_value != 0) { \
            PORT_LOGD("error jump fail, value: %d", _value); \
            goto FAIL; \
        } \
    }while (0)

#define PORT_CHECK_GOTO_FAIL(con) do { \
        if (!(con)) { \
            PORT_LOGD("check jump fail"); \
            goto FAIL; \
        } \
    }while (0)

#define PORT_ERROR_RETURN(value) do { \
        int _value = value;\
        if (_value != 0) { \
            PORT_LOGD("error return fail, value: %d", _value); \
            return STD_FAIL; \
        } \
    }while (0)

#define PORT_CHECK_RETURN(con) do { \
        if (!(con)) { \
            PORT_LOGD("check return fail"); \
            return STD_FAIL; \
        } \
    }while (0)

#define PORT_ERROR_BREAK(value) \
        int _value = value;\
        if (_value != 0) { \
            PORT_LOGD("error break, value: %d", _value); \
            break; \
        } 

#define PORT_CHECK_BREAK(con) \
        if (!(con)) { \
            PORT_LOGD("check break"); \
            break; \
        }

#define PORT_MALLOC(size) ({void *ptr = malloc(size); \
        if(!ptr){PORT_LOGE("malloc size: %d, ptr: %p, heap free: %d", size, ptr, esp_get_free_heap_size());assert(ptr);} \
        PORT_LOGV("malloc size: %d, ptr: %p, heap free: %d", size, ptr, esp_get_free_heap_size()); ptr;})

#define PORT_CALLOC(n, size) ({void *ptr = calloc(n, size); \
        if(!ptr){PORT_LOGE("calloc size: %d, ptr: %p, heap free: %d", (n) * (size), ptr, esp_get_free_heap_size());assert(ptr);} \
        PORT_LOGV("calloc size: %d, ptr: %p, heap free: %d", (n) * (size), ptr, esp_get_free_heap_size()); ptr;})

#define PORT_DMA_MALLOC(size) ({void *ptr = heap_caps_malloc(size, MALLOC_CAP_DMA); \
        if(!ptr){PORT_LOGE("dma malloc size: %d, ptr: %p, heap free: %d", size, ptr, esp_get_free_heap_size());assert(ptr);} \
        PORT_LOGV("malloc size: %d, ptr: %p, heap free: %d", size, ptr, esp_get_free_heap_size()); ptr;})

#define PORT_DMA_CALLOC(n, size) ({void *ptr = heap_caps_calloc(n, size, MALLOC_CAP_DMA); \
        if(!ptr){PORT_LOGE("dma calloc size: %d, ptr: %p, heap free: %d", (n) * (size), ptr, esp_get_free_heap_size());assert(ptr);} \
        PORT_LOGV("calloc size: %d, ptr: %p, heap free: %d", (n) * (size), ptr, esp_get_free_heap_size()); ptr;})

#define PORT_FREE(ptr) {if(ptr){ free(ptr); \
        PORT_LOGV("free ptr: %p, heap free: %d", ptr, esp_get_free_heap_size()); ptr = NULL;}}while(0)

#define PORT_HEAP() do{\
    PORT_LOGD("heap free: %d", esp_get_free_heap_size());\
}while(0)

/**
 * @brief convert mac address from ap to sta
 */
#define ADDR_AP2STA(mac) do { \
        *((int *)(mac + 2)) = htonl(htonl(*((int *)(mac + 2))) - 1); \
    } while(0)

/**
 * @brief convert mac address from sta to ap
 */
#define ADDR_STA2AP(mac) do { \
        *((int *)(mac + 2)) = htonl(htonl(*((int *)(mac + 2))) + 1); \
    } while(0)

/**
 * @brief convert mac address from bt to sta
 */
#define ADDR_BT2STA(mac) do { \
        *((int *)(mac + 2)) = htonl(htonl(*((int *)(mac + 2))) - 2); \
    } while(0)

#define ADDR_STA2BT(mac) do { \
        *((int *)(mac + 2)) = htonl(htonl(*((int *)(mac + 2))) + 2); \
    } while(0)
/**
 * @brief  convert mac from string format to binary
 */

#define PORT_WAIT_CONDITION(condition, time_s_wait, time_ms_period) ({\
    int result = -1;\
    if(time_s_wait == 0)\
    {\
        for(;;)\
        {\
            if(condition)\
            {\
                result = 0;\
                break;\
            }\
            vTaskDelay(time_ms_period/portTICK_PERIOD_MS);\
        }\
    }\
    else\
    {\
        for(int n = 0; n < time_s_wait * 1000 / time_ms_period; n++)\
        {\
            if(condition)\
            {\
                result = 0;\
                break;\
            }\
            vTaskDelay(time_ms_period/portTICK_PERIOD_MS);\
        }\
    }\
    result;\
})


#endif