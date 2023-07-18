#include "st3m_usb.h"

static const char *TAG = "st3m-usb";

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_private/usb_phy.h"
#include "tusb.h"

static SemaphoreHandle_t _mu = NULL;
static st3m_usb_mode_kind_t _mode = st3m_usb_mode_kind_disabled;
static usb_phy_handle_t phy_hdl;
static bool _connected = false;

static void _usb_task(void *_arg) {
	(void)_arg;
    while (true) {
  		tud_task_ext(10, false);

		xSemaphoreTake(_mu, portMAX_DELAY);
		st3m_usb_mode_kind_t mode = _mode;
		xSemaphoreGive(_mu);

		if (mode == st3m_usb_mode_kind_app) {
			st3m_usb_cdc_txpoll();
		}
    }
}

// Generate USB serial from on-board chip ID / MAC.
static void _generate_serial(void) {
	uint8_t mac[6];
	memset(mac, 0, 6);
	// Ignore error. Worst case we get zeroes.
	esp_read_mac(mac, ESP_MAC_WIFI_STA);

	char serial[13];
	snprintf(serial, 13, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	st3m_usb_descriptors_set_serial(serial);
}

void st3m_usb_init(void) {
	assert(_mu == NULL);
	_mu = xSemaphoreCreateMutex();
	assert(_mu != NULL);
	_mode = st3m_usb_mode_kind_disabled;

	_generate_serial();
	st3m_usb_cdc_init();
    usb_phy_config_t phy_conf = {
        .controller = USB_PHY_CTRL_OTG,
        .otg_mode = USB_OTG_MODE_DEVICE,
        .target = USB_PHY_TARGET_INT,
    };
	// TODO(q3k): set self-powered based on battery state?
    ESP_ERROR_CHECK(usb_new_phy(&phy_conf, &phy_hdl));
    usb_phy_action(phy_hdl, USB_PHY_ACTION_HOST_FORCE_DISCONN);

    if (!tusb_init()) {
        ESP_LOGE(TAG, "TInyUSB init failed");
        assert(false);
    }

   	xTaskCreate(_usb_task, "usb", 4096, NULL, 5, NULL);
	ESP_LOGI(TAG, "USB stack started");
}

void st3m_usb_mode_switch(st3m_usb_mode_t *mode) {
	xSemaphoreTake(_mu, portMAX_DELAY);

	bool running = false;
	switch (_mode) {
	case st3m_usb_mode_kind_app:
	case st3m_usb_mode_kind_disk:
		running = true;
	default:
	}

	bool should_run = false;
	switch (mode->kind) {
	case st3m_usb_mode_kind_app:
	case st3m_usb_mode_kind_disk:
		should_run = true;
	default:
	}

	if (running) {
		ESP_LOGI(TAG, "stopping and disconnecting");
        usb_phy_action(phy_hdl, USB_PHY_ACTION_HOST_FORCE_DISCONN);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	st3m_usb_descriptors_switch(mode);
	if (mode->kind == st3m_usb_mode_kind_disk) {
		assert(mode->disk != NULL);
		st3m_usb_msc_set_conf(mode->disk);
	}
	if (mode->kind == st3m_usb_mode_kind_app) {
		assert(mode->app != NULL);
		st3m_usb_cdc_set_conf(mode->app);
	}

	if (should_run) {
		ESP_LOGI(TAG, "reconnecting and starting");
        usb_phy_action(phy_hdl, USB_PHY_ACTION_HOST_ALLOW_CONN);
	}

	_mode = mode->kind;
	_connected = false;
	xSemaphoreGive(_mu);
}

bool st3m_usb_connected(void) {
	xSemaphoreTake(_mu, portMAX_DELAY);
	bool res = _connected;
	xSemaphoreGive(_mu);
	return res;
}

void tud_mount_cb(void) {
	ESP_LOGI(TAG, "USB attached");
	xSemaphoreTake(_mu, portMAX_DELAY);
	_connected = true;
	xSemaphoreGive(_mu);
}

void tud_suspend_cb(bool remote_wakeup_en) {
	ESP_LOGI(TAG, "USB detached");

	xSemaphoreTake(_mu, portMAX_DELAY);
	st3m_usb_mode_kind_t mode = _mode;
	_connected = false;
	xSemaphoreGive(_mu);
	if (mode == st3m_usb_mode_kind_app) {
		st3m_usb_cdc_detached();
	}
}