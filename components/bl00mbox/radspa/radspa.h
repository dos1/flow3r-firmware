#pragma once

#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// realtime audio developer's simple plugin api
// written from scratch but largely inspired by
// faint memories of the excellent ladspa.h

#define RADSPA_SIGNAL_HINT_INPUT 1
#define RADSPA_SIGNAL_HINT_OUTPUT 2
#define RADSPA_SIGNAL_HINT_TRIGGER 4

#define RADSPA_SIGNAL_HINT_LIN 8
#define RADSPA_SIGNAL_HINT_LOG 16
#define RADSPA_SIGNAL_HINT_SCT 32

/* CONVENTIONS
 *
 * All plugins communicate all signals in int16_t data.
 *
 * SCT (semi-cent, 2/ct) is a mapping between int16_t pitch values and frequency values comparable
 * to 1V/oct. in analog music electronics. A cent is a common unit in music theory and describes
 * 1/100 of a semitone or 1/1200 of an octave. Typically 1 cent is precise enough for synth
 * applications, however 1/2 cent is better and allows for a 27 octave range. INT16_MAX
 * represents a frequency of 28160Hz, 6 octaves above middle A (440Hz) which is represented by
 * INT16_MAX - 6 * 2400.
 */

/* This funciton must be provided by the host and allow for fast conversion between sct values
 * and a frequency relative to the current sample rate.
 * The return value should equal frequency(scl) * UINT32_MAX / sample_rate with:
 * frequency(scl) = pow(2, (sct + 2708)/2400)
 */
extern uint32_t radspa_sct_to_rel_freq(int16_t sct, int16_t undersample_pow);

/* This function must be prodived by the host. Return 1 if the buffer wasn't rendered already,
 * 0 otherwise.
 */
extern bool radspa_host_request_buffer_render(int16_t * buf, uint16_t num_samples);

struct _radspa_descriptor_t;
struct _radspa_signal_t;
struct _radspa_t;

typedef struct _radspa_descriptor_t{
    char name[32];
    uint32_t id; // unique id number
    char description[255];
    struct _radspa_t * (* create_plugin_instance)(uint32_t init_var);
    void (* destroy_plugin_instance)(struct _radspa_t * plugin); // point to radspa_t
} radspa_descriptor_t;

typedef struct _radspa_signal_t{
    uint32_t hints;
    char name[32]; //name

    int16_t * buffer; // full buffer of num_samples. may be NULL.

    // used for input channels only
    int16_t value;  //static value, used buffer is NULL.
    uint32_t render_pass_id;

    struct _radspa_signal_t * next; //linked list
} radspa_signal_t;

typedef struct _radspa_t{
    const radspa_descriptor_t * descriptor;

    // linked list of all i/o signals of the module and length of list
    radspa_signal_t * signals; 
    uint8_t len_signals;
    
    // renders all signal outputs for num_samples if render_pass_id has changed
    // since the last call, else does nothing.
    void (* render)(struct _radspa_t * plugin, uint16_t num_samples, uint32_t render_pass_id);

    // stores id number of render pass.
    uint32_t render_pass_id;


    void * plugin_data; // internal data for the plugin to use. should not be accessed from outside.
} radspa_t;

// HELPER FUNCTIONS
// defined in radspa.c

// renders num_samples
bool radspa_render(radspa_t * plugin, uint16_t num_samples, uint32_t render_pass_id);

// adds signal to plugin instance struct. typically used to initiate a plugin instance.
int16_t radspa_signal_add(radspa_t * plugin, char * name, uint32_t hints, int16_t value);

radspa_t * radspa_standard_plugin_create(radspa_descriptor_t * desc, uint8_t num_signals, size_t plugin_data_size);
void radspa_standard_plugin_destroy(radspa_t * plugin);
void radspa_signal_set(radspa_t * plugin, uint8_t signal_index, char * name, uint32_t hints, int16_t value);


// get signal struct from a signal index
radspa_signal_t * radspa_signal_get_by_index(radspa_t * plugin, uint16_t signal_index);

// frees all signal structs. typically used to destroy a plugin instance.
void radspa_signals_free(radspa_t * plugin);

/* returns the value that a signal has at a given moment in time. time is
 * represented as the buffer index. requests rendering from host and requires implementation
 * of radspa_host_request_buffer_render.
 */
int16_t radspa_signal_get_value(radspa_signal_t * sig, int16_t index, uint16_t num_samples, uint32_t render_pass_id);

// general math
int16_t i16_clip(int32_t a);
// a, b must be restricted to int16 range
int16_t i16_add_sat(int32_t a, int32_t b);
int16_t i16_mult_shift(int32_t a, int32_t b);
