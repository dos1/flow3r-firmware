#pragma once
#include <stdbool.h>
#include <stdint.h>

#include <string.h>

#include "flow3r_bsp_spio.h"
#include "st3m_captouch.h"
#include "st3m_imu.h"

typedef struct st3m_input_state_sink_t {
    flow3r_bsp_spio_sink_t buttons;
    st3m_captouch_sink_t captouch;
    st3m_imu_sink_t imu;
} st3m_inputstate_sink_t;

typedef struct _st3m_inputstate_sw_t {
    bool _is_pressed;
    bool _press_event;
    bool _release_event;

    bool ignore;

    bool is_pressed;
    bool press_event;
    bool release_event;
    int32_t pressed_since_ms;
} st3m_inputstate_sw_t;

