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

typedef struct _st3m_inputstate_captouch_petal_pad_t {
    int32_t raw;
    int32_t threshold;
    bool is_pressed;
} st3m_inputstate_captouch_petal_pad_t;

typedef struct _st3m_inputstate_captouch_petal_pads_t {
    st3m_inputstate_captouch_petal_pad_t cw;
    st3m_inputstate_captouch_petal_pad_t ccw;
    st3m_inputstate_captouch_petal_pad_t base;
    st3m_inputstate_captouch_petal_pad_t tip;
} st3m_inputstate_captouch_petal_pads_t;

typedef struct _st3m_inputstate_captouch_petal_t {
    st3m_inputstate_sw_t sw;
    st3m_inputstate_captouch_petal_pads_t pads;
    float touch_angle;
    float touch_radius;
    float touch_area;
    bool touch_is_calculated;
} st3m_inputstate_captouch_petal_t;

typedef struct _st3m_inputstate_t {
    float pressure;
    float temperature;
    float battery_voltage;
    float acc[x, y, z];
    float gyro[x, y, z];
    st3m_inputstate_captouch_petal_t captouch[10];
    st3m_inputstate_sw_t app_sw[3]; //left, mid, right
    st3m_inputstate_sw_t os_sw[3]; //left, mid, right
} st3m_inputstate_t;
