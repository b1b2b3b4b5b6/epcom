#ifndef STD_RDEBUG_H
#define STD_RDEBUG_H

#include "std_port_common.h"
#include "std_espnow.h"
#include "std_rdebug_protocol.h"

void std_rdebug_init(bool control);
void set_global_log_level(int level);

#endif