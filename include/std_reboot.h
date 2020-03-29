#ifndef STD_REBOOT_H
#define STD_REBOOT_H

#include "std_port_common.h"

void std_reboot_init();
void std_reboot(int time_ms);
void std_reset(int time_ms);

#endif