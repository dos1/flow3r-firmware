#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    st3m_audio_input_source_none = 0,
    // Line in on riht jack.
    st3m_audio_input_source_line_in = 1,
    // Headset microphone on left jack.
    st3m_audio_input_source_headset_mic = 2,
    // Onboard microphone (enabled red LED).
    st3m_audio_input_source_onboard_mic = 3,
    // auto switching depending on availability
    // line in preferred to headset mic preferred to onboard mic.
    st3m_audio_input_source_auto = 4
} st3m_audio_input_source_t;

/* Initializes the audio engine and passes sample rate as well as max buffer
 * length. At this point those values are always 48000/128, but this might
 * become variable in the future. However, we see flow3r primarily as a real
 * time instrument, and longer buffers introduce latency; the current buffer
 * length corresponds to 1.3ms latency which isn't much, but given the up to
 * 10ms captouch latency on top we shouldn't be super careless here.
 */
typedef void (*st3m_audio_engine_init_function_t)(uint32_t sample_rate,
                                                  uint16_t max_len);

/* Renders the output of the audio engine and returns whether or not it has
 * overwritten tx. Always called for each buffer, no exceptions. This means you
 * can keep track of time within the engine easily and use the audio player task
 * to handle musical events (the current 1.3ms buffer rate is well paced for
 * this), but it also puts the burden on you of exiting early if there's nothing
 * to do.
 *
 * rx (input) and tx (output) are both stereo interlaced, i.e. the even indices
 * represent the left channel, the odd indices represent the right channel. The
 * length is the total length of the buffer so that each channel has len/2 data
 * points. len is always even.
 *
 * The function must never modify rx. This is so that we can pass the same
 * buffer to all the engines without having to memcpy by default, so if you need
 * to modify rx please do your own memcpy of it.
 *
 * In a similar manner, tx is not cleaned up when calling the function, it
 * carries random junk data that is not supposed to be read by the user. The
 * return value indicates whether tx should be used or if tx should be ignored
 * andit should be treated as if you had written all zeroes into it (without you
 * actually doing so). If you choose to return true please make sure you have
 * overwritten the entirety of tx with valid data.
 */
typedef bool (*st3m_audio_engine_render_function_t)(int16_t* rx, int16_t* tx,
                                                    uint16_t len);

typedef struct {
    char* name;  // used for UI, no longer than 14 characters
    st3m_audio_engine_render_function_t render_fun;
    st3m_audio_engine_init_function_t init_fun;  // optional, else NULL
} st3m_audio_engine_t;

/* Initializes I2S bus, the audio task and required data structures.
 * Expects an initialized I2C bus, will fail ungracefully otherwise (TODO).
 */
void st3m_audio_init(void);

/* Returns true if headphones with or without microphone were connected to the
 * headphone jack at the last call of st3m_audio_update_jacksense.
 */
bool st3m_audio_headphones_are_connected(void);

/* Returns true if headphones with microphone were connected to the headphone
 * jack at the last call of audio_update_jacksense.
 */
bool st3m_audio_headset_is_connected(void);

/* Returns true if the line-in jack is connected to a cable. */
bool st3m_audio_line_in_is_connected(void);

/* If a sleeve contact mic doesn't pull the detection pin low enough the
 * codec's built in headphone detection might fail. Calling this function
 * with 'enable = 1' overrides the detection and assumes there's headphones
 * plugged in. Call with 'enable = 0' to revert to automatic detection.
 */
void st3m_audio_headphones_detection_override(bool enable);

/* Attempts to set target volume for the headphone output/onboard speakers
 * respectively, clamps/rounds if necessary and returns the actual volume.
 * Absolute reference arbitrary.
 * Does not unmute, use st3m_audio_{headphones_/speaker_/}set_mute as needed.
 * Enters fake mute if requested volume is below the value set by
 * st3m_audio_{headphones/speaker}_set_minimum_volume_user.
 *
 * Note: This function uses a hardware PGA for the coarse value and software
 * for the fine value. These two methods are as of yet not synced so that there
 * may be a transient volume "hiccup". "p1" badges only use software volume.
 * The unspecified variant automatically chooses the adequate channel (**).
 */
float st3m_audio_headphones_set_volume_dB(float vol_dB);
float st3m_audio_speaker_set_volume_dB(float vol_dB);
float st3m_audio_set_volume_dB(float vol_dB);

/* Like the st3m_audio_{headphones_/speaker_/}set_volume family but changes
 * relative to last volume value.
 */
float st3m_audio_headphones_adjust_volume_dB(float vol_dB);
float st3m_audio_speaker_adjust_volume_dB(float vol_dB);
float st3m_audio_adjust_volume_dB(float vol_dB);

/* Returns volume as set with st3m_audio_{headphones/speaker}_set_volume_dB. The
 * unspecified variant automatically chooses the adequate channel (**).
 */
float st3m_audio_headphones_get_volume_dB(void);
float st3m_audio_speaker_get_volume_dB(void);
float st3m_audio_get_volume_dB(void);

/* Mutes (mute = 1) or unmutes (mute = 0) the specified channel.
 * The unspecified variant automatically chooses the adequate channel (**).
 *
 * Note: Even if a channel is unmuted it might not play sound depending on
 * the return value of st3m_audio_headphone_are_connected. There is no override
 * for this (see HEADPHONE PORT POLICY below).
 */
void st3m_audio_headphones_set_mute(bool mute);
void st3m_audio_speaker_set_mute(bool mute);
void st3m_audio_set_mute(bool mute);

/* Returns true if channel is muted, false otherwise.
 * The unspecified variant automatically chooses the adequate channel (**).
 */
bool st3m_audio_headphones_get_mute(void);
bool st3m_audio_speaker_get_mute(void);
bool st3m_audio_get_mute(void);

/* Set the minimum and maximum allowed volume levels for speakers and headphones
 * respectively. Clamps with hardware limitations. Maximum clamps below the
 * minimum value, minimum clamps above the maximum. Returns clamped value.
 */
float st3m_audio_headphones_set_minimum_volume_dB(float vol_dB);
float st3m_audio_headphones_set_maximum_volume_dB(float vol_dB);
float st3m_audio_speaker_set_minimum_volume_dB(float vol_dB);
float st3m_audio_speaker_set_maximum_volume_dB(float vol_dB);

/* Returns the minimum and maximum allowed volume levels for speakers and
 * headphones respectively. Change with
 * st3m_audio_{headphones/speaker}_set_{minimum/maximum}_volume_dB.
 */
float st3m_audio_headphones_get_minimum_volume_dB(void);
float st3m_audio_headphones_get_maximum_volume_dB(void);
float st3m_audio_speaker_get_minimum_volume_dB(void);
float st3m_audio_speaker_get_maximum_volume_dB(void);

/* Syntactic sugar for drawing UI: Returns channel volume in a 0..1 range,
 * scaled into a 0.01..1 range according to the values set with
 * st3m_audio_{headphones_/speaker_/}set_{maximum/minimum}_volume_ and 0 if in a
 * fake mute condition.
 *
 * The unspecified variant automatically chooses the adequate channel (**).
 */
float st3m_audio_headphones_get_volume_relative(void);
float st3m_audio_speaker_get_volume_relative(void);
float st3m_audio_get_volume_relative(void);

/* (**) if st3m_audio_headphones_are_connected returns 1 the "headphone" variant
 *      is chosen, else the "speaker" variant is chosen.
 */

/* These route whatever is on the line in port directly to the headphones or
 * speaker respectively (enable = 1), or don't (enable = 0). Is affected by mute
 * and coarse hardware volume settings, however software fine volume is not
 * applied.
 *
 * Good for testing, might deprecate later, idk~
 */
void st3m_audio_headphones_line_in_set_hardware_thru(bool enable);
void st3m_audio_speaker_line_in_set_hardware_thru(bool enable);
void st3m_audio_line_in_set_hardware_thru(bool enable);

/* The codec can transmit audio data from different sources. This function
 * enables one or no source as provided by the st3m_audio_input_source_t enum.
 *
 * Note: The onboard digital mic turns on an LED on the top board if it receives
 * a clock signal which is considered a good proxy for its capability of reading
 * data.
 */
void st3m_audio_input_engine_set_source(st3m_audio_input_source_t source);
st3m_audio_input_source_t st3m_audio_input_engine_get_source(void);
st3m_audio_input_source_t st3m_audio_input_engine_get_target_source(void);

void st3m_audio_input_thru_set_source(st3m_audio_input_source_t source);
st3m_audio_input_source_t st3m_audio_input_thru_get_source(void);
st3m_audio_input_source_t st3m_audio_input_thru_get_target_source(void);

st3m_audio_input_source_t st3m_audio_input_get_source(void);

/* Gain of headset mic source
 */
float st3m_audio_headset_mic_set_gain_dB(float gain_dB);
float st3m_audio_headset_mic_get_gain_dB(void);

/* Gain of onboard mic source
 */
float st3m_audio_onboard_mic_set_gain_dB(float gain_dB);
float st3m_audio_onboard_mic_get_gain_dB(void);

/* Gain of line in source
 */
float st3m_audio_line_in_set_gain_dB(float gain_dB);
float st3m_audio_line_in_get_gain_dB(void);

/* You can route whatever source is selected with st3m_audio_input_set_source to
 * the audio output. Use these to control volume and mute.
 */
float st3m_audio_input_thru_set_volume_dB(float vol_dB);
float st3m_audio_input_thru_get_volume_dB(void);
void st3m_audio_input_thru_set_mute(bool mute);
bool st3m_audio_input_thru_get_mute(void);

/* Activate a EQ preset when speakers are enabled, or don't :D
 */

void st3m_audio_speaker_set_eq_on(bool enable);
bool st3m_audio_speaker_get_eq_on(void);

void st3m_audio_headset_mic_set_allowed(bool allowed);
bool st3m_audio_headset_mic_get_allowed(void);

void st3m_audio_onboard_mic_set_allowed(bool allowed);
bool st3m_audio_onboard_mic_get_allowed(void);

void st3m_audio_line_in_set_allowed(bool allowed);
bool st3m_audio_line_in_get_allowed(void);

void st3m_audio_onboard_mic_to_speaker_set_allowed(bool allowed);
bool st3m_audio_onboard_mic_to_speaker_get_allowed(void);

bool st3m_audio_input_engine_get_source_avail(st3m_audio_input_source_t source);
bool st3m_audio_input_thru_get_source_avail(st3m_audio_input_source_t source);

/*
HEADPHONE PORT POLICY

Under normal circumstances it is an important feature to have a reliable speaker
mute when plugging in headphones. However, since the headphone port on the badge
can also be used for badge link, there are legimate cases where it is desirable
to have the speakers unmuted while a cable is plugged into the jack.

As a person who plugs in the headphones on the tram, doesn't put them on, turns
on music to check if it's not accidentially playing on speakers and then finally
puts on headphones (temporarily, of course, intermittent checks if the speakers
didn't magically turn on are scheduled according to our general anxiety level)
we wish to make it difficult to accidentially have sound coming from the
speakers.

Our proposed logic is as follows (excluding boot conditions):

1) Badge link TX cannot be enabled for any of the headphone jack pins without a
cable detected in the jack. This is to protect users from plugging in headphones
while badge link is active and receiving a short but potentially very loud burst
of digital data before the software can react to the state change.

2) If the software detects that the headphone jack has changed from unplugged to
plugged it *always* turns off speakers, no exceptions.

3) If a user wishes to TX on headphone badge link, they must confirm a warning
that having headphones plugged in may potentially cause hearing damage *every
time*.

4) If a user wishes to RX or TX on headphone badge link while playing sound on
the onboard speakers, they must confirm a warning *every time*.

We understand that these means seem extreme, but we find them to be a sensible
default configuration to make sure people can safely operate the device without
needing to refer to a manual.

(TX here means any state that is not constantly ~GND with whatever impedance.
While there are current limiting resistors (value TBD at the time of writing,
but presumably 100R-470R) in series with the GPIOs, they still can generate
quite some volume with standard 40Ohm-ish headphones. Ideally the analog switch
will never switch to the GPIOs without a cable plugged in.)
*/
