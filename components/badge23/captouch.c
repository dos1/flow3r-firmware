#include "include/badge23/captouch.h"
#include <freertos/FreeRTOS.h>
#include <freertos/atomic.h>
#include <stdint.h>
#include "esp_log.h"
#include "flow3r_bsp_i2c.h"

#define PETAL_PAD_TIP 0
#define PETAL_PAD_CCW 1
#define PETAL_PAD_CW 2
#define PETAL_PAD_BASE 3

#define CIN CDC_NONE 0
#define CIN_CDC_NEG 1
#define CIN_CDC_POS 2
#define CIN_BIAS 3

#define AFE_INCR_CAP 1000

static const uint8_t top_map[] = {0, 0, 0, 2, 2, 2, 6, 6, 6, 4, 4, 4};
static const uint8_t top_stages = 12;
static const uint8_t bot_map[] = {1, 1, 3, 3, 5, 7, 7, 9, 9, 8, 8, 8};
static const uint8_t bot_stages = 12;
static const uint8_t bot_stage_config[] = {0, 1, 2, 3,  5,  6,
                                           7, 8, 9, 10, 11, 12};
#define DEFAULT_THRES_TOP 8000
#define DEFAULT_THRES_BOT 12000

#if defined(CONFIG_FLOW3R_HW_GEN_P4)
static const uint8_t top_segment_map[] = {1, 3, 2, 2, 3, 1,
                                          1, 3, 2, 1, 3, 2};  // PETAL_PAD_*
static const uint8_t bot_segment_map[] = {3, 0, 3, 0, 0, 0,
                                          3, 0, 3, 1, 2, 3};  // PETAL_PAD_*
#elif defined(CONFIG_FLOW3R_HW_GEN_P6)
static const uint8_t top_segment_map[] = {1, 3, 2, 2, 3, 1,
                                          1, 3, 2, 1, 3, 2};  // PETAL_PAD_*
static const uint8_t bot_segment_map[] = {3, 0, 3, 0, 0, 0,
                                          3, 0, 3, 1, 2, 3};  // PETAL_PAD_*
#elif defined(CONFIG_FLOW3R_HW_GEN_P3)
static const uint8_t top_segment_map[] = {0, 1, 2, 2, 1, 0,
                                          0, 1, 2, 2, 1, 0};  // PETAL_PAD_*
static const uint8_t bot_segment_map[] = {3, 0, 3, 0, 0, 0,
                                          3, 0, 3, 0, 2, 1};  // PETAL_PAD_*
#endif

static const char *TAG = "captouch";

#define AD7147_REG_PWR_CONTROL 0x00
#define AD7147_REG_STAGE_CAL_EN 0x01
#define AD7147_REG_STAGE_HIGH_INT_ENABLE 0x06
#define AD7147_REG_DEVICE_ID 0x17

#define TIMEOUT_MS 1000

#define PETAL_PRESSED_DEBOUNCE 2

static struct ad714x_chip *chip_top;
static struct ad714x_chip *chip_bot;

typedef struct {
    uint16_t amb;
    uint16_t cdc;
    uint16_t thres;
    uint8_t pressed;
} petal_pad_t;

typedef struct {
    uint8_t config_mask;
    petal_pad_t pads[4];  // ordered according to PETAL_PAD_*
    uint8_t pressed;
} petal_t;

static petal_t petals[10];

struct ad714x_chip {
    const flow3r_i2c_address *addr;
    uint8_t gpio;
    int pos_afe_offsets[13];
    int neg_afe_offsets[13];
    int neg_afe_offset_swap;
    int stages;
};

static struct ad714x_chip chip_top_rev5 = {
    .addr = &flow3r_i2c_addresses.touch_top,
    .gpio = 15,
    .pos_afe_offsets = {4, 2, 2, 2, 2, 3, 4, 2, 2, 2, 2, 0},
    .stages = top_stages,
};

static struct ad714x_chip chip_bot_rev5 = {
    .addr = &flow3r_i2c_addresses.touch_bottom,
    .gpio = 15,
    .pos_afe_offsets = {3, 2, 1, 1, 1, 1, 1, 1, 2, 3, 3, 3},
    .stages = bot_stages,
};

static esp_err_t ad714x_i2c_write(const struct ad714x_chip *chip,
                                  const uint16_t reg, const uint16_t data) {
    const uint8_t tx[] = {reg >> 8, reg & 0xFF, data >> 8, data & 0xFF};
    ESP_LOGD(TAG, "AD7147 write reg %X-> %X", reg, data);
    return flow3r_bsp_i2c_write_to_device(*chip->addr, tx, sizeof(tx),
                                          TIMEOUT_MS / portTICK_PERIOD_MS);
}

static esp_err_t ad714x_i2c_read(const struct ad714x_chip *chip,
                                 const uint16_t reg, uint16_t *data,
                                 const size_t len) {
    const uint8_t tx[] = {reg >> 8, reg & 0xFF};
    uint8_t rx[len * 2];
    esp_err_t ret = flow3r_bsp_i2c_write_read_device(
        *chip->addr, tx, sizeof(tx), rx, sizeof(rx),
        TIMEOUT_MS / portTICK_PERIOD_MS);
    for (int i = 0; i < len; i++) {
        data[i] = (rx[i * 2] << 8) | rx[i * 2 + 1];
    }
    return ret;
}

struct ad7147_stage_config {
    unsigned int cinX_connection_setup[13];
    unsigned int se_connection_setup : 2;
    unsigned int neg_afe_offset_disable : 1;
    unsigned int pos_afe_offset_disable : 1;
    unsigned int neg_afe_offset : 6;
    unsigned int neg_afe_offset_swap : 1;
    unsigned int pos_afe_offset : 6;
    unsigned int pos_afe_offset_swap : 1;
    unsigned int neg_threshold_sensitivity : 4;
    unsigned int neg_peak_detect : 3;
    unsigned int pos_threshold_sensitivity : 4;
    unsigned int pos_peak_detect : 3;
};

static const uint16_t bank2 = 0x80;

static void ad714x_set_stage_config(const struct ad714x_chip *chip,
                                    const uint8_t stage,
                                    const struct ad7147_stage_config *config) {
    const uint16_t connection_6_0 = (config->cinX_connection_setup[6] << 12) |
                                    (config->cinX_connection_setup[5] << 10) |
                                    (config->cinX_connection_setup[4] << 8) |
                                    (config->cinX_connection_setup[3] << 6) |
                                    (config->cinX_connection_setup[2] << 4) |
                                    (config->cinX_connection_setup[1] << 2) |
                                    (config->cinX_connection_setup[0] << 0);
    const uint16_t connection_12_7 = (config->pos_afe_offset_disable << 15) |
                                     (config->neg_afe_offset_disable << 14) |
                                     (config->se_connection_setup << 12) |
                                     (config->cinX_connection_setup[12] << 10) |
                                     (config->cinX_connection_setup[11] << 8) |
                                     (config->cinX_connection_setup[10] << 6) |
                                     (config->cinX_connection_setup[9] << 4) |
                                     (config->cinX_connection_setup[8] << 2) |
                                     (config->cinX_connection_setup[7] << 0);
    const uint16_t afe_offset =
        (config->pos_afe_offset_swap << 15) | (config->pos_afe_offset << 8) |
        (config->neg_afe_offset_swap << 7) | (config->neg_afe_offset << 0);
    const uint16_t sensitivity = (config->pos_peak_detect << 12) |
                                 (config->pos_threshold_sensitivity << 8) |
                                 (config->neg_peak_detect << 4) |
                                 (config->neg_threshold_sensitivity << 0);

    // ESP_LOGI(TAG, "Stage %d config-> %X %X %X %X", stage, connection_6_0,
    // connection_12_7, afe_offset, sensitivity); ESP_LOGI(TAG, "Config: %X %X
    // %X %X %X %X %X %X %X", config->pos_afe_offset_disable,
    // config->pos_afe_offset_disable, config->se_connection_setup,
    // config->cinX_connection_setup[12], config->cinX_connection_setup[11],
    // config->cinX_connection_setup[10], config->cinX_connection_setup[9],
    // config->cinX_connection_setup[8], config->cinX_connection_setup[7]);

    ad714x_i2c_write(chip, bank2 + stage * 8, connection_6_0);
    ad714x_i2c_write(chip, bank2 + stage * 8 + 1, connection_12_7);
    ad714x_i2c_write(chip, bank2 + stage * 8 + 2, afe_offset);
    ad714x_i2c_write(chip, bank2 + stage * 8 + 3, sensitivity);
}

struct ad7147_device_config {
    unsigned int power_mode : 2;
    unsigned int lp_conv_delay : 2;
    unsigned int sequence_stage_num : 4;
    unsigned int decimation : 2;
    unsigned int sw_reset : 1;
    unsigned int int_pol : 1;
    unsigned int ext_source : 1;
    unsigned int cdc_bias : 2;

    unsigned int stage0_cal_en : 1;
    unsigned int stage1_cal_en : 1;
    unsigned int stage2_cal_en : 1;
    unsigned int stage3_cal_en : 1;
    unsigned int stage4_cal_en : 1;
    unsigned int stage5_cal_en : 1;
    unsigned int stage6_cal_en : 1;
    unsigned int stage7_cal_en : 1;
    unsigned int stage8_cal_en : 1;
    unsigned int stage9_cal_en : 1;
    unsigned int stage10_cal_en : 1;
    unsigned int stage11_cal_en : 1;
    unsigned int avg_fp_skip : 2;
    unsigned int avg_lp_skip : 2;

    unsigned int stage0_high_int_enable : 1;
    unsigned int stage1_high_int_enable : 1;
    unsigned int stage2_high_int_enable : 1;
    unsigned int stage3_high_int_enable : 1;
    unsigned int stage4_high_int_enable : 1;
    unsigned int stage5_high_int_enable : 1;
    unsigned int stage6_high_int_enable : 1;
    unsigned int stage7_high_int_enable : 1;
    unsigned int stage8_high_int_enable : 1;
    unsigned int stage9_high_int_enable : 1;
    unsigned int stage10_high_int_enable : 1;
    unsigned int stage11_high_int_enable : 1;
};

static void ad714x_set_device_config(
    const struct ad714x_chip *chip, const struct ad7147_device_config *config) {
    const uint16_t pwr_control =
        (config->cdc_bias << 14) | (config->ext_source << 12) |
        (config->int_pol << 11) | (config->sw_reset << 10) |
        (config->decimation << 8) | (config->sequence_stage_num << 4) |
        (config->lp_conv_delay << 2) | (config->power_mode << 0);
    const uint16_t stage_cal_en =
        (config->avg_lp_skip << 14) | (config->avg_fp_skip << 12) |
        (config->stage11_cal_en << 11) | (config->stage10_cal_en << 10) |
        (config->stage9_cal_en << 9) | (config->stage8_cal_en << 8) |
        (config->stage7_cal_en << 7) | (config->stage6_cal_en << 6) |
        (config->stage5_cal_en << 5) | (config->stage4_cal_en << 4) |
        (config->stage3_cal_en << 3) | (config->stage2_cal_en << 2) |
        (config->stage1_cal_en << 1) | (config->stage0_cal_en << 0);
    const uint16_t stage_high_int_enable =
        (config->stage11_high_int_enable << 11) |
        (config->stage10_high_int_enable << 10) |
        (config->stage9_high_int_enable << 9) |
        (config->stage8_high_int_enable << 8) |
        (config->stage7_high_int_enable << 7) |
        (config->stage6_high_int_enable << 6) |
        (config->stage5_high_int_enable << 5) |
        (config->stage4_high_int_enable << 4) |
        (config->stage3_high_int_enable << 3) |
        (config->stage2_high_int_enable << 2) |
        (config->stage1_high_int_enable << 1) |
        (config->stage0_high_int_enable << 0);

    ad714x_i2c_write(chip, AD7147_REG_PWR_CONTROL, pwr_control);
    ad714x_i2c_write(chip, AD7147_REG_STAGE_CAL_EN, stage_cal_en);
    ad714x_i2c_write(chip, AD7147_REG_STAGE_HIGH_INT_ENABLE,
                     stage_high_int_enable);
}

static struct ad7147_stage_config ad714x_default_config(void) {
    return (struct ad7147_stage_config){
        .cinX_connection_setup = {CIN_BIAS, CIN_BIAS, CIN_BIAS, CIN_BIAS,
                                  CIN_BIAS, CIN_BIAS, CIN_BIAS, CIN_BIAS,
                                  CIN_BIAS, CIN_BIAS, CIN_BIAS, CIN_BIAS},
        .se_connection_setup = 0b01,
        .pos_afe_offset = 0,
    };
}

static void captouch_configure_stage(struct ad714x_chip *chip, uint8_t stage) {
    struct ad7147_stage_config stage_config;
    stage_config = ad714x_default_config();
    if (chip == chip_bot) {
        stage_config.cinX_connection_setup[bot_stage_config[stage]] =
            CIN_CDC_POS;
    } else {
        stage_config.cinX_connection_setup[stage] = CIN_CDC_POS;
    }
    stage_config.pos_afe_offset = chip->pos_afe_offsets[stage];
    ad714x_set_stage_config(chip, stage, &stage_config);
}

static int8_t captouch_configure_stage_afe_offset(uint8_t top, uint8_t stage,
                                                  int8_t delta_afe) {
    int8_t sat = 0;
    struct ad714x_chip *chip = chip_bot;
    if (top) chip = chip_top;
    int8_t afe = chip->pos_afe_offsets[stage] - chip->neg_afe_offsets[stage];
    if ((afe >= 63) && (delta_afe > 0)) sat = 1;
    if ((afe <= 63) && (delta_afe < 0)) sat = -1;
    afe += delta_afe;
    if (afe >= 63) afe = 63;
    if (afe <= -63) afe = -63;

    if (afe > 0) {
        chip->pos_afe_offsets[stage] = afe;
        chip->neg_afe_offsets[stage] = 0;
    } else {
        chip->pos_afe_offsets[stage] = 0;
        chip->neg_afe_offsets[stage] = -afe;
    }
    captouch_configure_stage(chip, stage);
    return sat;
}

static void captouch_init_chip(
    struct ad714x_chip *chip, const struct ad7147_device_config device_config) {
    uint16_t data;
    ad714x_i2c_read(chip, AD7147_REG_DEVICE_ID, &data, 1);
    ESP_LOGI(TAG, "DEVICE ID = %X", data);

    ad714x_set_device_config(chip, &device_config);

    for (int i = 0; i < chip->stages; i++) {
        captouch_configure_stage(chip, i);
    }
}

static void captouch_init_petals() {
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 4; j++) {
            petals[i].pads[j].amb = 0;
            petals[i].pads[j].cdc = 0;
            if (i % 2) {
                petals[i].pads[j].thres = DEFAULT_THRES_BOT;
            } else {
                petals[i].pads[j].thres = DEFAULT_THRES_TOP;
            }
        }
        petals[i].config_mask = 0;
    }
    for (int i = 0; i < bot_stages; i++) {
        petals[bot_map[i]].config_mask |= 1 << bot_segment_map[i];
    }
    for (int i = 0; i < top_stages; i++) {
        petals[top_map[i]].config_mask |= 1 << top_segment_map[i];
    }
}

int32_t captouch_get_petal_rad(uint8_t petal) {
    if (petal > 9) petal = 9;
    uint8_t cf = petals[petal].config_mask;
    if (cf == 0b1110) {  // CCW, CW, BASE
        int32_t left = petals[petal].pads[PETAL_PAD_CCW].cdc;
        left -= petals[petal].pads[PETAL_PAD_CCW].amb;
        int32_t right = petals[petal].pads[PETAL_PAD_CW].cdc;
        right -= petals[petal].pads[PETAL_PAD_CW].amb;
        int32_t base = petals[petal].pads[PETAL_PAD_BASE].cdc;
        base -= petals[petal].pads[PETAL_PAD_BASE].amb;
        return (left + right) / 2 - base;
    }
    if (cf == 0b111) {  // CCW, CW, TIP
        int32_t left = petals[petal].pads[PETAL_PAD_CCW].cdc;
        left -= petals[petal].pads[PETAL_PAD_CCW].amb;
        int32_t right = petals[petal].pads[PETAL_PAD_CW].cdc;
        right -= petals[petal].pads[PETAL_PAD_CW].amb;
        int32_t tip = petals[petal].pads[PETAL_PAD_TIP].cdc;
        tip -= petals[petal].pads[PETAL_PAD_TIP].amb;
        return (-left - right) / 2 + tip;
    }
    if (cf == 0b1001) {  // TIP, BASE
        int32_t tip = petals[petal].pads[PETAL_PAD_TIP].cdc;
        tip -= petals[petal].pads[PETAL_PAD_TIP].amb;
        int32_t base = petals[petal].pads[PETAL_PAD_BASE].cdc;
        base -= petals[petal].pads[PETAL_PAD_BASE].amb;
        return tip - base;
    }
    if (cf == 0b1) {  // TIP
        int32_t tip = petals[petal].pads[PETAL_PAD_TIP].cdc;
        tip -= petals[petal].pads[PETAL_PAD_TIP].amb;
        return tip;
    }
    return 0;
}

int32_t captouch_get_petal_phi(uint8_t petal) {
    if (petal > 9) petal = 9;
    uint8_t cf = petals[petal].config_mask;
    if ((cf == 0b1110) || (cf == 0b110) || (cf == 0b111)) {  // CCW, CW, (BASE)
        int32_t left = petals[petal].pads[PETAL_PAD_CCW].cdc;
        left -= petals[petal].pads[PETAL_PAD_CCW].amb;
        int32_t right = petals[petal].pads[PETAL_PAD_CW].cdc;
        right -= petals[petal].pads[PETAL_PAD_CW].amb;
        return left - right;
    }
    return 0;
}

void captouch_init(void) {
    captouch_init_petals();
    chip_top = &chip_top_rev5;
    chip_bot = &chip_bot_rev5;

    captouch_init_chip(chip_top, (struct ad7147_device_config){
                                     .sequence_stage_num = 11,
                                     .decimation = 1,
                                 });

    captouch_init_chip(chip_bot, (struct ad7147_device_config){
                                     .sequence_stage_num = 11,
                                     .decimation = 1,
                                 });
}

uint16_t read_captouch() {
    uint16_t bin_petals = 0;
    for (int i = 0; i < 10; i++) {
        if (petals[i].pressed) {
            bin_petals |= (1 << i);
        }
    }
    return bin_petals;
}

void read_captouch_ex(captouch_state_t *state) {
    for (int i = 0; i < 10; i++) {
        state->petals[i].pressed = petals[i].pressed > 0;
        state->petals[i].pads.base_pressed =
            petals[i].pads[PETAL_PAD_BASE].pressed > 0;
        state->petals[i].pads.tip_pressed =
            petals[i].pads[PETAL_PAD_TIP].pressed > 0;
        state->petals[i].pads.cw_pressed =
            petals[i].pads[PETAL_PAD_CW].pressed > 0;
        state->petals[i].pads.ccw_pressed =
            petals[i].pads[PETAL_PAD_CCW].pressed > 0;
    }
}

uint16_t cdc_data[2][12] = {
    0,
};
uint16_t cdc_ambient[2][12] = {
    0,
};

static volatile uint32_t calib_active = 0;

static uint8_t calib_cycles = 0;
void captouch_force_calibration() {
    if (!calib_cycles) {    // last calib has finished
        calib_cycles = 16;  // goal cycles, can be argument someday
        Atomic_Increment_u32(&calib_active);
    }
}

uint8_t captouch_calibration_active() {
    return Atomic_CompareAndSwap_u32(&calib_active, 0, 0) ==
           ATOMIC_COMPARE_AND_SWAP_FAILURE;
}

void check_petals_pressed() {
    for (int i = 0; i < 10; i++) {
        bool petal_pressed = false;
        for (int j = 0; j < 4; j++) {
            bool pad_pressed = false;
            if ((petals[i].pads[j].amb + petals[i].pads[j].thres) <
                petals[i].pads[j].cdc) {
                petal_pressed = true;
                pad_pressed = true;
            }

            if (pad_pressed) {
                petals[i].pads[j].pressed = PETAL_PRESSED_DEBOUNCE;
            } else if (petals[i].pads[j].pressed) {
                petals[i].pads[j].pressed--;
            }
        }
        if (petal_pressed) {
            petals[i].pressed = PETAL_PRESSED_DEBOUNCE;
        } else if (petals[i].pressed) {
            petals[i].pressed--;
        }
    }
}

void cdc_to_petal(bool bot, bool amb, uint16_t cdc_data[],
                  uint8_t cdc_data_length) {
    for (int i = 0; i < cdc_data_length; i++) {
        size_t petal_index = bot ? bot_map[i] : top_map[i];
        size_t pad_index = bot ? bot_segment_map[i] : top_segment_map[i];
        petal_pad_t *pad = &petals[petal_index].pads[pad_index];
        uint16_t *target = amb ? &pad->amb : &pad->cdc;
        *target = cdc_data[i];
    }
}

uint16_t captouch_get_petal_pad_raw(uint8_t petal, uint8_t pad) {
    if (petal > 9) petal = 9;
    if (pad > 3) pad = 3;
    return petals[petal].pads[pad].cdc;
}
uint16_t captouch_get_petal_pad_calib_ref(uint8_t petal, uint8_t pad) {
    if (petal > 9) petal = 9;
    if (pad > 3) pad = 3;
    return petals[petal].pads[pad].amb;
}
uint16_t captouch_get_petal_pad(uint8_t petal, uint8_t pad) {
    if (petal > 9) petal = 9;
    if (pad > 3) pad = 3;
    if (petals[petal].pads[pad].amb < petals[petal].pads[pad].cdc) {
        return petals[petal].pads[pad].cdc - petals[petal].pads[pad].amb;
    }
    return 0;
}

void captouch_set_petal_pad_threshold(uint8_t petal, uint8_t pad,
                                      uint16_t thres) {
    if (petal > 9) petal = 9;
    if (pad > 3) pad = 3;
    petals[petal].pads[pad].thres = thres;
}

static int32_t calib_target = 6000;

void captouch_set_calibration_afe_target(uint16_t target) {
    calib_target = target;
}

void captouch_read_cycle() {
    static uint8_t calib_cycle = 0;
    static uint8_t calib_div = 1;
    static uint32_t ambient_acc[2][12] = {{
                                              0,
                                          },
                                          {
                                              0,
                                          }};
    if (calib_cycles) {
        if (calib_cycle == 0) {  // last cycle has finished, setup new
            calib_cycle = calib_cycles;
            calib_div = calib_cycles;
            for (int j = 0; j < 12; j++) {
                ambient_acc[0][j] = 0;
                ambient_acc[1][j] = 0;
            }
        }

        ad714x_i2c_read(chip_top, 0xB, cdc_ambient[0], chip_top->stages);
        ad714x_i2c_read(chip_bot, 0xB, cdc_ambient[1], chip_bot->stages);
        for (int j = 0; j < 12; j++) {
            ambient_acc[0][j] += cdc_ambient[0][j];
            ambient_acc[1][j] += cdc_ambient[1][j];
        }

        // TODO: use median instead of average
        calib_cycle--;
        if (!calib_cycle) {  // calib cycle is complete
            for (int i = 0; i < 12; i++) {
                cdc_ambient[0][i] = ambient_acc[0][i] / calib_div;
                cdc_ambient[1][i] = ambient_acc[1][i] / calib_div;
            }
            cdc_to_petal(0, 1, cdc_ambient[0], 12);
            cdc_to_petal(1, 1, cdc_ambient[1], 12);
            calib_cycles = 0;

            uint8_t recalib = 0;
            for (int i = 0; i < 12; i++) {
                for (int j = 0; j < 2; j++) {
                    int32_t diff = ((int32_t)cdc_ambient[j][i]) - calib_target;
                    int8_t steps = diff / (AFE_INCR_CAP);
                    if ((steps > 1) || (steps < -1)) {
                        if (!captouch_configure_stage_afe_offset(1 - j, i,
                                                                 steps)) {
                            recalib = 1;
                        }
                    }
                }
            }
            if (recalib) {
                calib_cycles = 16;  // do another round
            } else {
                Atomic_Decrement_u32(&calib_active);
            }
        }
    } else {
        ad714x_i2c_read(chip_top, 0xB, cdc_data[0], chip_top->stages);
        cdc_to_petal(0, 0, cdc_data[0], 12);

        ad714x_i2c_read(chip_bot, 0xB, cdc_data[1], chip_bot->stages);
        cdc_to_petal(1, 0, cdc_data[1], 12);

        check_petals_pressed();
    }
}

static void captouch_print_debug_info_chip(const struct ad714x_chip *chip) {
    uint16_t *data;
    uint16_t *ambient;
    const int stages = chip->stages;

    if (chip == chip_top) {
        data = cdc_data[0];
        ambient = cdc_ambient[0];
    } else {
        data = cdc_data[1];
        ambient = cdc_ambient[1];
    }

    // Appease clang-tidy.
    (void)data;
    (void)ambient;
    ESP_LOGI(TAG, "CDC results: %X %X %X %X %X %X %X %X %X %X %X %X", data[0],
             data[1], data[2], data[3], data[4], data[5], data[6], data[7],
             data[8], data[9], data[10], data[11]);

    for (int stage = 0; stage < stages; stage++) {
        ESP_LOGI(TAG, "stage %d ambient: %X diff: %d", stage, ambient[stage],
                 data[stage] - ambient[stage]);
    }
}

void captouch_print_debug_info(void) {
    captouch_print_debug_info_chip(chip_top);
    captouch_print_debug_info_chip(chip_bot);
}
