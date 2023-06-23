#include "flow3r_bsp_i2c.h"

#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static SemaphoreHandle_t mutex;

static const char *TAG = "flow3r-bsp-i2c";

#if defined(CONFIG_BADGE23_HW_GEN_P1)
const flow3r_i2c_addressdef flow3r_i2c_addresses = {
	.codec = 0, // p1 has no i2c control channel to codec.
	.touch_top = 0x2d,
	.touch_bottom = 0x2c,
	.portexp = { 0x6e, 0x6d },
};
#elif defined(CONFIG_BADGE23_HW_GEN_P3)
const flow3r_i2c_addressdef flow3r_i2c_addresses = {
	.codec = 0x10,
	.touch_top = 0x2d,
	.touch_bottom = 0x2c,
	.portexp = { 0x6e, 0x6d },
};
#elif defined(CONFIG_BADGE23_HW_GEN_P4)
const flow3r_i2c_addressdef flow3r_i2c_addresses = {
	.codec = 0x10,
	.touch_top = 0x2c,
	.touch_bottom = 0x2d,
	.portexp = { 0x6e, 0x6d },
};
#elif defined(CONFIG_BADGE23_HW_GEN_P6)
const flow3r_i2c_addressdef flow3r_i2c_addresses = {
	.codec = 0x10,
	.touch_top = 0x2c,
	.touch_bottom = 0x2d,
	.portexp = { 0x6e, 0x6d },
};
#else
#error "i2c not implemented for this badge generation"
#endif

#if defined(CONFIG_BADGE23_HW_GEN_P3) || defined(CONFIG_BADGE23_HW_GEN_P4) || defined(CONFIG_BADGE23_HW_GEN_P6)
static i2c_config_t i2c_conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = 2,
    .scl_io_num = 1,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 400000,
};
#elif defined(CONFIG_BADGE23_HW_GEN_P1)
static i2c_config_t i2c_conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = 10,
    .scl_io_num = 9,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 400000,
};
#else
#error "i2c not implemented for this badge generation"
#endif

void flow3r_bsp_i2c_init(void) {
	if (mutex != NULL) {
		return;
	}

	mutex = xSemaphoreCreateMutex();
	assert(mutex != NULL);

    assert(i2c_param_config(0, &i2c_conf) == ESP_OK);
	assert(i2c_driver_install(0, i2c_conf.mode, 0, 0, 0) == ESP_OK);
}

// Take I2C bus lock.
static void flow3r_bsp_i2c_get(void) {
    xSemaphoreTake(mutex, portMAX_DELAY);
}

// Release I2C bus lock.
static void flow3r_bsp_i2c_put(void) {
    xSemaphoreGive(mutex);
}

esp_err_t flow3r_bsp_i2c_write_to_device(uint8_t address, const uint8_t *buffer, size_t write_size, TickType_t ticks_to_wait) {
	flow3r_bsp_i2c_get();
	esp_err_t res = i2c_master_write_to_device(0, address, buffer, write_size, ticks_to_wait);
	flow3r_bsp_i2c_put();
	return res;
}

esp_err_t flow3r_bsp_i2c_write_read_device(uint8_t address, const uint8_t *write_buffer, size_t write_size, uint8_t *read_buffer, size_t read_size, TickType_t ticks_to_wait) {
	flow3r_bsp_i2c_get();
	esp_err_t res = i2c_master_write_read_device(0, address, write_buffer, write_size, read_buffer, read_size, ticks_to_wait);
	flow3r_bsp_i2c_put();
	return res;
}