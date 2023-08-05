#include "st3m_imu.h"

#include "flow3r_bsp_imu.h"

static const char *TAG = "st3m-imu";
#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static void _task(void *data);

static flow3r_bsp_imu_t _imu;

static SemaphoreHandle_t _mu;
#define LOCK xSemaphoreTake(_mu, portMAX_DELAY)
#define UNLOCK xSemaphoreGive(_mu)

static float _acc_x, _acc_y, _acc_z;
static float _gyro_x, _gyro_y, _gyro_z;
static float _pressure;
static float _temperature;

void st3m_imu_init() {
    _mu = xSemaphoreCreateMutex();
    assert(_mu != NULL);

    esp_err_t ret = flow3r_bsp_imu_init(&_imu);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "IMU init failed: %s", esp_err_to_name(ret));
        return;
    }

    xTaskCreate(&_task, "imu", 4096, NULL, configMAX_PRIORITIES - 2, NULL);
    ESP_LOGI(TAG, "IMU task started");
}

void st3m_imu_read_acc_mps(float *x, float *y, float *z) {
    LOCK;
    *x = _acc_x;
    *y = _acc_y;
    *z = _acc_z;
    UNLOCK;
}

void st3m_imu_read_gyro_dps(float *x, float *y, float *z) {
    LOCK;
    *x = _gyro_x;
    *y = _gyro_y;
    *z = _gyro_z;
    UNLOCK;
}

void st3m_imu_read_pressure(float *pressure, float *temperature) {
    LOCK;
    *pressure = _pressure;
    *temperature = _temperature;
    UNLOCK;
}

static void _task(void *data) {
    TickType_t last_wake = xTaskGetTickCount();
    esp_err_t ret;
    float a, b, c;
    while (1) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));  // 100 Hz

        ret = flow3r_bsp_imu_update(&_imu);
        if (ret != ESP_OK) {
            continue;
        }

        LOCK;
        ret = flow3r_bsp_imu_read_acc_mps(&_imu, &a, &b, &c);
        if (ret == ESP_OK) {
            _acc_x = a;
            _acc_y = b;
            _acc_z = c;
        }

        flow3r_bsp_imu_read_gyro_dps(&_imu, &a, &b, &c);
        if (ret == ESP_OK) {
            _gyro_x = a;
            _gyro_y = b;
            _gyro_z = c;
        }

        flow3r_bsp_imu_read_pressure(&_imu, &a, &b);
        if (ret == ESP_OK) {
            _pressure = a;
            _temperature = b;
        }
        UNLOCK;
    }
}
