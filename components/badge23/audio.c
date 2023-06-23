#include "badge23/audio.h"

#include "st3m_scope.h"

#include "driver/i2s.h"
#include "flow3r_bsp_i2c.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#define TIMEOUT_MS 1000

static void audio_player_task(void* arg);

#define DMA_BUFFER_SIZE 64
#define DMA_BUFFER_COUNT 4
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

#if defined(CONFIG_BADGE23_HW_GEN_P3) || defined(CONFIG_BADGE23_HW_GEN_P4) || defined(CONFIG_BADGE23_HW_GEN_P6)

static uint8_t max98091_i2c_read(const uint8_t reg)
{
    const uint8_t tx[] = {reg};
    uint8_t rx[1];
    flow3r_bsp_i2c_write_read_device(flow3r_i2c_addresses.codec, tx, sizeof(tx), rx, sizeof(rx), TIMEOUT_MS / portTICK_PERIOD_MS);
    // TODO(q3k): handle error
    return rx[0];
}

static esp_err_t max98091_i2c_write(const uint8_t reg, const uint8_t data)
{
    const uint8_t tx[] = {reg, data};
    return flow3r_bsp_i2c_write_to_device(flow3r_i2c_addresses.codec, tx, sizeof(tx), TIMEOUT_MS / portTICK_PERIOD_MS);
}

static esp_err_t max98091_i2c_write_readback(const uint8_t reg, const uint8_t data)
{
    esp_err_t ret = max98091_i2c_write(reg, data);
    if (ret != ESP_OK) {
        return ret;
    }
    if (max98091_i2c_read(reg) != data) {
        printf("readback of %04X to %02X write failed\n", data, reg);
    }
    return ESP_OK;
}

void audio_codec_i2c_write(const uint8_t reg, const uint8_t data) {
    max98091_i2c_write_readback(reg, data);
}

void audio_headphones_line_in_set_hardware_thru(bool enable){
    max98091_i2c_write_readback(0x2B, (1<<5) | (1<<4)); // Enable Headphone Mixer
    uint8_t trust_issue = enable ? 1 : 0;
    max98091_i2c_write_readback(0x29, (trust_issue<<2) | (1<<1)); // Line A + DAC_L -> Left HP
    max98091_i2c_write_readback(0x2A, (trust_issue<<3) | (1<<0)); // LIne B + DAC_R -> Right HP
}


void audio_speaker_line_in_set_hardware_thru(bool enable){
    uint8_t trust_issue = enable ? 1 : 0;
    max98091_i2c_write_readback(0x2E, (trust_issue<<2) | (1<<1)); // Line A -> Left Speaker
    max98091_i2c_write_readback(0x2F, (trust_issue<<3) | (1<<0)); // LIne B -> Right Speaker
}


void audio_line_in_set_hardware_thru(bool enable){
    audio_speaker_line_in_set_hardware_thru(enable);
    audio_headphones_line_in_set_hardware_thru(enable);
}

static void onboard_mic_set_power(bool enable){
    uint8_t trust_issue = enable ? 1 : 0;
    max98091_i2c_write_readback(0x13, (1<<4) | (1<<5) | (trust_issue<<1) | (trust_issue<<0) );
}

#define ADC_MIXER_MIC_1     5
#define ADC_MIXER_LINE_IN_B    4
#define ADC_MIXER_LINE_IN_A    3

static void adc_left_set_mixer(uint8_t mask){
    max98091_i2c_write_readback(0x15, mask);
}

static void adc_right_set_mixer(uint8_t mask){
    max98091_i2c_write_readback(0x16, mask);
}

void audio_input_set_source(uint8_t source){
    if(source == AUDIO_INPUT_SOURCE_NONE){
        onboard_mic_set_power(0);
        adc_left_set_mixer(0);
        adc_right_set_mixer(0);
        input_source = source;
    } else if(source == AUDIO_INPUT_SOURCE_LINE_IN){
        onboard_mic_set_power(0);
        adc_left_set_mixer(1<<ADC_MIXER_LINE_IN_A);
        adc_right_set_mixer(1<<ADC_MIXER_LINE_IN_B);
        input_source = source;
    } else if(source == AUDIO_INPUT_SOURCE_HEADSET_MIC){
        onboard_mic_set_power(0);
        adc_left_set_mixer(1<<ADC_MIXER_MIC_1);
        adc_right_set_mixer(1<<ADC_MIXER_MIC_1);
        input_source = source;
    } else if(source == AUDIO_INPUT_SOURCE_ONBOARD_MIC){
        onboard_mic_set_power(1);
        adc_left_set_mixer(0);
        adc_right_set_mixer(0);
        input_source = source;
    }
}

static void init_codec()
{
    // Enable CODEC

    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x00, 0x80)); // shutdown
    vTaskDelay(10 / portTICK_PERIOD_MS);

    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x45, 0)); // shutdown

    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x1b, 1 << 4)); // pclk = mclk / 1

    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x26,  (1 << 7) | (1 << 6) | (1 << 5) | (1 << 2))); // music, dc filter in record and playback

    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x06, 1 << 2)); // Sets up DAI for left-justified slave mode operation.
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x07, 1 << 5)); // Sets up the DAC to speaker path

    // Somehow this was needed to get an input signal to the ADC, even though
    // all other registers should be taken care of later. Don't know why.
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x09, 1 << 6)); // Sets up the line in to adc path

    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x25, (1 << 1) | (1 << 0))); // SDOUT, SDIN enabled
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x42, 1 << 0)); // bandgap bias
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x43, 1 << 0)); // high performane mode

    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x3E, (1<<4) |(1<<3) | (1<<2) | (1<<1) | (1<<0))); // enable micbias, line input amps, ADCs
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x0D, (1<<3) | (1<<2))); // IN3 SE -> Line A, IN4 SE -> Line B
    //ESP_ERROR_CHECK(max98091_i2c_write_readback(0x44, (1<<2) | (1<<1) | (1<<0) )); // 128x oversampling, dithering, high performance ADC
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x44, (1<<1) | (1<<0) )); // 64x oversampling, dithering, high performance ADC

    // Enable headset mic preamp
    max98091_i2c_write_readback(0x13, 0);
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x0F, (1<<0) )); // IN5/IN6 to MIC1

    audio_line_in_set_hardware_thru(0);
    audio_headset_set_gain_dB(0);
    audio_input_set_source(AUDIO_INPUT_SOURCE_NONE);
    audio_input_thru_set_volume_dB(-20); //mute is on by default

    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x3F, (1<<1) | (1<<0))); // output enable: enable dacs

    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x45, 1<<7)); // power up
    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x3F, (1<<7) | (1<<6) | (1<<5) | (1<<4) | (1<<1) | (1<<0))); // enable outputs, dacs

    //max98091_i2c_write_readback(0x27, (1<<4) | (1<<5)); // full playback gain

    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x41, 0x0)); // disable all digital filters except for dc blocking

    ESP_ERROR_CHECK(max98091_i2c_write_readback(0x3D, 1<<7)); // jack detect enable
    printf("4 readbacks failing here is normal dw ^w^\n");
}

static void i2s_init(void){
    init_codec();
    vTaskDelay(100 / portTICK_PERIOD_MS); // dunno if necessary
    
    static const i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX,
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = 16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_LSB,
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
        .data_in_num = 13,
    };
    ESP_ERROR_CHECK(i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL));

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

uint8_t audio_headset_set_gain_dB(uint8_t gain_dB){
    if(gain_dB < 0) gain_dB = 0;
    if(gain_dB > 50) gain_dB = 50;

    uint8_t hi_bits = 0b01;
    if(gain_dB > 30){
        gain_dB -= 30;
        hi_bits = 0b11;
    } else if(gain_dB > 20){
        gain_dB -= 20;
        hi_bits = 0b10;
    }
    uint8_t reg = (hi_bits<<5) | (0x14-gain_dB);
    max98091_i2c_write(0x10, reg);
    headset_gain = gain_dB;
    return headset_gain;
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

void audio_headphones_set_mute(uint8_t mute){
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

void audio_headphones_line_in_set_hardware_thru(bool enable){}
void audio_speaker_line_in_set_hardware_thru(bool enable){}
void audio_line_in_set_hardware_thru(bool enable){}
void audio_input_set_source(uint8_t source){}
uint8_t audio_headset_set_gain_dB(uint8_t gain_dB){ return 0;}

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

    // TODO: this assumes I2C is already initialized
    i2s_init();
    audio_update_jacksense();
    TaskHandle_t handle;
    xTaskCreate(&audio_player_task, "Audio player", 3000, NULL, configMAX_PRIORITIES - 1, &handle);
    audio_player_function = audio_player_function_dummy;
}

static void _audio_deinit(void){
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
    int16_t buffer_tx[DMA_BUFFER_SIZE * 2];
    int16_t buffer_rx[DMA_BUFFER_SIZE * 2];
    memset(buffer_tx, 0, sizeof(buffer_tx));
    memset(buffer_rx, 0, sizeof(buffer_rx));
    size_t count;

    while(true) {
        count = 0;

#if defined(CONFIG_BADGE23_HW_GEN_P3) || defined(CONFIG_BADGE23_HW_GEN_P4) || defined(CONFIG_BADGE23_HW_GEN_P6)
        count = 0;
        i2s_read(I2S_PORT, buffer_rx, sizeof(buffer_rx), &count, 1000);
        if (count != sizeof(buffer_rx)) {
            printf("i2s_read_bytes: count (%d) != length (%d)\n", count, sizeof(buffer_rx));
            abort();
        }
#endif

        (* audio_player_function)(buffer_rx, buffer_tx, DMA_BUFFER_SIZE*2);

        for(int i = 0; i < (DMA_BUFFER_SIZE * 2); i += 2){
            int32_t acc = buffer_tx[i];

            acc = (acc * software_volume) >> 15;

            if(!input_thru_mute){
                acc += (((int32_t) buffer_rx[i]) * input_thru_vol_int) >> 15;
            }
            buffer_tx[i] = acc;
        }

        i2s_write(I2S_PORT, buffer_tx, sizeof(buffer_tx), &count, 1000);
        if (count != sizeof(buffer_tx)) {
            printf("i2s_write_bytes: count (%d) != length (%d)\n", count, sizeof(buffer_tx));
            abort();
        }

    }
}

void audio_init() { _audio_init(); }
