#include "ampliverter.h"

// we're defining a prototype for the create function because it has a circular
// dependency with the descriptor
radspa_t* ampliverter_create(uint32_t init_var);
radspa_descriptor_t ampliverter_desc = {
    .name = "ampliverter",
    .id = 69,
    .description = "saturating multiplication and addition",
    .create_plugin_instance = ampliverter_create,
    // with this we can only use radspa_standard_plugin_create to allocate
    // memory. this restricts data layout flexibility for large buffers, but in
    // return it offers offers a bit of protection from double free/memory leak
    // issues and makes plugins easier to write.
    .destroy_plugin_instance = radspa_standard_plugin_destroy
};

#define AMPLIVERTER_NUM_SIGNALS 4
#define AMPLIVERTER_OUTPUT 0
#define AMPLIVERTER_INPUT 1
#define AMPLIVERTER_GAIN 2
#define AMPLIVERTER_BIAS 3

void ampliverter_run(radspa_t* ampliverter, uint16_t num_samples,
                     uint32_t render_pass_id) {
    // step 1: get signal pointers. since these are stored in a linked list this
    // is a rather slow operation and should ideally be only be done once per
    // call. if no signals with output hint are being read the run function may
    // exit early.
    radspa_signal_t* output_sig =
        radspa_signal_get_by_index(ampliverter, AMPLIVERTER_OUTPUT);
    if (output_sig->buffer == NULL) return;
    radspa_signal_t* input_sig =
        radspa_signal_get_by_index(ampliverter, AMPLIVERTER_INPUT);
    radspa_signal_t* gain_sig =
        radspa_signal_get_by_index(ampliverter, AMPLIVERTER_GAIN);
    radspa_signal_t* bias_sig =
        radspa_signal_get_by_index(ampliverter, AMPLIVERTER_BIAS);

    static int16_t ret = 0;
    for (uint16_t i = 0; i < num_samples; i++) {
        // step 2: render the outputs. most of the time a simple for loop will
        // be fine. using {*radspa_signal_t}->get_value is required to
        // automatically switch between static values and streamed data at
        // various sample rates, don't access the data directly
        int16_t bias =
            bias_sig->get_value(bias_sig, i, num_samples, render_pass_id);
        int16_t gain =
            gain_sig->get_value(gain_sig, i, num_samples, render_pass_id);
        if (gain == 0) {
            // make sure that the output buffer exists by comparing to NULL!
            // (here done earlier outside of the loop)
            ret = bias;
        } else {
            ret =
                input_sig->get_value(input_sig, i, num_samples, render_pass_id);
            // the helper functions make sure that potential future float
            // versions behave as intended
            ret = radspa_mult_shift(ret, gain);
            ret = radspa_add_sat(ret, bias);
        }
        (output_sig->buffer)[i] = ret;
    }
    output_sig->value = ret;
}

radspa_t* ampliverter_create(uint32_t init_var) {
    // init_var is not used in this example

    // step 1: try to allocate enough memory for a standard plugin with the
    // given amount of signals and plugin data. there is no plugin data in this
    // case, but sizeof(void) is invalid sooo we're taking the next smallest
    // thing (char) ig? we're not good at C. providing the descriptor address is
    // required to make sure it is not forgotten.
    radspa_t* ampliverter = radspa_standard_plugin_create(
        &ampliverter_desc, AMPLIVERTER_NUM_SIGNALS, sizeof(char), 0);
    if (ampliverter == NULL) return NULL;

    // step 2: define run function
    ampliverter->render = ampliverter_run;

    // step 3: standard_plugin_create has already created dummy signals for us,
    // we just need to fill them
    radspa_signal_set(ampliverter, AMPLIVERTER_OUTPUT, "output",
                      RADSPA_SIGNAL_HINT_OUTPUT, 0);
    radspa_signal_set(ampliverter, AMPLIVERTER_INPUT, "input",
                      RADSPA_SIGNAL_HINT_INPUT, 0);
    radspa_signal_set(ampliverter, AMPLIVERTER_GAIN, "gain",
                      RADSPA_SIGNAL_HINT_INPUT, 32767);
    radspa_signal_set(ampliverter, AMPLIVERTER_BIAS, "bias",
                      RADSPA_SIGNAL_HINT_INPUT, 0);
    return ampliverter;
}
