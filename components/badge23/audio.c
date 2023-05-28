#include "badge23/audio.h"
#include "badge23/synth.h" 
#include "badge23/scope.h"
#include "badge23/lock.h"
#include "badge23_hwconfig.h"

#include "driver/i2s.h"
#include "driver/i2c.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#define TIMEOUT_MS                  1000

#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */

static void audio_player_task(void* arg);

#define DMA_BUFFER_SIZE     64
#define DMA_BUFFER_COUNT    2
#define I2S_PORT 0

#if defined(CONFIG_BADGE23_HW_GEN_P3) || defined(CONFIG_BADGE23_HW_GEN_P4)
static uint8_t max98091_i2c_read(const uint8_t reg)
{
    const uint8_t tx[] = {reg};
    uint8_t rx[1];
    xSemaphoreTake(mutex_i2c, portMAX_DELAY);
    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_NUM, 0x10, tx, sizeof(tx), rx, sizeof(rx), TIMEOUT_MS / portTICK_PERIOD_MS);
    xSemaphoreGive(mutex_i2c);
    return rx[0];
}

static esp_err_t max98091_i2c_write(const uint8_t reg, const uint8_t data)
{
    const uint8_t tx[] = {reg, data};
    xSemaphoreTake(mutex_i2c, portMAX_DELAY);
    esp_err_t ret = i2c_master_write_to_device(I2C_MASTER_NUM, 0x10, tx, sizeof(tx), TIMEOUT_MS / portTICK_PERIOD_MS);
    xSemaphoreGive(mutex_i2c);
    return ret;
}

static esp_err_t max98091_i2c_write_readback(const uint8_t reg, const uint8_t data)
{
    const uint8_t tx[] = {reg, data};
    xSemaphoreTake(mutex_i2c, portMAX_DELAY);
    esp_err_t ret = i2c_master_write_to_device(I2C_MASTER_NUM, 0x10, tx, sizeof(tx), TIMEOUT_MS / portTICK_PERIOD_MS);
    xSemaphoreGive(mutex_i2c);
    if(max98091_i2c_read(reg) != data) printf("readback of %04X to %02X write failed\n", data, reg);
    return ret;
}

static void init_codec()
{
    // Enable CODEC

    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x00, 0x80)); // shutdown
    vTaskDelay(10 / portTICK_PERIOD_MS);

    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x45, 0)); // shutdown

    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x1b, 1 << 4)); // pclk = mclk / 1

    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x26,  (1 << 7) | (1 << 6))); // music, dc filter in record

    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x06, 1 << 2)); // Sets up DAI for left-justified slave mode operation.
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x07, 1 << 5)); // Sets up the DAC to speaker path

    // Somehow this was needed to get an input signal to the ADC, even though
    // all other registers should be taken care of later. Don't know why.
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x09, 1 << 6)); // Sets up the line in to adc path

    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x25, (1 << 1) | (1 << 0))); // SDOUT, SDIN enabled
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x42, 1 << 0)); // bandgap bias
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x43, 1 << 0)); // high performane mode

    // Table 51. Digital Audio Interface (DAI) Format Configuration Register

    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x2E, 1)); // Left DAC -> Left Speaker
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x2F, 2)); // Right DAC -> Right Speaker

    //max98091_i2c_write_readback(0x2E, (1<<2) | (1<<1)); // Line A -> Left Speaker
    //max98091_i2c_write_readback(0x2F, (1<<3) | (1<<0)); // LIne B -> Right Speaker

    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x29, 1)); // Left DAC -> Left HP
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x2A, 2)); // Right DAC -> Right HP

    // Mic bias is off
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x3E, (1<<4) |(1<<3) | (1<<2) | (1<<1) | (1<<0))); // enable micbias, line input amps, ADCs
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x0D, (1<<3) | (1<<2))); // IN3 SE -> Line A, IN4 SE -> Line B
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x15, (1<<4) )); // line B -> left ADC
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x16, (1<<3) )); // line A -> right ADC
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x44, (1<<2) | (1<<1) | (1<<0) )); // 128x oversampling, dithering, high performance ADC

    max98091_i2c_write_readback(0x13, (1<<4) | (1<<5) | (1<<1) | (1<<0) ); // enable digital mic

    // Enable headset mic
#if 0
    max98091_i2c_write_readback(0x13, 0);
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x0F, (0<<1) | (1<<0) )); // IN5/IN6 to MIC1
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x10, (1<<6) | (1<<4) | (1<<2) )); // 20 dB gain on MIC1
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x15, (1<<5) )); // MIC1 -> left ADC
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x16, (1<<5) )); // MIC1 -> right ADC
#endif

    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x3F, (1<<1) | (1<<0))); // output enable: enable dacs

    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x45, 1<<7)); // power up
    //max98091_i2c_write_readback(0x31, 0x2c); // 0db, no mute
    //max98091_i2c_write_readback(0x32, 0x2c); // 0db, no mute
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x3F, (1<<7) | (1<<6) | (1<<5) | (1<<4) | (1<<1) | (1<<0))); // enable outputs, dacs

    //max98091_i2c_write_readback(0x27, (1<<4) | (1<<5)); // full playback gain

    //max98091_i2c_write_readback(0x31, 0x3f); // +14 db speaker
    //max98091_i2c_write_readback(0x32, 0x3f); // +14 db speaker
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x41, 0x0));

    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x3D, 1<<7)); // jack detect enable
    printf("4 readbacks failing here is normal dw ^w^");
}

static void i2s_init(void){
    init_codec();
    vTaskDelay(100 / portTICK_PERIOD_MS); // dunno if necessary
    
    static const i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = 16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        //.communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_LSB,
        //^...technically wrong but works...? in idf v5 it's msb but don't try that late at night
        .intr_alloc_flags = 0, // default interrupt priority
        .dma_buf_count = DMA_BUFFER_COUNT,
        .dma_buf_len = DMA_BUFFER_SIZE,
        .use_apll = false
    };
    static const i2s_pin_config_t pin_config = {
        .bck_io_num = 10,
        .mck_io_num = 18,
        .ws_io_num = 11,
        .data_out_num = 12,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

    i2s_set_pin(I2S_PORT, &pin_config);

}

static uint8_t headphones_volume = 0x1A;
static uint8_t speaker_volume = 0x2C;

static bool headphones_connected = 0;
static bool headset_connected = 0;
static bool speaker_on = 1;

static void codec_set_headphones_volume_dB(float dB){
    uint8_t reg = 0x1A; // 0dB
    if(dB > 3.) dB = 3.; //max volume
    if(dB < -67.) dB = -67.; //min volume (could also trigger false mute someday)

    if(dB > 1.){ //0.5dB steps
        reg += 2.*(dB-1) + 0.5 + 1;
    } else if(dB > -5.){  //1dB steps
        reg += dB + 0.5;
    } else if(dB > -19.){ //2dB steps
        reg += 2*(dB +19) + 0.5 - 19;
    } else if(dB > -43.){ //3dB steps
        reg += 3.*(dB + 43.) + 0.5 - 43.;
    } else {              //4dB steps
        reg += 4.*(dB + 67.) + 0.5 - 67.;
    }

    headphones_volume = reg;
    max98091_i2c_write(0x2C, headphones_volume); //left chan
    max98091_i2c_write(0x2D, headphones_volume); //right chan
    //note: didn't check if chan physically mapped to l/r or flipped.
}

static void codec_set_spkr_volume_dB(float dB){
    // could add finer steps in software someday but since
    // lineout and speakers can be doing stuff at the same
    // time this needs to be thought through a little more
    uint8_t reg = 0x2C; // 0dB
    if(dB > 14.) dB = 14.; //max volume
    if(dB < -48.) dB = -48.; //min volume (could also trigger false mute someday)

    // not cool maxim not cool
    if(dB > 9.){ //0.5dB steps
        reg += 2.*(dB-9) + 0.5 + 9;
    } else if(dB > -6.){  //1dB steps
        reg += dB + 0.5;
    } else if(dB > -14.){ //2dB steps
        reg += 2*(dB +14) + 0.5 - 14;
    } else if(dB > -32.){ //3dB steps
        reg += 3.*(dB + 32.) + 0.5 - 32.;
    } else {              //4dB steps
        reg += 4.*(dB + 48.) + 0.5 - 48.;
    }

    speaker_volume = reg;
    reg = (speaker_on ? 0 : (1 << 7)) | reg;
    max98091_i2c_write(0x31, reg); //left chan
    max98091_i2c_write(0x32, reg); //right chan
    //note: didn't check if chan physically mapped to l/r or flipped.
}

static void unmute_speaker(){
    max98091_i2c_write(0x31, speaker_volume); //left chan
    max98091_i2c_write(0x32, speaker_volume); //right chan
    speaker_on = 1;
}

static void mute_speaker(){
    max98091_i2c_write(0x31, (1<<7) | speaker_volume); //left chan
    max98091_i2c_write(0x32, (1<<7) | speaker_volume); //right chan
    speaker_on = 0;
}

#elif defined(CONFIG_BADGE23_HW_GEN_P1)

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
#else
#error "audio not implemented for this badge generation"
#endif

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

#if defined(CONFIG_BADGE23_HW_GEN_P4) || defined(CONFIG_BADGE23_HW_GEN_P3)
void set_global_vol_dB(int8_t vol_dB){
    //note: _global_vol could be higher in this mode but we got plans trust us oka :D?
    if(speaker_on){
        codec_set_spkr_volume_dB(vol_dB);
    } else {
        codec_set_headphones_volume_dB(vol_dB);
    }
}

void audio_lineout_update_jacksense(){
    uint8_t jck = max98091_i2c_read(0x02);
    if(jck == 6){
        headphones_connected = 0;
        headset_connected = 0;
        if(!speaker_on) unmute_speaker();
    } else if(jck == 0){
        headphones_connected = 1;
        headset_connected = 0;
        if(speaker_on) mute_speaker();
    } else if(jck == 2){
        headphones_connected = 1;
        headset_connected = 1;
        if(speaker_on) mute_speaker();
    }
}
#elif defined(CONFIG_BADGE23_HW_GEN_P1)

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

void audio_lineout_update_jacksense(){ }
#endif

uint8_t audio_speaker_is_on(){ return speaker_on; }
uint8_t audio_headset_is_connected(){ return headset_connected; }
uint8_t audio_headphones_are_connected(){ return headphones_connected; }

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
