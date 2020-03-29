#ifndef STD_KEY_H
#define STD_KEY_H

#include "std_port_common.h"

typedef enum key_event_type_t{
    KEY_PRESS,
    KEY_RELEASE,
}key_event_type_t;

typedef struct key_event_t{
    int io;
    int type;
    uint32_t time_stamp_ms;
    uint32_t time_hold_ms;
}key_event_t;

key_event_t *std_wait_event(int time_ms);
void std_key_add(int io, key_event_type_t type, int shake_time);
void std_free_key_event(key_event_t *event);

#endif