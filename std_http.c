#include "std_http.h"

#define BUFF_SIZE 1024
#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG

static void http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}


int http_download(char *url, uint8_t *data)
{
    esp_err_t res;

    STD_LOGI("Starting download[%s]", url);

    esp_http_client_config_t config = {
        .url = url,
        .cert_pem = (char *)NULL,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    if(client == NULL)
    {
        STD_LOGE("can not connect http server[%s]", url);
        return -1;
    }
        
    res = esp_http_client_open(client, 0);
    if (res != ESP_OK) {
        esp_http_client_cleanup(client);
        return -1;
    }

    esp_http_client_fetch_headers(client);

    int binary_file_length = 0;
    char *rbuf = PORT_DMA_CALLOC(1, BUFF_SIZE);
    while (1) {
        int data_read = esp_http_client_read(client, rbuf, BUFF_SIZE);
        if (data_read < 0) {
            STD_LOGE("Error: SSL data read error");
            binary_file_length = -1;
        }

        if(data_read == 0)
        {       
            STD_LOGI("Connection closed, all data received");
            STD_LOGI("Total http download data length : %d", binary_file_length);
            break;
        } 

        if(data_read > 0)
        {
            STD_LOGI("http read[%d]", data_read);
            memcpy(data, rbuf, data_read);
            data += data_read;
            binary_file_length += data_read;
        }
    }

    if(binary_file_length <= 0)
    {
        res = -1;
        STD_LOGE("img length error");
        goto FAIL;       
    }
    res = binary_file_length;
//FIXME:may fail, need md5 check

FAIL:
    http_cleanup(client);
    PORT_FREE(rbuf);
    if(res > 0)
        STD_LOGD("http image success");
    else
        STD_LOGE("http image fail");
    return res;
}

// esp_err_t _http_event_handler(esp_http_client_event_t *evt)
// {
//     switch(evt->event_id) {
//         case HTTP_EVENT_ERROR:
//             STD_LOGD("HTTP_EVENT_ERROR");
//             break;
//         case HTTP_EVENT_ON_CONNECTED:
//             STD_LOGD("HTTP_EVENT_ON_CONNECTED");
//             break;
//         case HTTP_EVENT_HEADER_SENT:
//             STD_LOGD("HTTP_EVENT_HEADER_SENT");
//             break;
//         case HTTP_EVENT_ON_HEADER:
//             STD_LOGD("HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
//             break;
//         case HTTP_EVENT_ON_DATA:
//             // STD_LOGD("HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
//             // if (!esp_http_client_is_chunked_response(evt->client)) {
//             //     printf("%.*s", evt->data_len, (char*)evt->data);
//             // }
//             break;
//         case HTTP_EVENT_ON_FINISH:
//             STD_LOGD("HTTP_EVENT_ON_FINISH");
//             break;
//         case HTTP_EVENT_DISCONNECTED:
//             STD_LOGD("HTTP_EVENT_DISCONNECTED");
//             break;
//     }
//     return ESP_OK;
// }


char *http_post(char *url, char *body)
{
    int err;
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = NULL,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, body, strlen(body));
    STD_LOGD("http post body[%s]", body);
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        STD_LOGD("HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    }
    else 
    {
        STD_LOGE("HTTP POST request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    return NULL;
}