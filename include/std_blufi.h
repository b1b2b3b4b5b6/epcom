#ifndef NETWORK_BLUFI_H
#define NETWORK_BLUFI_H

#include "esp_bt.h"
#include "esp_blufi_api.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"

#include "std_blufi_security.h"

void std_blufi_init(data_handle_cb_t cb);
//once buufi deinit ,it can not be used again
void std_blufi_deinit();

#endif