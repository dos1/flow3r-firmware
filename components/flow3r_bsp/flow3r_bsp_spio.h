#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

void flow3r_bsp_spio_set_sink(void * sink, SemaphoreHandle_t * sink_sema );

typedef struct _flow3r_bsp_spio_sink_t{
    bool app_button_is_left;
    int8_t app_button;
    int8_t os_button;
    bool charger_state;
    bool jacksense_line_in;
    bool fresh_run;
} flow3r_bsp_spio_sink_t;
