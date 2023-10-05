#pragma once
#include "radspa.h"
#include "radspa_helpers.h"

typedef struct _poly_squeeze_note_t {
    int16_t pitch;
    int16_t vol;
    int8_t voice;
    struct _poly_squeeze_note_t * lower;
    struct _poly_squeeze_note_t * higher;
} poly_squeeze_note_t;

typedef struct {
    int16_t trigger_out;
    int16_t pitch_out;
    int16_t _start_trigger;
} poly_squeeze_voice_t;

typedef struct {
    int16_t trigger_in_hist;
} poly_squeeze_input_t;

typedef struct {
    uint8_t num_voices;
    uint8_t num_notes;
    uint8_t num_inputs;
    poly_squeeze_note_t * active_notes_top;
    poly_squeeze_note_t * active_notes_bottom;
    poly_squeeze_note_t * free_notes;
} poly_squeeze_data_t;

extern radspa_descriptor_t poly_squeeze_desc;
radspa_t * poly_squeeze_create(uint32_t init_var);
void poly_squeeze_run(radspa_t * osc, uint16_t num_samples, uint32_t render_pass_id);
