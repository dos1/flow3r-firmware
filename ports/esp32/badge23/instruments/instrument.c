void render_audio(
    instrument_descriptor_t instrument_descriptor,
    instrument_t instance,
    unsigned int sample_count,
    float * output_vector
){
    for(unsigned int i = 0; i < sample_count; i++){
        instrument_descriptor->render_audio_sample(instance, sample_count, output_vector[2*i]);
    }
}

void render_audio_adding(
    instrument_descriptor_t instrument_descriptor,
    instrument_t instance,
    unsigned int sample_count,
    float * output_vector,
    float gain
){
    if(gain <= 0.0000001) return;
    float temp[2];
    for(unsigned int i = 0; i < sample_count; i++){
        instrument_descriptor->render_audio_sample(instance, sample_count, temp);
        output_vector[2*i] += gain * buffer[0];
        output_vector[2*i+1] += gain * buffer[1];
    }
}

void append_instrument_descriptor(
    instrument_descriptor_list_t * list,
    void * construct_instrument_descriptor
){
    lle_t * element = list;
    while(element->next != NULL){
        element = element->next;
    }
    element->next = malloc(sizeof(lle_t);
    if(element->next == NULL) return;
    element = element->next;
    element->next = NULL;

    element->content = construct_instrument_descriptor();
}

instrument_descriptor_list_t * list_builtin_instrument_descriptors(){
    //really hope we can make this more elegant some day
    instrument_descriptor_list_t * list = NULL;

    //add your instrument here!
    append_instrument_descriptor(list, &minimal_example_descriptor);

    return list;
}

void instantiate_instrument(active_instrument_list_t instruments,
                            instrument_descriptor_t descriptor
){
    descriptor->new_instrument
}

void mix_instruments_audio(active_instrument_list_t instruments,
                           unsigned int sample_count,
                           float * output_vector
){
    active_instrument_t instrument = instruments;
    while(instrument->next != NULL){
        render_audio_adding(instrument->descriptor,
                            instrument->instrument,
                            sample_count,
                            output_vector,
                            instrument->gain)
        instrument = instrument->next;
    }
}
