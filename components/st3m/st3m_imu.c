#include "st3m_imu.h"

#include "flow3r_bsp_imu.h"

#include "flow3r_bsp.h"

#include "esp_system.h"
#include "esp_task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static flow3r_bsp_imu_t imu;

void st3m_imu_init() { flow3r_bsp_imu_init(&imu); }

esp_err_t st3m_imu_read_acc_mps(float *x, float *y, float *z) {
    return flow3r_bsp_imu_read_acc_mps(&imu, x, y, z);
}
