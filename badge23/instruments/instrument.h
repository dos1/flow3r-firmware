/* wip instrument api for badge23 (in large parts inspired by ladspa)

current status: none of this is hooked up or functional or compiles, just drafting things rn

some core concepts:

- several instruments can run at the same time (e.g., drum computer running in background),
with only one of them being in foreground and using buttons and display

- instruments are as "self contained" as possible, i.e. they access buttons and display with
minimal host involvement and pretty much just produce audio "on their own". aside from
scheduling the host mostly does some cap touch preprocessing and the audio output mixdown

- different timing requirements -> different "threads" for audio, buttons, display each
(leds: special case, see below)

open questions:

- led animations: instruments should be able to output led patterns. maybe keeping a dummy
led array in ram for each running instrument and allowing users to create and run "shaders"
would be a desirable functional mode; do we want this/does this need any extra api?

- for performance reasons: instruments are expected to behave themselves, i.e. not access hw
without permission or write to read-only locations, can we do better than that without
excessive overhead? and if we can, do we _want_ to? (devices that make electronic musical
instruments malfunction on purpose are not uncommon in general)

- badge link/cv data input/output: can probably be added to the descriptor easily? shouldn't
freeze api before tho
*/

//===========================================================================================
//some hardware dummy definitions, move somewhere else someday maybe
typedef struct {
    int intensity;  //touch strength, considered as no touch if below 0
    int rad;        //radial position
    int az;         //cw azimuthal position (only meaningful for top petals);
} petal_t;

typedef int button_t; //0: no press, -1: left, 1: right, 2: down

typedef struct {    //read-only (shared between all instruments, unprotected)
    petal_t petals[10];  //even: top, odd: bottom, 0 at usb-c jack, count ccw
    button_t button;    //can be either left or right, depending on host
                        //handedness settings. the other button is reserved for
                        //host use and is not exposed here.
} hardware_inputs_t;
//===========================================================================================

typedef void * instrument_t; //contains instrument instance data, not to be interpreted by host

typedef struct _instrument_descriptor_t {
    unsigned int unique_id;
    //null terminated instrument name
    const char * name;

    //allocates memory for new instance data and returns pointer to it (mandatory)
    instrument_t (* new_instrument)(const struct _instrument_descriptor * descriptor,
                                    unsigned int sample_rate,
                                    hardware_inputs_t * hw);

    //frees instance data memory (mandatory)
    void (* delete_instrument) (instrument_t instance);

    //renders a single stereo audio sample (optional, NULL if not supported)
    void (* render_audio_sample) (instrument_t instance);

    //handles petal/button/sensor input, ideally runs every 3-8ms (optional, NULL if not supported)
    void (* process_user_input) (instrument_t instance);

    //only runs when instrument is in foreground (optional, NULL if not supported)
    void (* update_display) (instrument_t instance);

    //(optional, NULL if not supported)
    void (* update_leds) (instrument_t instance);
} instrument_descriptor_t;

// this function is called once per instrument type before use (i.e. around boot) and returns a
// filled out instrument descriptor struct to be used by the host for creating instrument instances
// returns null if allocation fails
const instrument_descriptor_t * contruct_instrument_descriptor();

//===========================================================================================
//host-side helper functions

void render_audio(
    instrument_descriptor_t instrument_descriptor,
    instrument_t instance,
    unsigned int sample_count,
    float * output_vector
);

void render_audio_adding(
    instrument_descriptor_t instrument_descriptor,
    instrument_t instance,
    unsigned int sample_count,
    float * output_vector,
    float gain
);

typedef struct {
    void * content;
    lle_t * next;
} lle_t; //linked list element

typedef lle_t instrument_descriptor_list_t;

typedef struct {
    instrument_t * instrument;
    instrument_descriptor_t descriptor;
    char is_foreground;
    char is_rendering_leds;
    float gain;
} active_instrument_t;

void append_instrument_descriptor(
    instrument_descriptor_list_t * list,
    void * construct_instrument_descriptor
);

instrument_descriptor_list_t * list_builtin_instrument_descriptors();

typedef lle_t active_instrument_list_t;
void mix_instruments_audio(active_instrument_list_t instruments,
                           unsigned int sample_count,
                           float * output_vector
);


//===========================================================================================
