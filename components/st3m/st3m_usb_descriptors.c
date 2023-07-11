#include "st3m_usb.h"

#include "tusb.h"
#include "esp_log.h"

static const char *TAG = "st3m-usb-descs";

static const char * const _manufacturer = "flow3r.garden";
static const char * const _product_app = "flow3r";
static const char * const _product_disk = "flow3r (disk mode)";
static const char * const _cdc_repl = "MicroPython REPL";
static char _serial[32+1] = {0};

typedef struct {
	const tusb_desc_device_t dev;
	const uint8_t *cfg;
	size_t num_str;
	const char *str[16];
} st3m_usb_descriptor_set_t;

static const uint32_t st3m_usb_configdesc_len_app = TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN;

#define EPNUM_CDC_0_NOTIF 0x81
#define EPNUM_CDC_0_OUT   0x02
#define EPNUM_CDC_0_IN    0x82

static uint8_t _cfg_desc_app[] = {
   	TUD_CONFIG_DESCRIPTOR(1, st3m_usb_interface_app_total, 0, st3m_usb_configdesc_len_app, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
	TUD_CDC_DESCRIPTOR(st3m_usb_interface_app_cdc, 4, EPNUM_CDC_0_NOTIF, 8, EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN, 64),
};

static st3m_usb_descriptor_set_t _descset_app = {
	.dev = {
    	.bLength = sizeof(tusb_desc_device_t),
    	.bDescriptorType = TUSB_DESC_DEVICE,
    	.bcdUSB = 0x0200,

    	.bDeviceClass = 0x00,
    	.bDeviceSubClass = 0x00,
    	.bDeviceProtocol = 0x00,

    	.bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    	.idVendor = 0x303a,
    	.idProduct = 0x4042,

    	.bcdDevice = 0x0100,

    	.iManufacturer = 0x01,
    	.iProduct = 0x02,
    	.iSerialNumber = 0x03,

    	.bNumConfigurations = 0x01,
	},
	.cfg = _cfg_desc_app,
	.num_str = 5,
	.str = {
    	(char[]){0x09, 0x04}, // EN
		_manufacturer,
		_product_app,
		_serial,
		_cdc_repl,
	},
};

static const uint32_t st3m_usb_configdesc_len_disk = TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN;

 // Use separate endpoint numbers otherwise TinyUSB seems to get very confused...
 #define EPNUM_1_MSC_OUT 0x03
 #define EPNUM_1_MSC_IN  0x83

static uint8_t _cfg_desc_disk[] = {
   	TUD_CONFIG_DESCRIPTOR(1, st3m_usb_interface_disk_total, 0, st3m_usb_configdesc_len_disk, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
	TUD_MSC_DESCRIPTOR(st3m_usb_interface_disk_msc, 0, EPNUM_1_MSC_OUT, EPNUM_1_MSC_IN, 64),
};

static st3m_usb_descriptor_set_t _descset_disk = {
	.dev = {
    	.bLength = sizeof(tusb_desc_device_t),
    	.bDescriptorType = TUSB_DESC_DEVICE,
    	.bcdUSB = 0x0200,

    	.bDeviceClass = 0x00,
    	.bDeviceSubClass = 0x00,
    	.bDeviceProtocol = 0x00,

    	.bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    	.idVendor = 0x303a,
    	.idProduct = 0x4023,

    	.bcdDevice = 0x0100,

    	.iManufacturer = 0x01,
    	.iProduct = 0x02,
    	.iSerialNumber = 0x03,

    	.bNumConfigurations = 0x01,
	},
	.cfg = _cfg_desc_disk,
	.num_str = 4,
	.str = {
    	(char[]){0x09, 0x04}, // EN
		_manufacturer,
		_product_disk,
		_serial,
	},
};

static st3m_usb_descriptor_set_t *_descset_current = &_descset_app;

uint8_t const *tud_descriptor_device_cb(void) {
	return (uint8_t const *)&_descset_current->dev;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
	return _descset_current->cfg;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void) langid; // Unused, this driver supports only one language in string descriptors
    uint8_t chr_count;
    static uint16_t _desc_str[64];

	const char **descs = _descset_current->str;

    if (index == 0) {
        memcpy(&_desc_str[1], descs[0], 2);
        chr_count = 1;
    } else {
        if (index >= _descset_current->num_str) {
            ESP_LOGW(TAG, "String index (%u) is out of bounds, check your string descriptor", index);
            return NULL;
        }

        if (descs[index] == NULL) {
            ESP_LOGW(TAG, "String index (%u) points to NULL, check your string descriptor", index);
            return NULL;
        }

        const char *str = descs[index];
        chr_count = strnlen(str, 63); // Buffer len - header

        // Convert ASCII string into UTF-16
        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    // First byte is length in bytes (including header), second byte is descriptor type (TUSB_DESC_STRING)
    _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2 * chr_count + 2);

    return _desc_str;
}

void st3m_usb_descriptors_switch(st3m_usb_mode_t *mode) {
	switch (mode->kind) {
	case st3m_usb_mode_kind_app:
		_descset_current = &_descset_app;
		break;
	case st3m_usb_mode_kind_disk:
		_descset_current = &_descset_disk;
		break;
	default:
	}
}

void st3m_usb_descriptors_set_serial(const char *serial) {
	memset(_serial, 0, 33);
	strncpy(_serial, serial, 32);
}