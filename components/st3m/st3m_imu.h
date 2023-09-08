#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

void st3m_imu_init(void);

void st3m_imu_read_acc_mps(float *x, float *y, float *z);
void st3m_imu_read_gyro_dps(float *x, float *y, float *z);
void st3m_imu_read_pressure(float *pressure, float *temperature);

void st3m_imu_set_sink(void * sink, SemaphoreHandle_t * sink_sema );

typedef struct _st3m_imu_sink_t {
    float acc[3];
    float gyro[3];
    float pressure;
    float temperature;
} st3m_imu_sink_t;
