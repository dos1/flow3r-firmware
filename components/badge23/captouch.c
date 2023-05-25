//#include <stdio.h>
//#include <string.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "badge23_hwconfig.h"
#include <stdint.h>

#define PETAL_SEGMENT_TIP 0
#define PETAL_SEGMENT_LEFT 1
#define PETAL_SEGMENT_RIGHT 2
#define PETAL_SEGMENT_BASE 3

#if defined(CONFIG_BADGE23_HW_GEN_P3) || defined(CONFIG_BADGE23_HW_GEN_P4)
static const uint8_t top_map[] = {0, 0, 0, 2, 2, 2, 6, 6, 6, 4, 4, 4};
static const uint8_t top_stages = 12;
static const uint8_t bot_map[] = {1, 1, 3, 3, 5, 7, 7, 9, 9, 8, 8, 8};
static const uint8_t bot_stages = 12;
static const uint8_t top_segment_map[] = {1,3,2,2,3,1,1,3,2,1,3,2}; //PETAL_SEGMENT_*
static const uint8_t bot_segment_map[] = {3,0,3,0,0,0,3,0,3,1,2,3}; //PETAL_SEGMENT_*
static const uint8_t bot_stage_config[] = {0,1,2,3,5,6,7,8,9,10,11,12};
#define DEFAULT_THRES_TOP 8000
#define DEFAULT_THRES_BOT 12000
#elif defined(CONFIG_BADGE23_HW_GEN_P1)
static const uint8_t top_map[] = {2, 2, 2, 0, 0, 8, 8, 8, 6, 6, 4, 4};
static const uint8_t top_stages = 12;
static const uint8_t bot_map[] = {1, 1, 3, 3, 5, 5, 7, 7, 9, 9};
static const uint8_t bot_stages = 10;
static const uint8_t top_segment_map[] = {1,2,0,1,2,1,2,0,1,2,1,2}; //idk
static const uint8_t bot_segment_map[] = {3,0,3,0,3,0,0,3,0,3}; //idk
static const uint8_t bot_stage_config[] = {0,1,2,3,4,5,6,7,8,9,10,11};
#define DEFAULT_THRES_TOP 2000
#define DEFAULT_THRES_BOT 12000
#define AD7147_ADDR_TOP            0b101101
#define AD7147_ADDR_BOT            0b101100
#else
#error "captouch not implemented for this badge generation"
#endif

#if defined(CONFIG_BADGE23_HW_GEN_P4)
#define AD7147_ADDR_TOP            0b101100
#define AD7147_ADDR_BOT            0b101101
#elif defined(CONFIG_BADGE23_HW_GEN_P3)
#define AD7147_ADDR_TOP            0b101101
#define AD7147_ADDR_BOT            0b101100
#endif
 
static const char *TAG = "captouch";

#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */

#define AD7147_REG_PWR_CONTROL              0x00
#define AD7147_REG_STAGE_CAL_EN             0x01
#define AD7147_REG_STAGE_HIGH_INT_ENABLE    0x06
#define AD7147_REG_DEVICE_ID                0x17

#define TIMEOUT_MS                  1000

#define PETAL_PRESSED_DEBOUNCE 2

static struct ad714x_chip *chip_top;
static struct ad714x_chip *chip_bot;

typedef struct{
    uint8_t config_mask;
    uint16_t amb_values[4]; //ordered according to PETAL_SEGMENT_*
    uint16_t cdc_values[4]; //ordered according to PETAL_SEGMENT_*
    uint16_t thres_values[4]; //ordered according to PETAL_SEGMENT_*
    uint8_t pressed;
} petal_t;

static petal_t petals[10];

struct ad714x_chip {
    uint8_t addr;
    uint8_t gpio;
    int pos_afe_offsets[13];
    int neg_afe_offsets[13];
    int neg_afe_offset_swap;
    int stages;
};  

static struct ad714x_chip chip_top_rev5 = {.addr = AD7147_ADDR_TOP, .gpio = 15,
    .pos_afe_offsets = {4, 2, 2, 2, 2, 3, 4, 2, 2, 2, 2, 0},
    .stages=top_stages};
static struct ad714x_chip chip_bot_rev5 = {.addr = AD7147_ADDR_BOT, .gpio = 15,
    .pos_afe_offsets = {3, 2, 1, 1 ,1, 1, 1, 1, 2, 3, 3, 3},
    .stages=bot_stages};

static esp_err_t ad714x_i2c_write(const struct ad714x_chip *chip, const uint16_t reg, const uint16_t data)
{
    const uint8_t tx[] = {reg >> 8, reg & 0xFF, data >> 8, data & 0xFF};
    ESP_LOGI(TAG, "AD7147 write reg %X-> %X", reg, data);
    return i2c_master_write_to_device(I2C_MASTER_NUM, chip->addr, tx, sizeof(tx), TIMEOUT_MS / portTICK_PERIOD_MS);
}

static esp_err_t ad714x_i2c_read(const struct ad714x_chip *chip, const uint16_t reg, uint16_t *data, const size_t len)
{
    const uint8_t tx[] = {reg >> 8, reg & 0xFF};
    uint8_t rx[len * 2];
    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_NUM, chip->addr, tx, sizeof(tx), rx, sizeof(rx), TIMEOUT_MS / portTICK_PERIOD_MS);
    for(int i = 0; i < len; i++) {
        data[i] = (rx[i * 2] << 8) | rx[i * 2 + 1];
    }
    return ret;
}

struct ad7147_stage_config {
    unsigned int cinX_connection_setup[13];
    unsigned int se_connection_setup:2;
    unsigned int neg_afe_offset_disable:1;
    unsigned int pos_afe_offset_disable:1;
    unsigned int neg_afe_offset:6;
    unsigned int neg_afe_offset_swap:1;
    unsigned int pos_afe_offset:6;
    unsigned int pos_afe_offset_swap:1;
    unsigned int neg_threshold_sensitivity:4;
    unsigned int neg_peak_detect:3;
    unsigned int pos_threshold_sensitivity:4;
    unsigned int pos_peak_detect:3;
};

#define CIN CDC_NONE    0
#define CIN_CDC_NEG     1
#define CIN_CDC_POS     2
#define CIN_BIAS        3

static const uint16_t bank2 = 0x80;

static void ad714x_set_stage_config(const struct ad714x_chip *chip, const uint8_t stage, const struct ad7147_stage_config * config)
{
    const uint16_t connection_6_0 = (config->cinX_connection_setup[6] << 12) | (config->cinX_connection_setup[5] << 10) | (config->cinX_connection_setup[4] << 8) | (config->cinX_connection_setup[3] << 6) | (config->cinX_connection_setup[2] << 4) | (config->cinX_connection_setup[1] << 2) | (config->cinX_connection_setup[0] << 0);
    const uint16_t connection_12_7 = (config->pos_afe_offset_disable << 15) | (config->neg_afe_offset_disable << 14) | (config->se_connection_setup << 12) | (config->cinX_connection_setup[12] << 10) | (config->cinX_connection_setup[11] << 8) | (config->cinX_connection_setup[10] << 6) | (config->cinX_connection_setup[9] << 4) | (config->cinX_connection_setup[8] << 2) | (config->cinX_connection_setup[7] << 0);
    const uint16_t afe_offset = (config->pos_afe_offset_swap << 15) | (config->pos_afe_offset << 8) | (config->neg_afe_offset_swap << 7) | (config->neg_afe_offset << 0);
    const uint16_t sensitivity = (config->pos_peak_detect << 12) | (config->pos_threshold_sensitivity << 8) | (config->neg_peak_detect << 4) | (config->neg_threshold_sensitivity << 0);

    //ESP_LOGI(TAG, "Stage %d config-> %X %X %X %X", stage, connection_6_0, connection_12_7, afe_offset, sensitivity);
    //ESP_LOGI(TAG, "Config: %X %X %X %X %X %X %X %X %X", config->pos_afe_offset_disable, config->pos_afe_offset_disable, config->se_connection_setup, config->cinX_connection_setup[12], config->cinX_connection_setup[11], config->cinX_connection_setup[10], config->cinX_connection_setup[9], config->cinX_connection_setup[8], config->cinX_connection_setup[7]);

    ad714x_i2c_write(chip, bank2 + stage * 8, connection_6_0);
    ad714x_i2c_write(chip, bank2 + stage * 8 + 1, connection_12_7);
    ad714x_i2c_write(chip, bank2 + stage * 8 + 2, afe_offset);
    ad714x_i2c_write(chip, bank2 + stage * 8 + 3, sensitivity);
}

struct ad7147_device_config {
    unsigned int power_mode:2;
    unsigned int lp_conv_delay:2;
    unsigned int sequence_stage_num:4;
    unsigned int decimation:2;
    unsigned int sw_reset:1;
    unsigned int int_pol:1;
    unsigned int ext_source:1;
    unsigned int cdc_bias:2;

    unsigned int stage0_cal_en:1;
    unsigned int stage1_cal_en:1;
    unsigned int stage2_cal_en:1;
    unsigned int stage3_cal_en:1;
    unsigned int stage4_cal_en:1;
    unsigned int stage5_cal_en:1;
    unsigned int stage6_cal_en:1;
    unsigned int stage7_cal_en:1;
    unsigned int stage8_cal_en:1;
    unsigned int stage9_cal_en:1;
    unsigned int stage10_cal_en:1;
    unsigned int stage11_cal_en:1;
    unsigned int avg_fp_skip:2;
    unsigned int avg_lp_skip:2;

    unsigned int stage0_high_int_enable:1;
    unsigned int stage1_high_int_enable:1;
    unsigned int stage2_high_int_enable:1;
    unsigned int stage3_high_int_enable:1;
    unsigned int stage4_high_int_enable:1;
    unsigned int stage5_high_int_enable:1;
    unsigned int stage6_high_int_enable:1;
    unsigned int stage7_high_int_enable:1;
    unsigned int stage8_high_int_enable:1;
    unsigned int stage9_high_int_enable:1;
    unsigned int stage10_high_int_enable:1;
    unsigned int stage11_high_int_enable:1;
};


static void ad714x_set_device_config(const struct ad714x_chip *chip, const struct ad7147_device_config * config)
{
    const uint16_t pwr_control = (config->cdc_bias << 14) | (config->ext_source << 12) | (config->int_pol << 11) | (config->sw_reset << 10) | (config->decimation << 8) | (config->sequence_stage_num << 4) | (config->lp_conv_delay << 2) | (config->power_mode << 0);
    const uint16_t stage_cal_en = (config->avg_lp_skip << 14) | (config->avg_fp_skip << 12) | (config->stage11_cal_en << 11) | (config->stage10_cal_en << 10) | (config->stage9_cal_en << 9) | (config->stage8_cal_en << 8) | (config->stage7_cal_en << 7) | (config->stage6_cal_en << 6) | (config->stage5_cal_en << 5) | (config->stage4_cal_en << 4) | (config->stage3_cal_en << 3) | (config->stage2_cal_en << 2) | (config->stage1_cal_en << 1) | (config->stage0_cal_en << 0);
    const uint16_t stage_high_int_enable = (config->stage11_high_int_enable << 11) | (config->stage10_high_int_enable << 10) | (config->stage9_high_int_enable << 9) | (config->stage8_high_int_enable << 8) | (config->stage7_high_int_enable << 7) | (config->stage6_high_int_enable << 6) | (config->stage5_high_int_enable << 5) | (config->stage4_high_int_enable << 4) | (config->stage3_high_int_enable << 3) | (config->stage2_high_int_enable << 2) | (config->stage1_high_int_enable << 1) | (config->stage0_high_int_enable << 0);

    ad714x_i2c_write(chip, AD7147_REG_PWR_CONTROL, pwr_control);
    ad714x_i2c_write(chip, AD7147_REG_STAGE_CAL_EN, stage_cal_en);
    ad714x_i2c_write(chip, AD7147_REG_STAGE_HIGH_INT_ENABLE, stage_high_int_enable);
}

static struct ad7147_stage_config ad714x_default_config(void)
{
    return (struct ad7147_stage_config) {
            .cinX_connection_setup={CIN_BIAS, CIN_BIAS, CIN_BIAS, CIN_BIAS, CIN_BIAS, CIN_BIAS, CIN_BIAS, CIN_BIAS, CIN_BIAS, CIN_BIAS, CIN_BIAS, CIN_BIAS},
            .se_connection_setup=0b01,
            .pos_afe_offset=0,
        };
}

static void captouch_init_chip(const struct ad714x_chip* chip, const struct ad7147_device_config device_config)
{
    uint16_t data;
    ad714x_i2c_read(chip, AD7147_REG_DEVICE_ID, &data, 1);
    ESP_LOGI(TAG, "DEVICE ID = %X", data);

    ad714x_set_device_config(chip, &device_config);

    for(int i=0; i<chip->stages; i++) {
        struct ad7147_stage_config stage_config;
        stage_config = ad714x_default_config();
        if(chip == chip_bot){
            stage_config.cinX_connection_setup[bot_stage_config[i]] = CIN_CDC_POS;
        } else {
            stage_config.cinX_connection_setup[i] = CIN_CDC_POS;
        }
        stage_config.pos_afe_offset=chip->pos_afe_offsets[i];
        ad714x_set_stage_config(chip, i, &stage_config);
    }
}

static void captouch_init_petals(){
    for(int i = 0; i < 10; i++){
        for(int j = 0; j < 4; j++){
            petals[i].amb_values[j] = 0;
            petals[i].cdc_values[j] = 0;
            if(i%2){
                petals[i].thres_values[j] = DEFAULT_THRES_BOT;
            } else {
                petals[i].thres_values[j] = DEFAULT_THRES_TOP;
            }
        }
        petals[i].config_mask = 0;
    }
    for(int i = 0; i < bot_stages; i++){
        petals[bot_map[i]].config_mask |= 1 << bot_segment_map[i]; 
    }
    for(int i = 0; i < top_stages; i++){
        petals[top_map[i]].config_mask |= 1 << top_segment_map[i]; 
    }
}

int32_t captouch_get_petal_rad(uint8_t petal){
    uint8_t cf = petals[petal].config_mask;
    if(cf == 0b1110){ //LEFT, RIGHT, BASE
        int32_t left = petals[petal].cdc_values[PETAL_SEGMENT_LEFT];
        left -= petals[petal].amb_values[PETAL_SEGMENT_LEFT];
        int32_t right = petals[petal].cdc_values[PETAL_SEGMENT_RIGHT];
        right -= petals[petal].amb_values[PETAL_SEGMENT_RIGHT];
        int32_t base = petals[petal].cdc_values[PETAL_SEGMENT_BASE];
        base -= petals[petal].amb_values[PETAL_SEGMENT_BASE];
        return (left + right)/2 - base;
    }
    if(cf == 0b111){ //LEFT, RIGHT, TIP
        int32_t left = petals[petal].cdc_values[PETAL_SEGMENT_LEFT];
        left -= petals[petal].amb_values[PETAL_SEGMENT_LEFT];
        int32_t right = petals[petal].cdc_values[PETAL_SEGMENT_RIGHT];
        right -= petals[petal].amb_values[PETAL_SEGMENT_RIGHT];
        int32_t tip = petals[petal].cdc_values[PETAL_SEGMENT_TIP];
        tip -= petals[petal].amb_values[PETAL_SEGMENT_TIP];
        return (-left - right)/2 + tip;
    }
    if(cf == 0b1001){ //TIP, BASE
        int32_t tip = petals[petal].cdc_values[PETAL_SEGMENT_TIP];
        tip -= petals[petal].amb_values[PETAL_SEGMENT_TIP];
        int32_t base = petals[petal].cdc_values[PETAL_SEGMENT_BASE];
        base -= petals[petal].amb_values[PETAL_SEGMENT_BASE];
        return tip - base;
    }
    if(cf == 0b1){ //TIP
        int32_t tip = petals[petal].cdc_values[PETAL_SEGMENT_TIP];
        tip -= petals[petal].amb_values[PETAL_SEGMENT_TIP];
        return tip;
    }
    return 0;
}

int32_t captouch_get_petal_phi(uint8_t petal){
    uint8_t cf = petals[petal].config_mask;
    if((cf == 0b1110) || (cf == 0b110) || (cf == 0b111)){ //LEFT, RIGHT, (BASE)
        int32_t left = petals[petal].cdc_values[PETAL_SEGMENT_LEFT];
        left -= petals[petal].amb_values[PETAL_SEGMENT_LEFT];
        int32_t right = petals[petal].cdc_values[PETAL_SEGMENT_RIGHT];
        right -= petals[petal].amb_values[PETAL_SEGMENT_RIGHT];
        return left - right;
    }
    return 0;
}

void captouch_init(void)
{
    captouch_init_petals();
    chip_top = &chip_top_rev5;
    chip_bot = &chip_bot_rev5;

    captouch_init_chip(chip_top, (struct ad7147_device_config){.sequence_stage_num = 11,
                                                 .decimation = 1,
                                                 });

    captouch_init_chip(chip_bot, (struct ad7147_device_config){.sequence_stage_num = 11,
                                                 .decimation = 1,
                                                 });
}

static void print_cdc(uint16_t *data)
{
    printf("CDC results: %X %X %X %X %X %X %X %X %X %X %X %X", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11]);
}

static void print_ambient(uint16_t *data)
{
    printf("AMB results: %X %X %X %X %X %X %X %X %X %X %X %X", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11]);
}

uint16_t read_captouch(){
    uint16_t bin_petals = 0;
    for(int i = 0; i < 10; i++) {
        if(petals[i].pressed){
            bin_petals |= (1<<i);
        }
    }
    return bin_petals;
}

uint16_t cdc_data[2][12] = {0,};
uint16_t cdc_ambient[2][12] = {0,};

static uint8_t calib_cycles = 0;
void captouch_force_calibration(){
    if(!calib_cycles){ //last calib has finished
        calib_cycles = 16; //goal cycles, can be argument someday
    }
}

uint8_t captouch_calibration_active(){
    return calib_cycles ? 1 : 0;
}


void check_petals_pressed(){
    for(int i = 0; i < 10; i++){
        bool pressed = 0;
        bool prev = petals[i].pressed;
        for(int j = 0; j < 4; j++){
            if((petals[i].amb_values[j] +
                petals[i].thres_values[j]) <
                petals[i].cdc_values[j]){
                pressed = 1;
            }
        }
        if(pressed){
            petals[i].pressed = PETAL_PRESSED_DEBOUNCE;
        } else if(petals[i].pressed){
            petals[i].pressed--;
        }

        if(petals[i].pressed && (!prev)){
            // TODO: PETAL_PRESS_CALLBACK
        }
        if((!petals[i].pressed) && prev){
            // TODO: PETAL_RELEASE_CALLBACK
        }
    }
}

void cdc_to_petal(bool bot, bool amb, uint16_t cdc_data[], uint8_t cdc_data_length){
    if(!bot){
        for(int i = 0; i < cdc_data_length; i++){
            if(amb){
                petals[top_map[i]].amb_values[top_segment_map[i]] = cdc_data[i];
            } else {
                petals[top_map[i]].cdc_values[top_segment_map[i]] = cdc_data[i];
            }
        }
    } else {
        for(int i = 0; i < cdc_data_length; i++){
            if(amb){
                petals[bot_map[i]].amb_values[bot_segment_map[i]] = cdc_data[i];
            } else {
                petals[bot_map[i]].cdc_values[bot_segment_map[i]] = cdc_data[i];
            }
        }
    }
}

uint16_t captouch_get_petal_pad_raw(uint8_t petal, uint8_t pad, uint8_t amb){
    if(petal > 9) petal = 9;
    if(pad > 3) pad = 3;
    if(amb){
        return petals[petal].amb_values[pad];
    } else {
        return petals[petal].cdc_values[pad];
    }
}

void captouch_set_petal_pad_threshold(uint8_t petal, uint8_t pad, uint16_t thres){
    if(petal > 9) petal = 9;
    if(pad > 3) pad = 3;
    petals[petal].thres_values[pad] = thres;    
}

void captouch_read_cycle(){
        static uint8_t calib_cycle = 0; 
        static uint8_t calib_div = 1;
        static uint32_t ambient_acc[2][12] = {{0,}, {0,}};
        if(calib_cycles){
            if(calib_cycle == 0){ // last cycle has finished
                calib_cycle = calib_cycles;
                calib_div = calib_cycles;
                for(int j=0;j<12;j++){
                    ambient_acc[0][j] = 0;
                    ambient_acc[1][j] = 0;
                }
            }

            ad714x_i2c_read(chip_top, 0xB, cdc_ambient[0], chip_top->stages);
            ad714x_i2c_read(chip_bot, 0xB, cdc_ambient[1], chip_bot->stages);
            for(int j=0;j<12;j++){
                ambient_acc[0][j] += cdc_ambient[0][j];
                ambient_acc[1][j] += cdc_ambient[1][j];
            }

            // TODO: use median instead of average
            calib_cycle--;
            if(!calib_cycle){ //calib cycle is complete
                for(int i=0;i<12;i++){
                    cdc_ambient[0][i] = ambient_acc[0][i] / calib_div;
                    cdc_ambient[1][i] = ambient_acc[1][i] / calib_div;
                }
                cdc_to_petal(0, 1, cdc_ambient[0], 12);
                cdc_to_petal(1, 1, cdc_ambient[1], 12);
                calib_cycles = 0;
            }
        } else {
            ad714x_i2c_read(chip_top, 0xB, cdc_data[0], chip_top->stages);
            cdc_to_petal(0, 0, cdc_data[0], 12);

            ad714x_i2c_read(chip_bot, 0xB, cdc_data[1], chip_bot->stages);
            cdc_to_petal(1, 0, cdc_data[1], 12);

            check_petals_pressed();
        }
}

static void captouch_task(void* arg){}

static void captouch_print_debug_info_chip(const struct ad714x_chip* chip)
{
    uint16_t *data;
    uint16_t *ambient;
    const int stages = chip->stages;

    if(chip == chip_top) {
        data = cdc_data[0];
        ambient = cdc_ambient[0];
    } else {
        data = cdc_data[1];
        ambient = cdc_ambient[1];
    }

#if 1
    ESP_LOGI(TAG, "CDC results: %X %X %X %X %X %X %X %X %X %X %X %X", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11]);

    for(int stage=0; stage<stages; stage++) {
        ESP_LOGI(TAG, "stage %d ambient: %X diff: %d", stage, ambient[stage], data[stage] - ambient[stage]);
    }
#endif
#if 0
    ad714x_i2c_read(chip, 8, data, 1);
    ESP_LOGI(TAG, "Low interrupt %X", data[0]);
    ad714x_i2c_read(chip, 9, data, 1);
    ESP_LOGI(TAG, "High interrupt %X", data[0]);
    ad714x_i2c_read(chip, 0x42, data, 1);
    ESP_LOGI(TAG, "Proximity %X", data[0]);
    //ESP_LOGI(TAG, "CDC result = %X", data[0]);
    //if(data[0] > 0xa000) {
        //ESP_LOGI(TAG, "Touch! %X", data[0]);
    //}
#endif
}

void captouch_print_debug_info(void)
{
    captouch_print_debug_info_chip(chip_top);
    captouch_print_debug_info_chip(chip_bot);
}

void captouch_get_cross(int paddle, int *x, int *y)
{
    uint16_t *data;
    uint16_t *ambient;

    int result[2] = {0, 0};
    float total = 0;

    const int paddle_info_1[] = {
        4,
        0,
        1,
        2,
        11,
        4,
        9,
        7,
        6,
        9,
    };
    const int paddle_info_2[] = {
        3,
        1,
        0,
        3,
        10,
        5,
        8,
        6,
        5,
        8,
    };

    struct ad714x_chip* chip;
    if (paddle % 2 == 0) {
        //chip = chip_top;
        data = cdc_data[0];
        ambient = cdc_ambient[0];
    } else {
        //chip = chip_bot;
        data = cdc_data[1];
        ambient = cdc_ambient[1];
    }
    //ESP_LOGI(TAG, "CDC results: %X %X %X %X %X %X %X %X %X %X %X %X", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11]);

    int diff1 = data[paddle_info_1[paddle]] - ambient[paddle_info_1[paddle]];
    int diff2 = data[paddle_info_2[paddle]] - ambient[paddle_info_2[paddle]];

    ESP_LOGI(TAG, "%10d %10d", diff1, diff2);

    int vectors[][2] = {{240, 240}, {240, 0}, {0, 120}};
    total = ((diff1) + (diff2));

    result[0] = vectors[0][0] * (diff1) + vectors[1][0] * (diff2);
    result[1] = vectors[0][1] * (diff1) + vectors[1][1] * (diff2);

    *x = result[0] / total;
    *y = result[1] / total;
}
