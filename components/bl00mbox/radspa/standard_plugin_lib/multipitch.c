#include "multipitch.h"

radspa_descriptor_t multipitch_desc = {
    .name = "multipitch",
    .id = 37,
    .description = "takes a pitch input and provides a number of shifted outputs."
                    "\ninit_var: number of outputs, default 0, max 127",
    .create_plugin_instance = multipitch_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define NUM_SIGNALS 8
#define INPUT 0
#define THRU 1
#define TRIGGER_IN 2
#define TRIGGER_THRU 3
#define MOD_IN 4
#define MOD_SENS 5
#define MAX_PITCH 6
#define MIN_PITCH 7

#define NUM_MPX 2
#define OUTPUT 8
#define SHIFT 9

typedef struct {
    int16_t trigger_in_prev;
    int16_t trigger_thru_prev;
} multipitch_data_t;

static inline int32_t pitch_limit(int32_t pitch, int32_t min, int32_t max){
    if(pitch > max){
        int32_t estimate = (13981*(pitch - max))>>25;
        pitch -= 2400 * estimate;
        while(pitch > max) pitch -= 2400;
    } else if(pitch < min){
        int32_t estimate = (13891*(min - pitch))>>25;
        pitch += 2400 * estimate;
        while(pitch < max) pitch += 2400;
    }
    return pitch;
}

void multipitch_run(radspa_t * multipitch, uint16_t num_samples, uint32_t render_pass_id){
    uint8_t num_outputs = (multipitch->len_signals - (NUM_SIGNALS))/2;
    multipitch_data_t * data = multipitch->plugin_data;

    uint16_t i;
    int16_t trigger_in = radspa_trigger_get_const(&multipitch->signals[TRIGGER_IN], &data->trigger_in_prev, &i, num_samples, render_pass_id);
    if(trigger_in > 0) radspa_trigger_start(trigger_in, &(data->trigger_thru_prev));
    if(trigger_in < 0) radspa_trigger_stop(&(data->trigger_thru_prev));
    radspa_signal_set_const_value(&multipitch->signals[TRIGGER_THRU], data->trigger_thru_prev);

    int32_t max_pitch = radspa_signal_get_value(&multipitch->signals[MAX_PITCH], 0, render_pass_id);
    int32_t min_pitch = radspa_signal_get_value(&multipitch->signals[MIN_PITCH], 0, render_pass_id);
    if(max_pitch < min_pitch){
        int32_t a = max_pitch;
        max_pitch = min_pitch;
        min_pitch = a;
    }

    int32_t input = radspa_signal_get_value(&multipitch->signals[INPUT], i, render_pass_id);
    int32_t mod = radspa_signal_get_value(&multipitch->signals[MOD_IN], i, render_pass_id);
    mod *= radspa_signal_get_value(&multipitch->signals[MOD_SENS], i, render_pass_id);
    mod = ((int64_t) mod * 76806) >> 32;
    input += mod;

    radspa_signal_set_const_value(&multipitch->signals[THRU], pitch_limit(input, min_pitch, max_pitch));
    for(uint8_t j = 0; j < num_outputs; j++){
        int32_t shift = input + radspa_signal_get_value(&multipitch->signals[(SHIFT) + (NUM_MPX)*j], i, render_pass_id) - RADSPA_SIGNAL_VAL_SCT_A440;
        radspa_signal_set_const_value(&multipitch->signals[(OUTPUT) + (NUM_MPX)*j], pitch_limit(shift, min_pitch, max_pitch));
    }
}

radspa_t * multipitch_create(uint32_t init_var){
    if(init_var > 127) init_var = 127;
    radspa_t * multipitch = radspa_standard_plugin_create(&multipitch_desc, NUM_SIGNALS + 2*init_var, sizeof(multipitch_data_t), 0);
    if(multipitch == NULL) return NULL;
    multipitch->render = multipitch_run;

    radspa_signal_set(multipitch, INPUT, "input", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_SCT, RADSPA_SIGNAL_VAL_SCT_A440);
    radspa_signal_set(multipitch, THRU, "thru", RADSPA_SIGNAL_HINT_OUTPUT | RADSPA_SIGNAL_HINT_SCT, RADSPA_SIGNAL_VAL_SCT_A440);
    radspa_signal_set(multipitch, TRIGGER_IN, "trigger_in", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    radspa_signal_set(multipitch, TRIGGER_THRU, "trigger_thru", RADSPA_SIGNAL_HINT_OUTPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    radspa_signal_set(multipitch, MOD_IN, "mod_in", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(multipitch, MOD_SENS, "mod_sens", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_GAIN, RADSPA_SIGNAL_VAL_UNITY_GAIN);
    radspa_signal_set(multipitch, MAX_PITCH, "max_pitch", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_SCT, RADSPA_SIGNAL_VAL_SCT_A440 + 2400 * 4);
    radspa_signal_set(multipitch, MIN_PITCH, "min_pitch", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_SCT, RADSPA_SIGNAL_VAL_SCT_A440 - 2400 * 4);

    radspa_signal_set_group(multipitch, init_var, NUM_MPX, OUTPUT, "output", RADSPA_SIGNAL_HINT_OUTPUT | RADSPA_SIGNAL_HINT_SCT,
            RADSPA_SIGNAL_VAL_SCT_A440);
    radspa_signal_set_group(multipitch, init_var, NUM_MPX, SHIFT, "shift", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_SCT,
            RADSPA_SIGNAL_VAL_SCT_A440);

    return multipitch;
}
