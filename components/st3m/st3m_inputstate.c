#include "st3m_inputstate.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static SemaphoreHandle_t sink_sema;

static st3m_inputstate_sink_t sink;
static st3m_inputstate_sink_t sink_processing;
static bool _next_run_app_button_is_left;

// the stateful views handed off to applications
static st3m_inputstate_sw_t app_sw[3];
static st3m_inputstate_sw_t os_sw[3];
static st3m_inputstate_sw_t captouch_sw[10];

// the operating system's stateful view on the os button.
static st3m_inputstate_sw_t os_os_sw[3];

static void end_run(){
    xSemaphoreTake(sink_sema, portMAX_DELAY);
    // grab data
    memcpy(&sink_processing, &sink, sizeof(sink));
    // get ready for next round
    sink.buttons.app_button_is_left = _next_run_app_button_is_left;
    sink.captouch.fresh_run = true;
    sink.buttons.fresh_run = true;
    xSemaphoreGive(sink_sema);
}

static void sw_run(st3m_inputstate_sw_t * sw, bool is_pressed, uint32_t delta_t_ms){
    sw->_press_event = !sw->_is_pressed && is_pressed;
    sw->_release_event = sw->_is_pressed && !is_pressed;
    sw->_is_pressed = is_pressed;

    
    if(sw->ignore){
        sw->is_pressed = false;
        sw->press_event = false;
        sw->release_event = false;
        sw->pressed_since_ms = -1;
    } else {
        sw->is_pressed = sw->_is_pressed;
        sw->press_event = sw->_press_event;
        sw->release_event = sw->_release_event;

        if(sw->_press_event){
            sw->pressed_since_ms = 0;
        } else if(sw->_is_pressed){
            if(sw->pressed_since_ms < 99999){
                sw->pressed_since_ms += delta_t_ms;
            }
        } else if(!sw->_is_pressed){
            sw->pressed_since_ms = -1;
        }
    }

    if(sw->_release_event || !sw->_is_pressed) sw->ignore = false;
}

static void tri_sw_run(st3m_inputstate_sw_t * tri_sw, int8_t is_pressed_dir, uint32_t delta_t_ms){
    sw_run(&(tri_sw[0]), is_pressed_dir == -1, delta_t_ms);
    sw_run(&(tri_sw[1]), is_pressed_dir ==  1, delta_t_ms);
    sw_run(&(tri_sw[2]), is_pressed_dir ==  2, delta_t_ms);
}

static void os_sw_run(st3m_inputstate_sw_t * tri_sw, int8_t is_pressed_dir, uint32_t delta_t_ms){
    tri_sw_run(tri_sw, is_pressed_dir, delta_t_ms);
    float step_size = 1.5;
    float repeat_step_size = 2.25;
    float repeat_delay_ms = 800;
    float repeat_rate_ms = 300;

    float audio_adjust_dB = 0;
    if(tri_sw[0].press_event) audio_adjust_dB -= step_size;
    if(tri_sw[2].press_event) audio_adjust_dB += step_size;
    if(tri_sw[0].pressed_since_ms > repeat_delay_ms){
        audio_adjust_dB -= repeat_step_size;
        tri_sw[0].pressed_since_ms -= repeat_rate_ms;
    }
    if(tri_sw[2].pressed_since_ms > repeat_delay_ms){
        audio_adjust_dB += repeat_step_size;
        tri_sw[2].pressed_since_ms -= repeat_rate_ms;
    }

    // TODO set audio volume accordingly, send audio change signal

    if(tri_sw[1].press_event) {} // TODO: send exit application signal
}


void st3m_inputstate_set_app_button_left(bool app_button_is_left){
    _next_run_app_button_is_left = app_button_is_left;
}

void st3m_inputstate_init(void){
    sink.captouch.fresh_run = true;
    sink.buttons.fresh_run = true;
    _next_run_app_button_is_left = sink.buttons.app_button_is_left;

    sink_sema = xSemaphoreCreateMutex();

    flow3r_bsp_spio_set_sink(&(sink.buttons), &sink_sema);
    st3m_captouch_set_sink(&(sink.captouch), &sink_sema);
    st3m_imu_set_sink(&(sink.imu), &sink_sema); 
}

void st3m_inputstate_application_update(uint32_t delta_t_ms){
    end_run();

    st3m_captouch_sink_petal_t * petals = sink_processing.captouch.petals;
    bool petal_is_pressed;
    for(uint8_t i = 0; i < 10; i++){
        petal_is_pressed = petals[i].cw.is_pressed || petals[i].ccw.is_pressed
            || petals[i].base.is_pressed || petals[i].tip.is_pressed;
        sw_run(&(captouch_sw[i]), petal_is_pressed, delta_t_ms);
    }

    tri_sw_run(os_sw, sink_processing.buttons.os_button, delta_t_ms);
    tri_sw_run(app_sw, sink_processing.buttons.app_button, delta_t_ms);

}

void st3m_inputstate_ignore_all(){
    for(uint8_t i = 0; i < 3; i++){
        app_sw[i].ignore = true;
        os_sw[i].ignore = true;
    }
    for(uint8_t i = 0; i < 10; i++){
        captouch_sw[i].ignore = true;
    }
}

void st3m_inputstate_os_update(uint32_t delta_t_ms){
    xSemaphoreTake(sink_sema, portMAX_DELAY);
    int8_t os_os = sink.buttons.os_button;
    xSemaphoreTake(sink_sema, portMAX_DELAY);
    // take care of os responsibilities like volume and application exit
    os_sw_run(os_os_sw, os_os, delta_t_ms);
}
