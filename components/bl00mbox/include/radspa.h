#pragma once

// realtime audio developer's simple plugin api
// written from scratch but largely inspired by
// faint memories of the excellent ladspa api

#define RADSPA_SIGNAL_INPUT 0
#define RADSPA_SIGNAL_TYPE_OUTPUT 1
#define RADSPA_SIGNAL_EVENT_TRIGGER 2

#define RADSPA_SIGNAL_HINT_LIN 0
#define RADSPA_SIGNAL_HINT_LOG 1

#if 0
typedef struct _radspa_signal_t(){
    uint16_t type;
    uint16_t hints;
    char name[32];
    _radspa_signal_t * next; //linked list
} radspa_signal_t;

typedef struct _radspa_t(){
    radspa_signal_t * signals;
    

} radspa_t;

#endif
