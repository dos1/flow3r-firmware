//simple instrument example implementation

//trad_osc is made up rn, didn't check how the existing implementation works

typedef struct{
    unsigned int * sample_rate;
    hardware_inputs_t * hw;
    trad_osc_t[30] osc;
    unsigned int sample_rate;
    hardware_inputs_t * hw;
    float last_freq; //frequency of last note played to write to display
} minimal_example_t;

void minimal_example_render_sample(instrument_handle inst, float * stereo_output){
    float acc = 0;
    for(int i = 0; i < 30; i++){
        acc += trad_osc_run(&inst.osc[i], inst.sample_rate);
    }
    stereo_output[0] = acc; // both channels the same -> mono
    stereo_output[1] = acc;
}

void minimal_example_process_user_input(instrument_handle inst){
    static int petal_prev[10];
    for(int i = 0; i < 10; i++){
        if(inst->hw.petals[i].intensity > 0){    //if the pad is touched...
            if(!petal_prev[i]){                 //and it's a new touch...
                if(button != 2){ //if button isn't pressed: play single note in different octaves
                    int j = i + (inst->hw.button + 1) * 10; //choose osc
                    trad_osc_start(&inst.osc[j]);   //play a tone
                    inst->last_freq = inst.osc[j].freq;
                } else { //for button center: all the octaves at once
                    trad_osc_start(&inst.osc[i]);   //play a tone
                    trad_osc_start(&inst.osc[i+10]);   //play a tone
                    trad_osc_start(&inst.osc[i+20]);   //play a tone
                    inst->last_freq = inst.osc[i+10].freq;
                }
            }
            petal_prev[i] = 1;
        } else {
            petal_prev[i] = 0;
        }
    }
}

void minimal_example_update_display(instrument_handle inst){
    display_print("%f", inst->last_freq);
}

static float pad_to_freq(int pad){
    int j = pad % 10;
    if(j) j++;      //skip min 2nd
    if(j>8) j++;    //skip maj 6th
    j += (pad/10) * 12; //add octaves
    float freq = 440 * pow(2., j/12.);
}

instrument_t new_minimal_example(
    const struct _instrument_descriptor * descriptor, 
    unsigned int sample_rate,
    hardware_inputs_t * hw)
{
    instrument_t inst = malloc(sizeof(minimal_example_t));
    if(inst == NULL) return NULL;
    inst->sample_rate = sample_rate;
    inst->hw = hw;
    for(int i = 0; i < 30; i++){
        inst->osc[i] = trad_osc_new(pad_to_freq(i), sample_rate);
        //other osc parameters (wave, envelope, etc.) omitted for clarity
    }
    return inst;
}

void delete_minimal_example(instrument_t inst){
    free(inst);
}

const instrument_descriptor_t * minimal_example_descriptor(){
    instrument_descriptor_t * inst = malloc(sizeof(instrument_descriptor_t));
    if(inst == NULL) return NULL;
    inst->unique_id = 0;
    inst->name = "simple instrument";
    inst->render_audio_sample = &minimal_example_render_sample;
    inst->new_instrument = &new_minimal_example;
    inst->delete_instrument = &delete_minimal_example;
    inst->update_display = NULL;
    inst->process_user_input = minimal_example_user_input;
    inst->update_leds = NULL;
}
