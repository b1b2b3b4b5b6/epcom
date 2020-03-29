#ifndef STD_HTTP_H
#define STD_HTTP_H

#include "std_port_common.h"

int http_download(char *url, uint8_t *data);
char *http_post(char *url, char *body);

#endif