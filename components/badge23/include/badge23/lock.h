#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

SemaphoreHandle_t mutex_i2c;
SemaphoreHandle_t mutex_LED;
