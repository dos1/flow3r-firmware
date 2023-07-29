#pragma once

#include "bmi2_defs.h"

#include "flow3r_bsp_i2c.h"

#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    struct bmi2_dev bmi;
    uint8_t dev_addr;
} flow3r_bsp_imu_t;

esp_err_t flow3r_bsp_imu_init(flow3r_bsp_imu_t *imu);
esp_err_t flow3r_bsp_imu_read_acc_mps(flow3r_bsp_imu_t *imu, float *x,
                                         float *y, float *z);
