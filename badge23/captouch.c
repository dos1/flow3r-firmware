//#include <stdio.h>
//#include <string.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include <stdint.h>


static const char *TAG = "captouch";

#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */

#define AD7147_BASE_ADDR            0x2C

#define AD7147_REG_PWR_CONTROL              0x00
#define AD7147_REG_STAGE_CAL_EN             0x01
#define AD7147_REG_STAGE_HIGH_INT_ENABLE    0x06
#define AD7147_REG_DEVICE_ID                0x17

#define TIMEOUT_MS                  1000

struct ad714x_chip {
    uint8_t addr;
    uint8_t gpio;
    int afe_offsets[13];
    int stages;
};

static const struct ad714x_chip chip_top = {.addr = AD7147_BASE_ADDR + 1, .gpio = 48, .afe_offsets = {24, 12, 16, 33, 30, 28, 31, 27, 22, 24, 18, 19, }, .stages=12};
static const struct ad714x_chip chip_bot = {.addr = AD7147_BASE_ADDR, .gpio = 3, .afe_offsets = {3, 2, 1, 1 ,1, 1, 1, 1, 2, 3}, .stages=10};

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

#define ESP_INTR_FLAG_DEFAULT 0

static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    struct ad714x_chip* chip = (struct ad714x_chip *) arg;
    xQueueSendFromISR(gpio_evt_queue, &chip, NULL);
}

void manual_captouch_readout(uint8_t top)
{
    struct ad714x_chip* chip = top ? (&chip_top) : (&chip_bot);
    xQueueSendFromISR(gpio_evt_queue, &chip, NULL);
}

void espan_handle_captouch(uint16_t pressed_top, uint16_t pressed_bot);

static uint16_t pressed_top, pressed_bot;
void gpio_event_handler(void* arg)
{
    static unsigned long counter = 0;
    struct ad714x_chip* chip;
    uint16_t pressed;
    while(true) {
        if(xQueueReceive(gpio_evt_queue, &chip, portMAX_DELAY)) {
            ad714x_i2c_read(chip, 9, &pressed, 1);
            ESP_LOGI(TAG, "Addr %x, High interrupt %X", chip->addr, pressed);

            pressed &= ((1 << chip->stages) - 1);

            if(chip == &chip_top) pressed_top = pressed;
            if(chip == &chip_bot) pressed_bot = pressed;
            espan_handle_captouch(pressed_top, pressed_bot);
        }
    }
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
        stage_config.cinX_connection_setup[i] = CIN_CDC_POS;
        stage_config.pos_afe_offset=chip->afe_offsets[i];
        ad714x_set_stage_config(chip, i, &stage_config);
    }

    // Force calibration
    ad714x_i2c_write(chip, 2, (1 << 14));

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << chip->gpio);
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);

    gpio_evt_queue = xQueueCreate(10, sizeof(const struct ad714x_chip*));
    xTaskCreate(gpio_event_handler, "gpio_event_handler", 2048 * 2, NULL, configMAX_PRIORITIES - 2, NULL);
    gpio_isr_handler_add(chip->gpio, gpio_isr_handler, (void *)chip);

}

void captouch_init(void)
{
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    captouch_init_chip(&chip_top, (struct ad7147_device_config){.sequence_stage_num = 11,
                                                 .decimation = 1,
                                                 .stage0_cal_en = 1,
                                                 .stage1_cal_en = 1,
                                                 .stage2_cal_en = 1,
                                                 .stage3_cal_en = 1,
                                                 .stage4_cal_en = 1,
                                                 .stage5_cal_en = 1,
                                                 .stage6_cal_en = 1,
                                                 .stage7_cal_en = 1,
                                                 .stage8_cal_en = 1,
                                                 .stage9_cal_en = 1,
                                                 .stage10_cal_en = 1,
                                                 .stage11_cal_en = 1,

                                                 .stage0_high_int_enable = 1,
                                                 .stage1_high_int_enable = 1,
                                                 .stage2_high_int_enable = 1,
                                                 .stage3_high_int_enable = 1,
                                                 .stage4_high_int_enable = 1,
                                                 .stage5_high_int_enable = 1,
                                                 .stage6_high_int_enable = 1,
                                                 .stage7_high_int_enable = 1,
                                                 .stage8_high_int_enable = 1,
                                                 .stage9_high_int_enable = 1,
                                                 .stage10_high_int_enable = 1,
                                                 .stage11_high_int_enable = 1,
                                                 });

    captouch_init_chip(&chip_bot, (struct ad7147_device_config){.sequence_stage_num = 11,
                                                 .decimation = 1,
                                                 .stage0_cal_en = 1,
                                                 .stage1_cal_en = 1,
                                                 .stage2_cal_en = 1,
                                                 .stage3_cal_en = 1,
                                                 .stage4_cal_en = 1,
                                                 .stage5_cal_en = 1,
                                                 .stage6_cal_en = 1,
                                                 .stage7_cal_en = 1,
                                                 .stage8_cal_en = 1,
                                                 .stage9_cal_en = 1,

                                                 .stage0_high_int_enable = 1,
                                                 .stage1_high_int_enable = 1,
                                                 .stage2_high_int_enable = 1,
                                                 .stage3_high_int_enable = 1,
                                                 .stage4_high_int_enable = 1,
                                                 .stage5_high_int_enable = 1,
                                                 .stage6_high_int_enable = 1,
                                                 .stage7_high_int_enable = 1,
                                                 .stage8_high_int_enable = 1,
                                                 .stage9_high_int_enable = 1,
                                                 });
}

static void captouch_print_debug_info_chip(const struct ad714x_chip* chip)
{
    uint16_t data[12] = {0,};
    uint16_t ambient[12] = {0,};
    const int stages = chip->stages;
#if 1
    for(int stage=0; stage<stages; stage++) {
        ad714x_i2c_read(chip, 0x0FA + stage * (0x104 - 0xE0), data, 1);
        ESP_LOGI(TAG, "stage %d threshold: %X", stage, data[0]);
    }

    ad714x_i2c_read(chip, 0xB, data, stages);
    ESP_LOGI(TAG, "CDC results: %X %X %X %X %X %X %X %X %X %X %X %X", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11]);

    for(int stage=0; stage<stages; stage++) {
        ad714x_i2c_read(chip, 0x0F1 + stage * (0x104 - 0xE0), &ambient[stage], 1);
        ESP_LOGI(TAG, "stage %d ambient: %X diff: %d", stage, ambient[stage], data[stage] - ambient[stage]);
    }

#endif
#if 1
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
    captouch_print_debug_info_chip(&chip_top);
    captouch_print_debug_info_chip(&chip_bot);
}

void captouch_get_cross(int paddle, int *x, int *y)
{
    uint16_t data[12] = {0,};
    uint16_t ambient[12] = {0,};

    int result[2] = {0, 0};
    float total = 0;

#if 0
    if(paddle == 2) {
        ad714x_i2c_read(&chip_top, 0xB, data, 3);
        //ESP_LOGI(TAG, "CDC results: %X %X %X %X %X %X %X %X %X %X %X %X", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11]);

        for(int stage=0; stage<3; stage++) {
            ad714x_i2c_read(&chip_top, 0x0F1 + stage * (0x104 - 0xE0), &ambient[stage], 1);
            //ESP_LOGI(TAG, "stage %d ambient: %X diff: %d", stage, ambient[stage], data[stage] - ambient[stage]);
        }

        int vectors[][2] = {{0, 0}, {0,240}, {240, 120}};
        total = (data[0] - ambient[0]) + (data[1] - ambient[1]) + (data[2] - ambient[2]);

        result[0] = vectors[0][0] * (data[0] - ambient[0]) + vectors[1][0] * (data[1] - ambient[1]) + vectors[2][0] * (data[2] - ambient[2]);
        result[1] = vectors[0][1] * (data[0] - ambient[0]) + vectors[1][1] * (data[1] - ambient[1]) + vectors[2][1] * (data[2] - ambient[2]);
    }

    if(paddle == 8) {
        ad714x_i2c_read(&chip_top, 0xB + 5, data + 5, 3);
        //ESP_LOGI(TAG, "CDC results: %X %X %X %X %X %X %X %X %X %X %X %X", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11]);

        for(int stage=5; stage<8; stage++) {
            ad714x_i2c_read(&chip_top, 0x0F1 + stage * (0x104 - 0xE0), &ambient[stage], 1);
            //ESP_LOGI(TAG, "stage %d ambient: %X diff: %d", stage, ambient[stage], data[stage] - ambient[stage]);
        }

        int vectors[][2] = {{240, 240}, {240, 0}, {0, 120}};
        total = (data[5] - ambient[5]) + (data[6] - ambient[6]) + (data[7] - ambient[7]);

        result[0] = vectors[0][0] * (data[5] - ambient[5]) + vectors[1][0] * (data[6] - ambient[6]) + vectors[2][0] * (data[7] - ambient[7]);
        result[1] = vectors[0][1] * (data[5] - ambient[5]) + vectors[1][1] * (data[6] - ambient[6]) + vectors[2][1] * (data[7] - ambient[7]);
    }

    *x = result[0] / total;
    *y = result[1] / total;

    //ESP_LOGI(TAG, "x=%d y=%d\n", *x, *y);
#endif

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
        chip = &chip_top;
    } else {
        chip = &chip_bot;
    }

    ad714x_i2c_read(chip, 0xB, data, 12);
    //ESP_LOGI(TAG, "CDC results: %X %X %X %X %X %X %X %X %X %X %X %X", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11]);

    for(int stage=0; stage<12; stage++) {
        ad714x_i2c_read(chip, 0x0F1 + stage * (0x104 - 0xE0), &ambient[stage], 1);
        //ESP_LOGI(TAG, "stage %d ambient: %X diff: %d", stage, ambient[stage], data[stage] - ambient[stage]);
    }

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
