//SPDX-License-Identifier: CC0-1.0

// Please do not define a new version number. If want to distribute a modified version of
// this file, kindly append "-modified" to the version string below so it is not mistaken
// for an official release.

// Version 0.2.1

/* Realtime Audio Developer's Simple Plugin Api
 *
 * Written from scratch but largely inspired by faint memories of the excellent ladspa.h
 *
 * Plugins may only include this file and the corresponding "radspa_helpers.h" with
 * the same version string. Specifically, do not include <math.h> no matter how tempting
 * it may be - it's a notoriously slow library on most architectures and has no place
 * in realtime audio synthesis.
 *
 * For a simple plugin implementation example check ampliverter.c/.h :D
 */

#pragma once

#include <string.h>
#include <stdbool.h>
#include <stdint.h>

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

// signal hints

#define RADSPA_SIGNAL_HINT_INPUT (1<<0)
#define RADSPA_SIGNAL_HINT_OUTPUT (1<<1)
#define RADSPA_SIGNAL_HINT_TRIGGER (1<<2)
#define RADSPA_SIGNAL_HINT_GAIN (1<<3)
#define RADSPA_SIGNAL_HINT_SCT (1<<5)

#define RADSPA_SIGNAL_VAL_SCT_A440 (INT16_MAX - 6*2400)
#define RADSPA_SIGNAL_VAL_UNITY_GAIN (1<<12)

struct _radspa_descriptor_t;
struct _radspa_signal_t;
struct _radspa_t;

typedef struct _radspa_descriptor_t{
    char * name;
    uint32_t id; // unique id number
    char * description;
    struct _radspa_t * (* create_plugin_instance)(uint32_t init_var);
    void (* destroy_plugin_instance)(struct _radspa_t * plugin); // point to radspa_t
} radspa_descriptor_t;

typedef struct _radspa_signal_t{
    // this bitfield determines the type of the signal, see RADSPA_SIGNAL_HINTS_*
    uint32_t hints;
    // this is the name of the signal as shown to the user.
    // allowed characters: lowercase, numbers, underscore, may not start with number
    // if name_multplex >= 0: may not end with number
    char * name;
    // arbitrary formatted string to describe what the signal is for
    char * description;
    // unit that corresponds to value, may be empty. note: some RADSPA_SIGNAL_HINTS_*
    // imply units. field may be empty.
    char * unit;
    // -1 to disable signal multiplexing
    int8_t name_multiplex;
    // buffer full of samples, may be NULL.
    int16_t * buffer;
    // static value to be used when buffer is NULL for input signals only
    int16_t value;
    // when the signal has last requested to render its source
    uint32_t render_pass_id;
} radspa_signal_t;

typedef struct _radspa_t{
    const radspa_descriptor_t * descriptor;
    
    // renders all signal outputs for num_samples if render_pass_id has changed
    // since the last call, else does nothing.
    void (* render)(struct _radspa_t * plugin, uint16_t num_samples, uint32_t render_pass_id);

    // stores id number of render pass.
    uint32_t render_pass_id;

    // init var that was used for creating the plugin. if the plugin needs to modify the value to
    // a valid range it may do so at any point in time.
    uint32_t init_var;

    void * plugin_data; // internal data for the plugin to use. should not be accessed from outside.
    uint32_t plugin_table_len;
    int16_t * plugin_table;

    uint8_t len_signals;
    radspa_signal_t signals[]; 
} radspa_t;

/* REQUIREMENTS
 * Hosts must provide implementations for the following functions:
 */

/* The return value should equal frequency(sct) * UINT32_MAX / (sample_rate>>undersample_pow) with:
 * frequency(sct) = pow(2, (sct + 2708)/2400)
 * /!\ Performance critical, might be called on a per-sample basis, do _not_ just use pow()!
 */
extern uint32_t radspa_sct_to_rel_freq(int16_t sct, int16_t undersample_pow);

// Return 1 if the buffer wasn't rendered already, 0 otherwise.
extern bool radspa_host_request_buffer_render(int16_t * buf);


extern int16_t radspa_random();
