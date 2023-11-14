#include "osc.h"

radspa_descriptor_t osc_desc = {
    .name = "osc",
    .id = 420,
    .description = "simple oscillator with waveshaping, linear fm and hardsync",
    .create_plugin_instance = osc_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define OSC_NUM_SIGNALS 9
#define OSC_OUT 0
#define OSC_PITCH 1
#define OSC_WAVEFORM 2
#define OSC_MORPH 3
#define OSC_FM 4
#define OSC_SYNC_IN 5
#define OSC_SYNC_IN_PHASE 6
#define OSC_SYNC_OUT 7
#define OSC_SPEED 8

#pragma GCC push_options
#pragma GCC optimize ("-O3")

/* The oscillator receives a lot of control signals that
 * can be useful either as constant buffers or at full
 * resolution. This results in a lot of if(_const) in the
 * for loop, dragging down performance. We could split
 * the oscillator up in seperate less general purpose
 * plugins, but we wouldn't change the UI anyways so
 * instead we allow ourselves to generate a bunch of
 * code here by manually unswitching the for loop.
 *
 * If all conditionals are removed the code size explodes
 * a bit too much, but we can group them into blocks so that
 * if either element of a block receives a fast signal it takes
 * the performance hit for all of them:
 *
 * pitch, morph and waveform are grouped together since they
 * typically are constant and only need fast signals for ring
 * modulation like effects. We call this the RINGMOD group.
 *
 * fm and sync_in are grouped together for similar reasons
 * into the OSCMOD group.
 *
 * out and sync_out should be switched seperately, but we can
 * ignore the option of them both being disconnected and switch
 * to LFO mode in this case instead. This generates slightly
 * different antialiasing but that is okay, especially since
 * hosts are advised to not render a plugin at all if no
 * output is connected to anything.
 *
 * the index for OSC_MEGASWITCH is defined as follows:
 * ringmod_const + (oscmod_const<<1) + (out_const<<2) + (sync_out_const<<3);
 *
 * this limits the index range to [0..11], a reasonable amount
 * of extra code size in our opinion given the importance of a
 * fast and flexible basic oscillator building block.
 */

#define OSC_MEGASWITCH \
        case 0: \
            for(; i < num_samples; i++){ \
                RINGMOD_READ \
                FM_READ \
                OSCILLATE \
                SYNC_IN_READ \
                OUT_WRITE \
                SYNC_OUT_WRITE \
            } \
            break; \
        case 1: \
            for(; i < num_samples; i++){ \
                FM_READ \
                OSCILLATE \
                SYNC_IN_READ \
                OUT_WRITE \
                SYNC_OUT_WRITE \
            } \
            break; \
        case 2: \
            for(; i < num_samples; i++){ \
                RINGMOD_READ \
                OSCILLATE \
                OUT_WRITE \
                SYNC_OUT_WRITE \
            } \
            break; \
        case 3: \
            for(; i < num_samples; i++){ \
                OSCILLATE \
                OUT_WRITE \
                SYNC_OUT_WRITE \
            } \
            break; \
        case 4: \
            for(; i < num_samples; i++){ \
                RINGMOD_READ \
                FM_READ \
                OSCILLATE \
                SYNC_IN_READ \
                SYNC_OUT_WRITE \
            } \
            break; \
        case 5: \
            for(; i < num_samples; i++){ \
                FM_READ \
                OSCILLATE \
                SYNC_IN_READ \
                SYNC_OUT_WRITE \
            } \
            break; \
        case 6: \
            for(; i < num_samples; i++){ \
                RINGMOD_READ \
                OSCILLATE \
                SYNC_OUT_WRITE \
            } \
            break; \
        case 7: \
            for(; i < num_samples; i++){ \
                OSCILLATE \
                SYNC_OUT_WRITE \
            } \
            break; \
        case 8: \
            for(; i < num_samples; i++){ \
                RINGMOD_READ \
                FM_READ \
                OSCILLATE \
                SYNC_IN_READ \
                OUT_WRITE \
            } \
            break; \
        case 9: \
            for(; i < num_samples; i++){ \
                FM_READ \
                OSCILLATE \
                SYNC_IN_READ \
                OUT_WRITE \
            } \
            break; \
        case 10: \
            for(; i < num_samples; i++){ \
                RINGMOD_READ \
                OSCILLATE \
                OUT_WRITE \
            } \
            break; \
        case 11: \
            for(; i < num_samples; i++){ \
                OSCILLATE \
                OUT_WRITE \
            } \
            break; \

/*
int16_t poly_blep_saw(int32_t input, int32_t incr){
    input += 32767;
    incr = incr >> 16;
    incr_rec = (1<<(16+14)) / incr;

    if (input < incr){
        input = ((input * incr_rec) >> (14-1);
        input -= ((input*input)>>18);
        return input - 65536;
    } else if(input > 65535 - incr) {
        input = input - 65535;
        input = ((input * incr_rec) >> (14-1);
        input += ((input*input)>>18);
        return input + 65536;
    }
    return 0;
}
*/

static inline void get_ringmod_coeffs(osc_data_t * data, int16_t pitch, int32_t morph, int32_t waveform){
    int32_t morph_gate = data->morph_gate_prev;
    bool morph_no_pwm = data->morph_no_pwm_prev;
    if(pitch != data->pitch_prev){
        data->pitch_coeffs[0] = radspa_sct_to_rel_freq(pitch, 0);
        morph_gate = 30700 - (data->pitch_coeffs[0]>>12); // "anti" "aliasing"
        if(morph_gate < 0) morph_gate = 0;
        data->pitch_prev = pitch;
    }
    if(waveform != data->waveform_prev){
        data->waveform_prev = waveform;
        data->waveform_coeffs[0] = waveform + 32767;
        //if(waveform_coeffs[0] == (32767*2)){ data->waveform_coeffs[1] = UINT32_MAX; return; }
        data->waveform_coeffs[1] = (((uint32_t) data->waveform_coeffs[0]) * 196616);
        morph_no_pwm = waveform != 10922;
    }
    if(morph != data->morph_prev || (morph_gate != data->morph_gate_prev)){
        if(morph > morph_gate){
            morph = morph_gate;
        } else if(morph < -morph_gate){
            morph = -morph_gate;
        }
        data->morph_gate_prev = morph_gate;
        if((morph != data->morph_prev) || (morph_no_pwm != data->morph_no_pwm_prev)){
            data->morph_prev = morph;
            data->morph_no_pwm_prev = morph_no_pwm;
            data->morph_coeffs[0] = morph + 32767;
            if(morph && morph_no_pwm){;
                data->morph_coeffs[1] = (1L<<26)/((1L<<15) + morph);
                data->morph_coeffs[2] = (1L<<26)/((1L<<15) - morph);
            } else {
                data->morph_coeffs[1] = 0; //always valid for pwm, speeds up modulation
                data->morph_coeffs[2] = 0;
            }
        }
    }
}

static inline int16_t triangle(int32_t saw){
    saw -= 16384;
    saw += 65535 * (saw < -32767);
    saw -= 2*saw * (saw > 0);
    return saw * 2 + 32767;
}

static inline int16_t fake_sine(int16_t tri){
    int16_t sign = 2 * (tri > 0) - 1;
    tri *= sign;
    tri = 32767 - tri;
    tri = (tri*tri)>>15;
    tri = 32767 - tri;
    tri *= sign;
    return tri;
}

// <bad code>
// blepping is awfully slow atm but we don't have time to fix it before the next release.
// we believe it's important enough to keep it in even though it temporarily drags down
// performance. we see some low hanging fruits but we can't justify spending any more time
// on this until flow3r 1.3 is done.

static inline int16_t saw(int32_t saw, osc_data_t * data){
    int16_t saw_sgn = saw > 0 ? 1 : -1;
    int16_t saw_abs = saw * saw_sgn;
    if(saw_abs > data->blep_coeffs[0]){
        int32_t reci = (1<<15) / (32767 - data->blep_coeffs[0]);
        int32_t blep = (saw_abs - data->blep_coeffs[0]) * reci;
        blep = (blep * blep) >> 15;
        return saw - (blep * saw_sgn);
    }
    return saw;
}

static inline int16_t square(int16_t saw, osc_data_t * data){
    int16_t saw_sgn = saw >= 0 ? 1 : -1;
    return 32767 * saw_sgn;
    int16_t saw_abs = saw * saw_sgn;
    saw_abs = (saw_abs >> 14) ? saw_abs : 32767 - saw_abs;
    if(saw_abs > data->blep_coeffs[0]){
        int32_t reci = (1<<15) / (32767 - data->blep_coeffs[0]);
        int32_t blep = (saw_abs - data->blep_coeffs[0]) * reci;
        blep = (blep * blep) >> 15;
        return (32767 - blep) * saw_sgn;
    }
    return 32767 * saw_sgn;
}

static inline void get_blep_coeffs(osc_data_t * data, int32_t incr){
    if(incr == data->incr_prev) return;
    int32_t incr_norm = ((int64_t) incr * 65535) >> 32; // max 65534
    incr_norm = incr_norm > 0 ? incr_norm : -incr_norm; 
    data->blep_coeffs[0] = 32767 - incr_norm;
    data->incr_prev = incr;
}
// </bad code>

static inline int16_t apply_morph(osc_data_t * data, uint32_t counter){
    counter = counter << 1;
    int32_t input = ((uint64_t) counter * 65535) >> 32; // max 65534
    if(data->morph_coeffs[0] == 32767) return input - 32767; // fastpass

    if(input < data->morph_coeffs[0]){
        input = ((input * data->morph_coeffs[1]) >> 11);
        input -= 32767;
    } else {
        input -= data->morph_coeffs[0];
        input = ((input * data->morph_coeffs[2]) >> 11);
    }
    return input;
}

static inline int16_t apply_waveform(osc_data_t * data, int16_t input, int32_t incr){
    int32_t a, b;
    if(data->waveform_coeffs[0] < (32767-10922)){
        b = triangle(input);
        a = fake_sine(b);
    } else if(data->waveform_coeffs[0] < (32767+10922)){
        a = triangle(input);
        if(data->waveform_coeffs[0] == 32767-10922) return a;
        get_blep_coeffs(data, incr);
        b = square(input, data);
    } else if(data->waveform_coeffs[0] < (32767+32767)){
        get_blep_coeffs(data, incr);
        a = square(input, data);
        if(data->waveform_coeffs[0] == 32767+10922) return a;
        b = saw(input, data);
    } else {
        get_blep_coeffs(data, incr);
        return saw(input, data);
    }
    int32_t ret = (((int64_t) (b-a) * data->waveform_coeffs[1]) >> 32);
    return ret + a;
}

#define RINGMOD_READ { \
    if(!pitch_const) pitch = radspa_signal_get_value(pitch_sig, i, render_pass_id); \
    if(!morph_const) morph = radspa_signal_get_value(morph_sig, i, render_pass_id); \
    if(!waveform_const) waveform = radspa_signal_get_value(waveform_sig, i, render_pass_id); \
    get_ringmod_coeffs(data, pitch, morph, waveform); \
}

#define FM_READ { \
    if(!fm_const) fm_mult = (((int32_t)radspa_signal_get_value(fm_sig, i, render_pass_id)) << 15) + (1L<<28); \
}

#define OSCILLATE \
    int32_t incr = ((int64_t) data->pitch_coeffs[0] * (fm_mult)) >> 32; \
    data->counter += incr << 3;

#define SYNC_IN_APPLY { \
    data->counter = data->counter & (1<<31); \
    data->counter += (sync_in_phase + 32767) * 32769; \
}

#define SYNC_IN_READ { \
    if(!sync_in_const){ \
        sync_in = radspa_signal_get_value(sync_in_sig, i, render_pass_id); \
        if(radspa_trigger_get(sync_in, &(data->sync_in)) > 0){ \
            if(!sync_in_phase_const){ \
                sync_in_phase = radspa_signal_get_value(sync_in_phase_sig, i, render_pass_id); \
            } \
            SYNC_IN_APPLY \
        }\
    }\
}

#define OUT_APPLY \
    int32_t out = apply_morph(data, data->counter); \
    out = apply_waveform(data, out, incr); \

#define OUT_WRITE { \
    OUT_APPLY \
    radspa_signal_set_value_noclip(out_sig, i, out); \
}

#define SYNC_OUT_WRITE { \
    radspa_signal_set_value_noclip(sync_out_sig, i, data->counter > (1UL<<31) ? 32767 : -32767); \
}

void osc_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id){
    osc_data_t * data = osc->plugin_data;
    int8_t * table = (int8_t * ) osc->plugin_table;
    radspa_signal_t * speed_sig = radspa_signal_get_by_index(osc, OSC_SPEED);
    radspa_signal_t * out_sig = radspa_signal_get_by_index(osc, OSC_OUT);
    radspa_signal_t * pitch_sig = radspa_signal_get_by_index(osc, OSC_PITCH);
    radspa_signal_t * waveform_sig = radspa_signal_get_by_index(osc, OSC_WAVEFORM);
    radspa_signal_t * morph_sig = radspa_signal_get_by_index(osc, OSC_MORPH);
    radspa_signal_t * fm_sig = radspa_signal_get_by_index(osc, OSC_FM);
    radspa_signal_t * sync_in_sig = radspa_signal_get_by_index(osc, OSC_SYNC_IN);
    radspa_signal_t * sync_in_phase_sig = radspa_signal_get_by_index(osc, OSC_SYNC_IN_PHASE);
    radspa_signal_t * sync_out_sig = radspa_signal_get_by_index(osc, OSC_SYNC_OUT);

    int16_t pitch = radspa_signal_get_const_value(pitch_sig, render_pass_id);
    int16_t sync_in = radspa_signal_get_const_value(sync_in_sig, render_pass_id);
    int32_t sync_in_phase = radspa_signal_get_const_value(sync_in_phase_sig, render_pass_id);
    int16_t morph = radspa_signal_get_const_value(morph_sig, render_pass_id);
    int16_t waveform = radspa_signal_get_const_value(waveform_sig, render_pass_id);
    int16_t fm = radspa_signal_get_const_value(fm_sig, render_pass_id);
    int32_t fm_mult = (((int32_t) fm) << 15) + (1L<<28);

    int16_t speed = radspa_signal_get_value(speed_sig, 0, render_pass_id);

    bool out_const = out_sig->buffer == NULL;
    bool sync_out_const = sync_out_sig->buffer == NULL;

    bool lfo = speed < -10922; // manual setting
    lfo = lfo || (out_const && sync_out_const); // unlikely, host should ideally prevent that case

    {
        get_ringmod_coeffs(data, pitch, morph, waveform);
        lfo = lfo || ((speed < 10922) && (data->pitch_coeffs[0] < 1789569)); // auto mode below 20Hz

        OSCILLATE

        if(lfo) data->counter += incr * (num_samples-1);

        if(radspa_trigger_get(sync_in, &(data->sync_in)) > 0){
            SYNC_IN_APPLY
        }

        OUT_APPLY

        radspa_signal_set_const_value(out_sig, out);
        // future blep concept for hard sync: encode subsample progress in the last 5 bit of the
        // trigger  signal? would result in "auto-humanize" when attached to any other consumer
        // but that's fine we think.
        radspa_signal_set_const_value(sync_out_sig, data->counter > (1UL<<31) ? 32767 : -32767);
        table[(data->counter<<1)>>(32-6)] = out >> 8;
    }

    if(!lfo){
        uint16_t i = 1; // incrementing variable for megaswitch for loop
        bool waveform_const = waveform != RADSPA_SIGNAL_NONCONST;
        bool morph_const = morph != RADSPA_SIGNAL_NONCONST;
        bool sync_in_phase_const = sync_in_phase != RADSPA_SIGNAL_NONCONST;
        bool fm_const = fm != RADSPA_SIGNAL_NONCONST;
        bool pitch_const = pitch != RADSPA_SIGNAL_NONCONST;
        bool sync_in_const = sync_in != RADSPA_SIGNAL_NONCONST;

        bool ringmod_const = pitch_const && morph_const && waveform_const;
        bool oscmod_const = fm_const && sync_in_const;

        { // generate rest of samples with megaswitch
            uint8_t fun_index = ringmod_const + (oscmod_const<<1) + (out_const<<2) + (sync_out_const<<3);
            switch(fun_index){
                OSC_MEGASWITCH
            }
        }
    }
}

#pragma GCC pop_options

radspa_t * osc_create(uint32_t init_var){
    radspa_t * osc = radspa_standard_plugin_create(&osc_desc, OSC_NUM_SIGNALS, sizeof(osc_data_t), 32);
    osc->render = osc_run;
    radspa_signal_set(osc, OSC_OUT, "output", RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(osc, OSC_PITCH, "pitch", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_SCT, RADSPA_SIGNAL_VAL_SCT_A440);
    radspa_signal_set(osc, OSC_WAVEFORM, "waveform", RADSPA_SIGNAL_HINT_INPUT, -10922);
    radspa_signal_set(osc, OSC_MORPH, "morph", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(osc, OSC_FM, "fm", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(osc, OSC_SYNC_IN, "sync_input", RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    radspa_signal_set(osc, OSC_SYNC_IN_PHASE, "sync_input_phase", RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(osc, OSC_SYNC_OUT, "sync_output", RADSPA_SIGNAL_HINT_OUTPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    radspa_signal_set(osc, OSC_SPEED, "speed", RADSPA_SIGNAL_HINT_INPUT, 0);

    radspa_signal_get_by_index(osc, OSC_WAVEFORM)->unit = "{SINE:-32767} {TRI:-10922} {SQUARE:10922} {SAW:32767}";
    radspa_signal_get_by_index(osc, OSC_SPEED)->unit = "{LFO:-32767} {AUTO:0} {AUDIO:32767}";
    osc_data_t * data = osc->plugin_data;
    data->pitch_prev = -32768;
    data->morph_prev = -32768;
    data->waveform_prev = -32768;
    data->morph_gate_prev = -32768;
    return osc;
}
