#ifndef STD_DECODE_H
#define STD_DECODE_H

#include "std_common.h"
#include "esp32/rom/tjpgd.h"

esp_err_t std_decode_image(int w, int h, uint8_t *in, uint16_t *out);

#endif