// Driver for APA102-style LEDs, using SPI peripheral.

#include "flow3r_bsp_spiled.h"

#include <string.h>

#include "driver/spi_master.h"
#include "esp_log.h"

static const char *TAG = "flow3r-spiled";

static spi_bus_config_t buscfg;
static spi_device_interface_config_t devcfg;
static spi_device_handle_t spi_led;
static spi_transaction_t spi_trans_object;

typedef struct {
    uint16_t num_leds;
    uint8_t *buffer;
} flow3r_bsp_spiled_t;

static flow3r_bsp_spiled_t spiled;


esp_err_t flow3r_bsp_spiled_init(uint16_t num_leds) {
    // Set up the Bus Config struct
    buscfg.miso_io_num = -1;
    buscfg.mosi_io_num = 18;
    buscfg.sclk_io_num = 8;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = 8000;

    // Set up the SPI Device Configuration Struct
    devcfg.clock_speed_hz = 10000000;
    devcfg.mode = 0;
    devcfg.spics_io_num = -1;
    devcfg.queue_size = 1;

    // Initialize the SPI driver
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize: %s", esp_err_to_name(ret));
        return ret;
    }
    // Add SPI port to bus
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi_led);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device: %s", esp_err_to_name(ret));
        return ret;
    }

	spiled.num_leds = num_leds;
    spiled.buffer = calloc(4, num_leds + 2);
	if (spiled.buffer == NULL) {
		ESP_LOGE(TAG, "buffer allocation failed");
		return ESP_ERR_NO_MEM;
	}
    // Start Frame
    spiled.buffer[0] = 0;
    spiled.buffer[1] = 0;
    spiled.buffer[2] = 0;
    spiled.buffer[3] = 0;
    // Global brightness.
    for (int i = 0; i < num_leds; i++) {
        spiled.buffer[i * 4 + 4] = 255;
    }
    // End Frame (this only works with up to 64 LEDs, meh).
    int i = num_leds * 4 + 8;
    spiled.buffer[i + 0] = 255;
    spiled.buffer[i + 1] = 255;
    spiled.buffer[i + 2] = 255;
    spiled.buffer[i + 3] = 255;

    memset(&spi_trans_object, 0, sizeof(spi_trans_object));
    spi_trans_object.length = (4 * (num_leds+2))* 8;
    spi_trans_object.tx_buffer = spiled.buffer;
    return ESP_OK;
}

void flow3r_bsp_spiled_set_pixel(uint32_t index, uint32_t red, uint32_t green, uint32_t blue) {
	if (spiled.buffer == NULL) {
		return;
	}
	if (index >= spiled.num_leds) {
		return;
	}
    uint32_t start = index * 4 + 4;
    spiled.buffer[start + 1] = blue & 0xFF;
    spiled.buffer[start + 2] = green & 0xFF;
    spiled.buffer[start + 3] = red & 0xFF;
}

esp_err_t flow3r_bsp_spiled_refresh(int32_t timeout_ms) {
    return spi_device_queue_trans(spi_led, &spi_trans_object, timeout_ms);
}