#include "audio.h"
#include "synth.h" 
#include "scope.h"

#include "driver/i2s.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "../../py/mphal.h"

static void audio_player_task(void* arg);

#define DMA_BUFFER_SIZE     64
#define DMA_BUFFER_COUNT    2
#define I2S_PORT 0

static void i2s_init(void){
    
    static const i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = 16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        //.communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,

        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = 0, // default interrupt priority
        .dma_buf_count = DMA_BUFFER_COUNT,
        .dma_buf_len = DMA_BUFFER_SIZE,
        .use_apll = false
    };
    static const i2s_pin_config_t pin_config = {
        .bck_io_num = 13,
        .mck_io_num = 11,
        .ws_io_num = 12,
        .data_out_num = 14,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

    i2s_set_pin(I2S_PORT, &pin_config);

}

typedef struct _audio_source_t{
    void * render_data;
    float (* render_function)(void *);
    uint16_t index;
    struct _audio_source_t * next;
} audio_source_t;

static audio_source_t * _audio_sources = NULL;

uint16_t add_audio_source(void * render_data, void * render_function){
    //construct audio source struct
    audio_source_t * src = malloc(sizeof(audio_source_t));
    if(src == NULL) return;
    src->render_data = render_data;
    src->render_function = render_function;
    src->next = NULL;
    src->index = 0;

    //handle empty list special case
    if(_audio_sources == NULL){
        _audio_sources = src;
        return 0; //only nonempty lists from here on out!
    }

    //searching for lowest unused index
    audio_source_t * index_source = _audio_sources;
    while(1){
        if(src->index == (index_source->index)){
            src->index++; //someone else has index already, try next
            index_source = _audio_sources; //start whole list for new index
        } else {
            index_source = index_source->next;
        }
        if(index_source == NULL){ //traversed the entire list
            break;
        }
    }

    audio_source_t * audio_source = _audio_sources;
    //append new source to linked list
    while(audio_source != NULL){
        if(audio_source->next == NULL){
            audio_source->next = src;
            break;
        } else {
        audio_source = audio_source->next;
        }
    }
    return src->index;
}

void remove_audio_source(uint16_t index){
    audio_source_t * audio_source = _audio_sources;
    audio_source_t * start_gap = NULL;

    while(audio_source != NULL){
        if(index == audio_source->index){
            if(start_gap == NULL){
                _audio_sources = audio_source->next;
            } else {
                start_gap->next = audio_source->next;
            }
            vTaskDelay(20 / portTICK_PERIOD_MS); //give other tasks time to stop using
            free(audio_source); //terrible hack tbh
            break;
        }
        start_gap = audio_source;
        audio_source = audio_source->next;
    }
}

uint16_t count_audio_sources(){
    uint16_t ret = 0;
    audio_source_t * audio_source = _audio_sources;
    while(audio_source != NULL){
        audio_source = audio_source->next;
        ret++;
    }
    return ret;
}

static void _audio_init(void) {
    init_scope(241);
    i2s_init();
    //ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
    TaskHandle_t handle;
    xTaskCreate(&audio_player_task, "Audio player", 3000, NULL, configMAX_PRIORITIES - 1, &handle);
}

#define LR_PHASE 1
#define NAT_LOG_DB 0.1151292546497023

static uint16_t _global_vol = 3000;

void set_global_vol_dB(int8_t vol_dB){
    if(vol_dB < (BADGE_MIN_VOLUME_DB)){
        _global_vol = 0;
    } else {
        if(vol_dB > (BADGE_MAX_VOLUME_DB)) vol_dB = (BADGE_MAX_VOLUME_DB);
        uint16_t buf =  3000 * exp(vol_dB * NAT_LOG_DB);
        if(buf > (BADGE_VOLUME_LIMIT)) buf = (BADGE_VOLUME_LIMIT);
        _global_vol = buf;
    }
}

static void audio_player_task(void* arg) {
    int16_t buffer[DMA_BUFFER_SIZE * 2];
    memset(buffer, 0, sizeof(buffer));

    while(true) {

        for(int i = 0; i < (DMA_BUFFER_SIZE * 2); i += 2){
            float sample = 0;
            audio_source_t * audio_source = _audio_sources;
            while(audio_source != NULL){
                sample += (*(audio_source->render_function))(audio_source->render_data);
                audio_source = audio_source->next;
            }
            write_to_scope((int16_t) (1600. * sample));
            sample = _global_vol * sample;
            if(sample > 32767) sample = 32767;
            if(sample < -32767) sample = -32767;
            buffer[i] = (int16_t) sample;
            buffer[i+1] = LR_PHASE * buffer[i];
        }

        size_t count = 0;
        i2s_write(I2S_PORT, buffer, sizeof(buffer), &count, 1000);
        if (count != sizeof(buffer)) {
            printf("i2s_write_bytes: count (%d) != length (%d)\n", count, sizeof(buffer));
            abort();
        }
    }
}

void audio_init() { _audio_init(); }


/*
#define NAT_LOG_SEMITONE 0.05776226504666215

void synth_set_bend(int i, float bend){
    if(bend != bend) return;
    if((bend > -0.0001) && (bend < 0.0001)){
        synths[i].bend = 1;
    } else {
        synths[i].bend = exp(bend * NAT_LOG_SEMITONE);
    }
}
*/

/*
void synth_stop(int i){
    if(synths[i].env_phase){
        synths[i].env_phase = 3;
    }
}

void synth_fullstop(int i){
    synths[i].env_phase = 0;
}

void synth_start(int i){
    synths[i].env_phase = 1; //put into attack phase;
}

float synth_get_env(int i){
    return synths[i].env;
}
*/
