#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

void st3m_imu_init(void);

esp_err_t st3m_imu_read_acc_mps(float *x, float *y, float *z);
