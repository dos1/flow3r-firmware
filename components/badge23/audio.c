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

#define TIMEOUT_MS 1000

#define I2C_MASTER_NUM 0 /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */

static void audio_player_task(void* arg);

#define DMA_BUFFER_SIZE 64
#define DMA_BUFFER_COUNT 2
#define I2S_PORT 0

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

// irregular register value -> vol mapping taken from codec datasheet
typedef struct {
    uint8_t register_value;
    float volume_dB;
} vol_map_t;

const uint8_t speaker_map_len = 40;
const vol_map_t speaker_map[] = {{0x3F, +14}, {0x3E, +13.5}, {0x3D, +13}, {0x3C, +12.5}, {0x3B, +12}, {0x3A, +11.5}, {0x39, +11}, {0x38, +10.5}, {0x37, +10}, {0x36, +9.5}, {0x35, +9}, {0x34, +8}, {0x33, +7}, {0x32, +6}, {0x31, +5}, {0x30, +4}, {0x2F, +3}, {0x2E, +2}, {0x2D, +1}, {0x2C, +0}, {0x2B, -1}, {0x2A, -2}, {0x29, -3}, {0x28, -4}, {0x27, -5}, {0x26, -6}, {0x25, -8}, {0x24, -10}, {0x23, -12}, {0x22, -14}, {0x21, -17}, {0x20, -20}, {0x1F, -23}, {0x1E, -26}, {0x1D, -29}, {0x1C, -32}, {0x1B, -36}, {0x1A, -40}, {0x19, -44}, {0x18, -48}};

const uint8_t headphones_map_len = 32;
const vol_map_t headphones_map[] = {{0x1F, +3}, {0x1E, +2.5}, {0x1D, +2}, {0x1C, +1.5}, {0x1B, +1}, {0x1A, +0}, {0x19, -1}, {0x18, -2}, {0x17, -3}, {0x16, -4}, {0x15, -5}, {0x14, -7}, {0x13, -9}, {0x12, -11}, {0x11, -13}, {0x10, -15}, {0x0F, -17}, {0x0E, -19}, {0x0D, -22}, {0x0C, -25}, {0x0B, -28}, {0x0A, -31}, {0x09, -34}, {0x08, -37}, {0x07, -40}, {0x06, -43}, {0x06, -47}, {0x04, -51}, {0x03, -55}, {0x02, -59}, {0x01, -63}, {0x00, -67}};

void _audio_headphones_set_volume_dB(float vol_dB, bool mute){
    uint8_t map_index = headphones_map_len - 1;
    for(; map_index; map_index--){
        if(headphones_map[map_index].volume_dB >= vol_dB) break; 
    }
    uint8_t reg = headphones_map[map_index].register_value;
    reg = (mute ? (1 << 7) : 0) | reg;
    max98091_i2c_write(0x2C, reg); //left chan
    max98091_i2c_write(0x2D, reg); //right chan
    // note: didn't check if chan physically mapped to l/r or flipped.

    // do the fine steps in software
    // note: synchronizing both hw and software volume changes is somewhat tricky
    float hardware_volume_dB = headphones_map[map_index].volume_dB;
    float software_volume_dB = vol_dB - hardware_volume_dB;
    if(software_volume_dB > 0) software_volume_dB = 0;
    //if(!software_volume_enabled) software_volume_dB = 0; // breaks p1, might add option once it is retired
    software_volume = (int32_t) (32768 * exp(software_volume_dB * NAT_LOG_DB));
    headphones_volume_dB = hardware_volume_dB + software_volume_dB;
}

void _audio_speaker_set_volume_dB(float vol_dB, bool mute){
    uint8_t map_index = speaker_map_len - 1;
    for(; map_index; map_index--){
        if(speaker_map[map_index].volume_dB >= vol_dB) break; 
    }

    uint8_t reg = speaker_map[map_index].register_value;
    reg = (mute ?  (1 << 7) : 0) | reg;
    max98091_i2c_write(0x31, reg); //left chan
    max98091_i2c_write(0x32, reg); //right chan
    //note: didn't check if chan physically mapped to l/r or flipped.

    // do the fine steps in software
    // note: synchronizing both hw and software volume changes is somewhat tricky
    float hardware_volume_dB = speaker_map[map_index].volume_dB;
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

#elif defined(CONFIG_BADGE23_HW_GEN_P1)

#define MAX_VOLUME_DB 10
#define MIN_VOLUME_DB (-80)

int32_t software_volume_premute; // ugly but this is an old prototype that will be phased out soon

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

void _audio_speaker_set_volume_dB(float vol_dB, bool mute){
    int32_t buf =  32767 * exp(vol_dB * NAT_LOG_DB);
    software_volume_premute = buf;
    if(mute){
        software_volume = 0;
    } else {
        software_volume = software_volume_premute;
    }
    speaker_volume_dB = vol_dB;
}

void _audio_headphones_set_volume_dB(float vol_dB, bool mute){
}

void_audio_headphones_set_mute(uint8_t mute){
    headphones_mute = 1;
};

void audio_speaker_set_mute(uint8_t mute){
    speaker_mute = mute;
    if(speaker_mute){
        software_volume = 0;
    } else {
        software_volume = software_volume_premute;
    }
}

#else
#error "audio not implemented for this badge generation"
#endif

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
#if defined(CONFIG_BADGE23_HW_GEN_P1)
    line_in_connected = 0;
#elif defined(CONFIG_BADGE23_HW_GEN_P3) || defined(CONFIG_BADGE23_HW_GEN_P4)
    line_in_connected = 1;
#elif defined(CONFIG_BADGE23_HW_GEN_P6)
    line_in_connected = 0; //TODO: read port expander
#endif

#if defined(CONFIG_BADGE23_HW_GEN_P1)
    headphones_connected = 0;
    headset_connected = 0;
#elif defined(CONFIG_BADGE23_HW_GEN_P3) || defined(CONFIG_BADGE23_HW_GEN_P4)  || defined(CONFIG_BADGE23_HW_GEN_P6)
    static uint8_t jck_prev = 255; // unreachable value -> initial comparision always untrue
    uint8_t jck = max98091_i2c_read(0x02);
    if(jck == 6){
        headphones_connected = 0;
        headset_connected = 0;
    } else if(jck == 0){
        headphones_connected = 1;
        headset_connected = 0;
    } else if(jck == 2){
        headphones_connected = 1;
        headset_connected = 1;
    }

    if(jck != jck_prev){ // update volume to trigger mutes if needed
        audio_speaker_set_volume_dB(speaker_volume_dB);
        audio_headphones_set_volume_dB(headphones_volume_dB);
    }
    jck_prev = jck;
#endif
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
    // TODO: this assumes I2C is already initialized
    init_scope(241);
    i2s_init();
    audio_update_jacksense();
    //ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
    TaskHandle_t handle;
    xTaskCreate(&audio_player_task, "Audio player", 3000, NULL, configMAX_PRIORITIES - 1, &handle);
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
            sample = software_volume * (sample/10);
            if(sample > 32767) sample = 32767;
            if(sample < -32767) sample = -32767;
            buffer[i] = (int16_t) sample;
            buffer[i+1] = buffer[i];
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
