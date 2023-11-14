#include "poly_squeeze.h"

radspa_descriptor_t poly_squeeze_desc = {
    .name = "poly_squeeze",
    .id = 172,
    .description = "Multiplexes a number of trigger and pitch inputs into a lesser number of trigger pitch output pairs. "
                   "The latest triggered inputs are forwarded to the output. If such an input receives a stop trigger it is disconnected "
                   "from its output. If another inputs is in triggered state but not forwarded at the same time it will be connected to that "
                   "output and the output is triggered. Pitch is constantly streamed to the outputs if they are connected, else the last "
                   "connected value is being held."
                   "\ninit_var: lsb: number of outputs, 1..16, default 3; lsb+1: number of inputs, <lsb>..32, default 10; ",
    .create_plugin_instance = poly_squeeze_create,
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define NUM_MPX 2
// mpx block 1
#define TRIGGER_INPUT 0
#define PITCH_INPUT 1
// mpx block 2
#define TRIGGER_OUTPUT 0
#define PITCH_OUTPUT 1

static void assign_note_voices(poly_squeeze_data_t * data){
    poly_squeeze_note_t * note = data->active_notes_top;
    if(note == NULL) return;
    uint32_t active_voices = 0xFFFFFFFFUL << data->num_voices;

    for(uint8_t i = 0; i < data->num_voices; i++){
        if(note == NULL) break;
        if(note->voice >= 0) active_voices = active_voices | (1UL<<note->voice);
        note = note->lower;
    }
    while(note != NULL){
        note->voice = -1;
        note = note->lower;
    }
    
    if(active_voices == UINT32_MAX) return;

    note = data->active_notes_top;
    for(uint8_t i = 0; i < data->num_voices; i++){
        if(note == NULL) break;
        if(note->voice < 0){
            int8_t voice = -1;
            for(int8_t v = 0; v < data->num_voices; v++){
                if((~active_voices) & (1<<v)){
                    active_voices = active_voices | (1<<v);
                    voice = v;
                    break;
                }
            }
            note->voice = voice;
        }
        note = note->lower;
    }
}

static poly_squeeze_note_t * get_note_with_voice(poly_squeeze_data_t * data, int8_t voice){
    poly_squeeze_note_t * note = data->active_notes_top;
    for(uint8_t i = 0; i < data->num_voices; i++){
        if(note == NULL) break;
        if(note->voice == voice) return note;
        note = note->lower;
    }
    return NULL;
}

void take_note(poly_squeeze_data_t * data, poly_squeeze_note_t * note){
    if(note == NULL) return;
    if(data->free_notes == note) data->free_notes = note->lower;
    if(data->active_notes_bottom == note) data->active_notes_bottom = note->higher;
    if(data->active_notes_top == note) data->active_notes_top = note->lower;
    if(note->higher != NULL) note->higher->lower = note->lower;
    if(note->lower != NULL) note->lower->higher = note->higher;
    note->higher = NULL;
    note->lower = NULL;
}

static int8_t put_note_on_top(poly_squeeze_data_t * data, poly_squeeze_note_t * note){
    if(note == NULL) return -1;
    if(note == data->active_notes_top) return note->voice;
    take_note(data, note);
    note->lower = data->active_notes_top;
    if(note->lower != NULL) note->lower->higher = note;
    data->active_notes_top = note;

    if(note->lower == NULL) data->active_notes_bottom = note;
    assign_note_voices(data);
    return note->voice;
}

static int8_t free_note(poly_squeeze_data_t * data, poly_squeeze_note_t * note){
    if(note == NULL) return -1;
    take_note(data, note);
    int8_t ret = note->voice;
    note->voice = -1;
    note->lower = data->free_notes;
    if(note->lower != NULL) note->lower->higher = note;
    data->free_notes = note;
    assign_note_voices(data);
    return ret;
}

static void voice_start(poly_squeeze_voice_t * voice, int16_t pitch, int16_t vol){
    voice->pitch_out = pitch;
    int16_t tmp = voice->_start_trigger;
    radspa_trigger_start(vol, &tmp);
    voice->trigger_out = tmp;
} 

static void voice_stop(poly_squeeze_voice_t * voice){
    int16_t tmp = voice->_start_trigger;
    radspa_trigger_stop(&tmp);
    voice->trigger_out = tmp;
} 

void poly_squeeze_run(radspa_t * poly_squeeze, uint16_t num_samples, uint32_t render_pass_id){
    poly_squeeze_data_t * data = poly_squeeze->plugin_data;
    poly_squeeze_note_t * notes = (void *) (&(data[1]));
    poly_squeeze_input_t * inputs = (void *) (&(notes[data->num_notes]));
    poly_squeeze_voice_t * voices = (void *) (&(inputs[data->num_inputs]));

    for(uint8_t j = 0; j < data->num_inputs; j++){
        uint16_t pitch_index;
        int16_t trigger_in = radspa_trigger_get_const(&poly_squeeze->signals[TRIGGER_INPUT * NUM_MPX*j],
                                &inputs[j].trigger_in_hist, &pitch_index, num_samples, render_pass_id);
        notes[j].pitch = radspa_signal_get_value(&poly_squeeze->signals[PITCH_INPUT + NUM_MPX*j], pitch_index, render_pass_id);
        // should order events by pitch index some day maybe
        if(trigger_in > 0){
            notes[j].vol = trigger_in;
            int8_t voice = put_note_on_top(data, &(notes[j]));
            if(voice >= 0) voice_start(&(voices[voice]), notes[j].pitch, notes[j].vol);
        } else if(trigger_in < 0){
            int8_t voice = free_note(data, &(notes[j]));
            if(voice >= 0){
                poly_squeeze_note_t * note = get_note_with_voice(data, voice);
                if(note == NULL){
                    voice_stop(&(voices[voice]));
                } else {
                    voice_start(&(voices[voice]), note->pitch, note->vol);
                }
            }
        }
    }
    for(uint8_t j = 0; j < data->num_inputs; j++){
        if((notes[j].voice != -1) && (notes[j].voice < data->num_voices)){
            voices[notes[j].voice].pitch_out = notes[j].pitch;
        }
    }
    for(uint8_t j = 0; j < data->num_voices; j++){
        uint8_t k = data->num_inputs + j;
        radspa_signal_set_const_value(&poly_squeeze->signals[TRIGGER_OUTPUT + NUM_MPX * k], voices[j].trigger_out);
        radspa_signal_set_const_value(&poly_squeeze->signals[PITCH_OUTPUT + NUM_MPX * k], voices[j].pitch_out);
        voices[j]._start_trigger = voices[j].trigger_out;
    }
}

radspa_t * poly_squeeze_create(uint32_t init_var){
    if(!init_var) init_var = 3 + (1UL<<8) + 9 * (1UL<<16);
    uint8_t num_voices = init_var & 0xFF;
    if(num_voices > 32) num_voices = 32;
    if(num_voices < 1) num_voices = 1;

    init_var = init_var >> 8;
    uint8_t num_inputs = init_var & 0xFF;
    if(num_inputs > 32) num_inputs = 32;
    if(num_inputs < num_voices) num_inputs = num_voices;

    uint8_t num_notes = num_inputs;

    uint32_t num_signals = num_voices * NUM_MPX + num_inputs * NUM_MPX;
    size_t data_size = sizeof(poly_squeeze_data_t);
    data_size += sizeof(poly_squeeze_voice_t) * num_voices;
    data_size += sizeof(poly_squeeze_note_t) * num_notes;
    data_size += sizeof(poly_squeeze_input_t) * num_inputs;
    radspa_t * poly_squeeze = radspa_standard_plugin_create(&poly_squeeze_desc, num_signals, data_size, 0);
    if(poly_squeeze == NULL) return NULL;

    poly_squeeze->render = poly_squeeze_run;

    radspa_signal_set_group(poly_squeeze, num_inputs, NUM_MPX, TRIGGER_INPUT, "trigger_in",
            RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    radspa_signal_set_group(poly_squeeze, num_inputs, NUM_MPX, PITCH_INPUT, "pitch_in",
            RADSPA_SIGNAL_HINT_INPUT | RADSPA_SIGNAL_HINT_SCT, RADSPA_SIGNAL_VAL_SCT_A440);
    radspa_signal_set_group(poly_squeeze, num_voices, NUM_MPX, TRIGGER_OUTPUT + NUM_MPX*num_inputs, "trigger_out",
            RADSPA_SIGNAL_HINT_OUTPUT | RADSPA_SIGNAL_HINT_TRIGGER, 0);
    radspa_signal_set_group(poly_squeeze, num_voices, NUM_MPX, PITCH_OUTPUT + NUM_MPX*num_inputs, "pitch_out",
            RADSPA_SIGNAL_HINT_OUTPUT | RADSPA_SIGNAL_HINT_SCT, RADSPA_SIGNAL_VAL_SCT_A440);

    poly_squeeze_data_t * data = poly_squeeze->plugin_data;
    data->num_voices = num_voices;
    data->num_notes = num_notes;
    data->num_inputs = num_inputs;

    poly_squeeze_note_t * notes = (void *) (&(data[1]));
    poly_squeeze_input_t * inputs = (void *) (&(notes[data->num_notes]));
    poly_squeeze_voice_t * voices = (void *) (&(inputs[data->num_inputs]));

    data->active_notes_top = NULL;
    data->active_notes_bottom = NULL;
    data->free_notes = &(notes[0]);

    for(uint8_t i = 0; i < num_voices; i++){
        voices[i].trigger_out = 0;
        voices[i].pitch_out = RADSPA_SIGNAL_VAL_SCT_A440;
        voices[i]._start_trigger = voices[i].trigger_out;
    }

    for(uint8_t i = 0; i < num_notes; i++){
        notes[i].pitch = -32768;
        notes[i].voice = -1;
        if(i){
            notes[i].higher = &(notes[i-1]);
        } else{
            notes[i].higher = NULL;
        }
        if(i != (num_notes - 1)){
            notes[i].lower = &(notes[i+1]);
        } else {
            notes[i].lower = NULL;
        }
    }

    for(uint8_t i = 0; i < num_inputs; i++){
        inputs[i].trigger_in_hist = 0;
    }

    return poly_squeeze;
}
