#include "badge23/audio.h"

#include "st3m_scope.h"

#include "flow3r_bsp.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#define TIMEOUT_MS 1000

static void audio_player_task(void* arg);

// used for exp(vol_dB * NAT_LOG_DB)
#define NAT_LOG_DB 0.1151292546497023

// placeholder for "fake mute" -inf dB (we know floats can do that but we have trust issues when using NAN)
#define SILLY_LOW_VOLUME_DB (-10000.)

static bool headphones_connected = 0;
static bool headset_connected = 0;
static bool line_in_connected = 0;
static bool headphones_detection_override = 0;
static int32_t software_volume = 0;

// maybe struct these someday but eh it works
static float headphones_volume_dB = 0;
static bool headphones_mute = 0;
const static float headphones_maximum_volume_system_dB = 3;
static float headphones_maximum_volume_user_dB = headphones_maximum_volume_system_dB;
static float headphones_minimum_volume_user_dB = headphones_maximum_volume_system_dB - 70;

static float speaker_volume_dB = 0;
static bool speaker_mute = 0;
const static float speaker_maximum_volume_system_dB = 14;
static float speaker_maximum_volume_user_dB = speaker_maximum_volume_system_dB;
static float speaker_minimum_volume_user_dB = speaker_maximum_volume_system_dB - 60;

static uint8_t input_source = AUDIO_INPUT_SOURCE_NONE;
static uint8_t headset_gain = 0;

static int32_t input_thru_vol;
static int32_t input_thru_vol_int;
static int32_t input_thru_mute = 1;

uint8_t audio_headset_is_connected(){ return headset_connected; }
uint8_t audio_headphones_are_connected(){ return headphones_connected || headphones_detection_override; }
float audio_headphones_get_volume_dB(){ return headphones_volume_dB; }
float audio_speaker_get_volume_dB(){ return speaker_volume_dB; }
float audio_headphones_get_minimum_volume_dB(){ return headphones_minimum_volume_user_dB; }
float audio_speaker_get_minimum_volume_dB(){ return speaker_minimum_volume_user_dB; }
float audio_headphones_get_maximum_volume_dB(){ return headphones_maximum_volume_user_dB; }
float audio_speaker_get_maximum_volume_dB(){ return speaker_maximum_volume_user_dB; }
uint8_t audio_headphones_get_mute(){ return headphones_mute ? 1 : 0; }
uint8_t audio_speaker_get_mute(){ return speaker_mute ? 1 : 0; }
uint8_t audio_input_get_source(){ return input_source; }
uint8_t audio_headset_get_gain_dB(){ return headset_gain; }


void audio_codec_i2c_write(const uint8_t reg, const uint8_t data) {
    flow3r_bsp_audio_register_poke(reg, data);
}

void _audio_headphones_set_volume_dB(float vol_dB, bool mute){
    float hardware_volume_dB = flow3r_bsp_audio_headphones_set_volume(mute, vol_dB);
    // note: didn't check if chan physically mapped to l/r or flipped.

    // do the fine steps in software
    // note: synchronizing both hw and software volume changes is somewhat tricky
    float software_volume_dB = vol_dB - hardware_volume_dB;
    if(software_volume_dB > 0) software_volume_dB = 0;
    //if(!software_volume_enabled) software_volume_dB = 0; // breaks p1, might add option once it is retired
    software_volume = (int32_t) (32768 * exp(software_volume_dB * NAT_LOG_DB));
    headphones_volume_dB = hardware_volume_dB + software_volume_dB;
}

void _audio_speaker_set_volume_dB(float vol_dB, bool mute){
    float hardware_volume_dB = flow3r_bsp_audio_speaker_set_volume(mute, vol_dB);
    //note: didn't check if chan physically mapped to l/r or flipped.

    // do the fine steps in software
    // note: synchronizing both hw and software volume changes is somewhat tricky
    float software_volume_dB = vol_dB - hardware_volume_dB;
    if(software_volume_dB > 0) software_volume_dB = 0;
    //if(!software_volume_enabled) software_volume_dB = 0; // breaks p1, might add option once it is retired
    software_volume = (int32_t) (32768. * exp(software_volume_dB * NAT_LOG_DB));
    speaker_volume_dB = hardware_volume_dB + software_volume_dB;
}

void audio_headphones_set_mute(uint8_t mute){
    headphones_mute = mute;
    audio_headphones_set_volume_dB(headphones_volume_dB);
}

void audio_speaker_set_mute(uint8_t mute){
    speaker_mute = mute;
    audio_speaker_set_volume_dB(speaker_volume_dB);
}

uint8_t audio_headset_set_gain_dB(uint8_t gain_dB){
    flow3r_bsp_audio_headset_set_gain_dB(gain_dB);
    headset_gain = gain_dB;
    return headset_gain;
}

void audio_input_set_source(uint8_t source) {
    switch (source) {
    case AUDIO_INPUT_SOURCE_NONE:
        flow3r_bsp_audio_input_set_source(flow3r_bsp_audio_input_source_none);
        break;
    case AUDIO_INPUT_SOURCE_LINE_IN:
        flow3r_bsp_audio_input_set_source(flow3r_bsp_audio_input_source_line_in);
        break;
    case AUDIO_INPUT_SOURCE_HEADSET_MIC:
        flow3r_bsp_audio_input_set_source(flow3r_bsp_audio_input_source_headset_mic);
        break;
    case AUDIO_INPUT_SOURCE_ONBOARD_MIC:
        flow3r_bsp_audio_input_set_source(flow3r_bsp_audio_input_source_onboard_mic);
        break;
    }
    input_source = source;
}

void audio_headphones_line_in_set_hardware_thru(bool enable) {
    flow3r_bsp_audio_headphones_line_in_set_hardware_thru(enable);
}

void audio_speaker_line_in_set_hardware_thru(bool enable) {
    flow3r_bsp_audio_speaker_line_in_set_hardware_thru(enable);
}

void audio_line_in_set_hardware_thru(bool enable) {
    flow3r_bsp_audio_line_in_set_hardware_thru(enable);
}

float audio_speaker_set_volume_dB(float vol_dB){
    bool mute  = speaker_mute || audio_headphones_are_connected();
    if(vol_dB > speaker_maximum_volume_user_dB) vol_dB = speaker_maximum_volume_user_dB;
    if(vol_dB < speaker_minimum_volume_user_dB){
        vol_dB = SILLY_LOW_VOLUME_DB; // fake mute
        mute = 1;
    }
    _audio_speaker_set_volume_dB(vol_dB, mute);
    return speaker_volume_dB;
}

float audio_headphones_set_volume_dB(float vol_dB){
    bool mute  = headphones_mute || (!audio_headphones_are_connected());
    if(vol_dB > headphones_maximum_volume_user_dB) vol_dB = headphones_maximum_volume_user_dB;
    if(vol_dB < headphones_minimum_volume_user_dB){
        vol_dB = SILLY_LOW_VOLUME_DB; // fake mute
        mute = 1;
    }
    _audio_headphones_set_volume_dB(vol_dB, mute);
    return headphones_volume_dB;
}

void audio_headphones_detection_override(uint8_t enable){
    headphones_detection_override = enable;
    audio_headphones_set_volume_dB(headphones_volume_dB);
    audio_speaker_set_volume_dB(speaker_volume_dB);
}

float audio_headphones_adjust_volume_dB(float vol_dB){
    if(audio_headphones_get_volume_dB() < headphones_minimum_volume_user_dB){ //fake mute
        if(vol_dB > 0){
            return audio_headphones_set_volume_dB(headphones_minimum_volume_user_dB);
        } else {
            return audio_headphones_get_volume_dB();
        }
    } else { 
        return audio_headphones_set_volume_dB(headphones_volume_dB + vol_dB);
    }
}

float audio_speaker_adjust_volume_dB(float vol_dB){
    if(audio_speaker_get_volume_dB() < speaker_minimum_volume_user_dB){ //fake mute
        if(vol_dB > 0){
            return audio_speaker_set_volume_dB(speaker_minimum_volume_user_dB);
            printf("hi");
        } else {
            return audio_speaker_get_volume_dB();
        }
    } else { 
        return audio_speaker_set_volume_dB(speaker_volume_dB + vol_dB);
    }
}

float audio_adjust_volume_dB(float vol_dB){
    if(audio_headphones_are_connected()){
        return audio_headphones_adjust_volume_dB(vol_dB);
    } else {
        return audio_speaker_adjust_volume_dB(vol_dB);
    }
}

float audio_set_volume_dB(float vol_dB){
    if(audio_headphones_are_connected()){
        return audio_headphones_set_volume_dB(vol_dB);
    } else {
        return audio_speaker_set_volume_dB(vol_dB);
    }
}

float audio_get_volume_dB(){
    if(audio_headphones_are_connected()){
        return audio_headphones_get_volume_dB();
    } else {
        return audio_speaker_get_volume_dB();
    }
}

void audio_set_mute(uint8_t mute){
    if(audio_headphones_are_connected()){
        audio_headphones_set_mute(mute);
    } else {
        audio_speaker_set_mute(mute);
    }
}

uint8_t audio_get_mute(){
    if(audio_headphones_are_connected()){
        return audio_headphones_get_mute();
    } else {
        return audio_speaker_get_mute();
    }
}

float audio_headphones_set_maximum_volume_dB(float vol_dB){
    if(vol_dB > headphones_maximum_volume_system_dB) vol_dB = headphones_maximum_volume_system_dB;
    if(vol_dB < headphones_minimum_volume_user_dB) vol_dB = headphones_minimum_volume_user_dB;
    headphones_maximum_volume_user_dB = vol_dB;
    return headphones_maximum_volume_user_dB;
}

float audio_headphones_set_minimum_volume_dB(float vol_dB){
    if(vol_dB > headphones_maximum_volume_user_dB) vol_dB = headphones_maximum_volume_user_dB;
    if((vol_dB + 1) < SILLY_LOW_VOLUME_DB) vol_dB = SILLY_LOW_VOLUME_DB + 1.;
    headphones_minimum_volume_user_dB = vol_dB;
    return headphones_minimum_volume_user_dB;
}

float audio_speaker_set_maximum_volume_dB(float vol_dB){
    if(vol_dB > speaker_maximum_volume_system_dB) vol_dB = speaker_maximum_volume_system_dB;
    if(vol_dB < speaker_minimum_volume_user_dB) vol_dB = speaker_minimum_volume_user_dB;
    speaker_maximum_volume_user_dB = vol_dB;
    return speaker_maximum_volume_user_dB;
}

float audio_speaker_set_minimum_volume_dB(float vol_dB){
    if(vol_dB > speaker_maximum_volume_user_dB) vol_dB = speaker_maximum_volume_user_dB;
    if((vol_dB + 1) < SILLY_LOW_VOLUME_DB) vol_dB = SILLY_LOW_VOLUME_DB + 1.;
    speaker_minimum_volume_user_dB = vol_dB;
    return speaker_minimum_volume_user_dB;
}

float audio_speaker_get_volume_relative(){
    float ret = audio_speaker_get_volume_dB();
    if(ret < speaker_minimum_volume_user_dB) return 0; // fake mute
    float vol_range = speaker_maximum_volume_user_dB - speaker_minimum_volume_user_dB;
    ret -= speaker_minimum_volume_user_dB; // shift to above zero
    ret /= vol_range; // restrict to 0..1 range
    return (ret*0.99) + 0.01; // shift to 0.01 to 0.99 range to distinguish from fake mute
}

float audio_headphones_get_volume_relative(){
    float ret = audio_headphones_get_volume_dB();
    if(ret < headphones_minimum_volume_user_dB) return 0; // fake mute
    float vol_range = headphones_maximum_volume_user_dB - headphones_minimum_volume_user_dB;
    ret -= headphones_minimum_volume_user_dB; // shift to above zero
    ret /= vol_range; // restrict to 0..1 range
    return (ret*0.99) + 0.01; // shift to 0.01 to 0.99 range to distinguish from fake mute
}

float audio_get_volume_relative(){
    if(audio_headphones_are_connected()){
        return audio_headphones_get_volume_relative();
    } else {
        return audio_speaker_get_volume_relative();
    }
}

void audio_update_jacksense(){
    static uint8_t jck_prev = 255; // unreachable value -> initial comparision always untrue

    flow3r_bsp_audio_jacksense_state_t st;
    flow3r_bsp_audio_read_jacksense(&st);

    headphones_connected = st.headphones;
    headset_connected = st.headset;
    line_in_connected = st.line_in;

    // Update volume to trigger mutes if needed. But only do that if the
    // jacks actually changed.
    uint8_t jck = (st.headphones ? 1 : 0) | (st.headset ? 2 : 0) | (st.line_in ? 4 : 0);
    if(jck != jck_prev){
        audio_speaker_set_volume_dB(speaker_volume_dB);
        audio_headphones_set_volume_dB(headphones_volume_dB);
    }
    jck_prev = jck;
}


void audio_player_function_dummy(int16_t * rx, int16_t * tx, uint16_t len){
    for(uint16_t i = 0; i < len; i++){
        tx[i] = 0;
    }
}

static audio_player_function_type audio_player_function = audio_player_function_dummy;

void audio_set_player_function(audio_player_function_type fun){
    // ...wonder how unsafe this is
    audio_player_function = fun;
}

static void _audio_init(void) {
    st3m_scope_init();

    flow3r_bsp_audio_init();
	// mute is on by default
    audio_input_thru_set_volume_dB(-20);

    // TODO: this assumes I2C is already initialized
    audio_update_jacksense();
    TaskHandle_t handle;
    xTaskCreate(&audio_player_task, "Audio player", 3000, NULL, configMAX_PRIORITIES - 1, &handle);
    audio_player_function = audio_player_function_dummy;
}

float audio_input_thru_set_volume_dB(float vol_dB){
    if(vol_dB > 0) vol_dB = 0;
    input_thru_vol_int = (int32_t) (32768. * exp(vol_dB * NAT_LOG_DB));
    input_thru_vol = vol_dB;
    return input_thru_vol;
}

float audio_input_thru_get_volume_dB(){ return input_thru_vol; }
void audio_input_thru_set_mute(bool mute){ input_thru_mute = mute; }
bool audio_input_thru_get_mute(){ return input_thru_mute; }

static void audio_player_task(void* arg) {
    int16_t buffer_tx[FLOW3R_BSP_AUDIO_DMA_BUFFER_SIZE * 2];
    int16_t buffer_rx[FLOW3R_BSP_AUDIO_DMA_BUFFER_SIZE * 2];
    memset(buffer_tx, 0, sizeof(buffer_tx));
    memset(buffer_rx, 0, sizeof(buffer_rx));
    size_t count;

    while(true) {
        count = 0;

        count = 0;
        flow3r_bsp_audio_read(buffer_rx, sizeof(buffer_rx), &count, 1000);
        if (count != sizeof(buffer_rx)) {
            printf("i2s_read_bytes: count (%d) != length (%d)\n", count, sizeof(buffer_rx));
            abort();
        }

        (* audio_player_function)(buffer_rx, buffer_tx, FLOW3R_BSP_AUDIO_DMA_BUFFER_SIZE*2);

		bool hwmute = flow3r_bsp_audio_has_hardware_mute();
        if (!hwmute && !(!speaker_mute || !headphones_mute)) {
            // Software muting needed. Only used on P1.
            for(int i = 0; i < (FLOW3R_BSP_AUDIO_DMA_BUFFER_SIZE * 2); i += 2){
                buffer_tx[i] = 0;
            }
        } else {
            for(int i = 0; i < (FLOW3R_BSP_AUDIO_DMA_BUFFER_SIZE * 2); i += 2){
                int32_t acc = buffer_tx[i];

                acc = (acc * software_volume) >> 15;

                if(!input_thru_mute){
                    acc += (((int32_t) buffer_rx[i]) * input_thru_vol_int) >> 15;
                }
                buffer_tx[i] = acc;
            }
        }

        flow3r_bsp_audio_write(buffer_tx, sizeof(buffer_tx), &count, 1000);
        if (count != sizeof(buffer_tx)) {
            printf("i2s_write_bytes: count (%d) != length (%d)\n", count, sizeof(buffer_tx));
            abort();
        }

    }
}

void audio_init() { _audio_init(); }
