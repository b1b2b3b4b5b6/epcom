#include "std_blufi.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_VERBOSE
#define BLUFI_DEVICE_NAME "MCB_LIGHT"

xSemaphoreHandle g_wifi_ok_sem;
data_handle_cb_t g_data_handle_cb;

static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x100,
    .adv_int_max        = 0x100,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static uint8_t service_uuid128[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
};
//static uint8_t manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data =  NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 16,
    .p_service_uuid = service_uuid128,
    .flag = 0x6,
};

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&adv_params);
        break;
    default:
        break;
    }
}

/* connect infor*/
static uint8_t server_if;
static uint16_t conn_id;



static void blufi_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param)
{
    /* actually, should post to blufi_task handle the procedure,
     * now, as a example, we do it more simply */
    switch (event) {
    case ESP_BLUFI_EVENT_INIT_FINISH:
        //STD_LOGD("BLUFI init finish\n");
        esp_ble_gap_set_device_name(BLUFI_DEVICE_NAME);
        esp_ble_gap_config_adv_data(&adv_data);
        break;
    case ESP_BLUFI_EVENT_DEINIT_FINISH:
        //STD_LOGD("BLUFI deinit finish\n");
        break;
    case ESP_BLUFI_EVENT_BLE_CONNECT:
        STD_LOGD("BLUFI ble connect\n");
        server_if = param->connect.server_if;
        conn_id = param->connect.conn_id;
        esp_ble_gap_stop_advertising();
        //esp_blufi_send_custom_data((uint8_t *)str, strlen(str));
        break;
    case ESP_BLUFI_EVENT_BLE_DISCONNECT:
        STD_LOGD("BLUFI ble disconnect\n");

        esp_ble_gap_start_advertising(&adv_params);
        break;

    case ESP_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
        STD_LOGD("blufi close a gatt connection");
        esp_blufi_close(server_if, conn_id);
        break;

    case ESP_BLUFI_EVENT_RECV_CUSTOM_DATA:
        STD_LOGD("Recv Custom Data %d\n", param->custom_data.data_len);
        g_data_handle_cb(param->custom_data.data, param->custom_data.data_len);
        break;

    default:
        break;
    }
}

static esp_blufi_callbacks_t blufi_callbacks = {
    .event_cb = blufi_event_callback,
    .negotiate_data_handler = blufi_dh_negotiate_data_handler,
    .encrypt_func = blufi_aes_encrypt,
    .decrypt_func = blufi_aes_decrypt,
    .checksum_func = blufi_crc_checksum,
};

void std_blufi_init(data_handle_cb_t cb)
{
    static bool init = false;
    if(init)
        return;
    else
        init = true;

    g_data_handle_cb = cb;
    blufi_security_init();
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg); 
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    esp_bluedroid_init();
    esp_bluedroid_enable();
    esp_ble_gap_register_callback(gap_event_handler);//example_gap_event_handler中仅处理ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT事件，并开始广播

    esp_blufi_register_callbacks(&blufi_callbacks); //注册blufi事件处理函数
    esp_blufi_profile_init();//初始化blufi

    ESP_ERROR_CHECK(esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL0, ESP_PWR_LVL_P7));
    ESP_ERROR_CHECK(esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P7));
    STD_LOGD("blufi init success");

}

void std_blufi_deinit()
{
    blufi_security_deinit();
    ESP_ERROR_CHECK(esp_blufi_profile_deinit());
    ESP_ERROR_CHECK(esp_bluedroid_disable());
    ESP_ERROR_CHECK(esp_bluedroid_deinit());
    ESP_ERROR_CHECK(esp_bt_controller_disable());
    ESP_ERROR_CHECK(esp_bt_controller_deinit());
    STD_LOGD("blufi deinit success");
}

#ifdef PRODUCT_TEST

/* 
contiune fail test: memory ok


*/

#endif