#pragma once

#include "bmi2_defs.h"
#include "bmp5.h"

#include "flow3r_bsp_i2c.h"

#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    struct bmi2_dev bmi;
    struct bmp5_dev bmp;
    uint8_t bmi_dev_addr;
    uint8_t bmp_dev_addr;
    int acc_range;   // accelerometer range in g.
    int gyro_range;  // gyroscope range in degrees per second.
    struct bmp5_osr_odr_press_config osr_odr_press_cfg;
} flow3r_bsp_imu_t;

// Init the IMU to default settings
//
// Configures the IMU to:
// Accelerometer: 100 Hz sample rate, 2 g range
// Gyroscope: 100 Hz sample rate, 200 dps range
// Pressure sensor: 50 Hz sample rate
esp_err_t flow3r_bsp_imu_init(flow3r_bsp_imu_t *imu);

// Query the IMU for an accelerometer reading.
//
// This directly calls the I2C bus and waits until the bus is available (max 1
// second). Returns ESP_ERR_NOT_FOUND if there is no new reading available.
// Returns ESP_FAIL if the sensor could not be read (e.g. I2C unavailable).
// Return values are raw data from the BMI270.
// Use imu->acc_range and imu->bmi.resolution for interpretation.
esp_err_t flow3r_bsp_imu_read_acc(flow3r_bsp_imu_t *imu, int *x, int *y,
                                  int *z);

// Query the IMU for a converted accelerometer reading.
//
// This directly calls the I2C bus and waits until the bus is available (max 1
// second). Returns ESP_ERR_NOT_FOUND if there is no new reading available.
// Returns ESP_FAIL if the sensor could not be read (e.g. I2C unavailable).
// Return values in m/s**2.
esp_err_t flow3r_bsp_imu_read_acc_mps(flow3r_bsp_imu_t *imu, float *x, float *y,
                                      float *z);

// Query the IMU for a gyroscope reading.
//
// This directly calls the I2C bus and waits until the bus is available (max 1
// second). Returns ESP_ERR_NOT_FOUND if there is no new reading available.
// Returns ESP_FAIL if the sensor could not be read (e.g. I2C unavailable).
// Return values are raw data from the BMI270.
// Use imu->gyro_range and imu->bmi.resolution for interpretation.
esp_err_t flow3r_bsp_imu_read_gyro(flow3r_bsp_imu_t *imu, int *x, int *y,
                                   int *z);

// Query the IMU for a converted gyroscope reading.
//
// This directly calls the I2C bus and waits until the bus is available (max 1
// second). Returns ESP_ERR_NOT_FOUND if there is no new reading available.
// Returns ESP_FAIL if the sensor could not be read (e.g. I2C unavailable).
// Return values in deg/s.
esp_err_t flow3r_bsp_imu_read_gyro_dps(flow3r_bsp_imu_t *imu, float *x,
                                       float *y, float *z);

// Query the IMU for a pressure sensor reading.
//
// Returns cached data if no new reading is available.
// Presssure in Pa, temperature in deg C
esp_err_t flow3r_bsp_imu_read_pressure(flow3r_bsp_imu_t *imu, float *pressure,
                                       float *temperature);
