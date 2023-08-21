#include "st3m_mode.h"

#include <string.h>

#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "st3m_fs_flash.h"
#include "st3m_fs_sd.h"
#include "st3m_gfx.h"
#include "st3m_io.h"
#include "st3m_usb.h"

static const char *TAG = "st3m-mode";

static st3m_mode_t _mode = {
    .kind = st3m_mode_kind_starting,
    .message = NULL,
};
static SemaphoreHandle_t _mu;

static void _diskmode_flash(void) {
    st3m_usb_msc_conf_t msc = {
        .block_size = st3m_fs_flash_get_blocksize(),
        .block_count = st3m_fs_flash_get_blockcount(),
        .product_id = { 'f', 'l', 'o', 'w', '3', 'r', ' ', 'f', 'l', 'a', 's',
                        'h', 0, 0, 0, 0 },
        .fn_read10 = st3m_fs_flash_read,
        .fn_write10 = st3m_fs_flash_write,
    };
    st3m_usb_mode_t usb_mode = {
        .kind = st3m_usb_mode_kind_disk,
        .disk = &msc,
    };
    st3m_usb_mode_switch(&usb_mode);
}

static void _diskmode_sd(void) {
    esp_err_t ret = st3m_fs_sd_unmount();
    if (ret != ESP_OK) {
        // TODO(q3k): handle error
        ESP_LOGE(TAG, "st3m_fs_sd_umount: %s", esp_err_to_name(ret));
        return;
    }

    st3m_fs_sd_props_t props;
    ret = st3m_fs_sd_probe(&props);
    if (ret != ESP_OK) {
        // TODO(q3k): handle error
        ESP_LOGE(TAG, "st3m_fs_sd_probe: %s", esp_err_to_name(ret));
        return;
    }

    st3m_usb_msc_conf_t msc = {
        .block_size = props.sector_size,
        .block_count = props.sector_count,
        .product_id = { 'f', 'l', 'o', 'w', '3', 'r', ' ', 'S', 'D', 0, 0, 0, 0,
                        0, 0, 0 },
        .fn_read10 = st3m_fs_sd_read10,
        .fn_write10 = st3m_fs_sd_write10,
    };
    st3m_usb_mode_t usb_mode = {
        .kind = st3m_usb_mode_kind_disk,
        .disk = &msc,
    };
    st3m_usb_mode_switch(&usb_mode);
}

void st3m_mode_set(st3m_mode_kind_t kind, const char *message) {
    xSemaphoreTake(_mu, portMAX_DELAY);
    if (_mode.message != NULL) {
        free(_mode.message);
        _mode.message = NULL;
    }

    if (message != NULL) {
        size_t sz = strlen(message) + 1;
        char *msg = malloc(sz);
        assert(msg != NULL);
        strncpy(msg, message, sz);
        _mode.message = msg;
    } else {
        _mode.message = NULL;
    }

    _mode.kind = kind;
    _mode.shown = false;

    if (kind == st3m_mode_kind_disk_flash) {
        _diskmode_flash();
    }
    if (kind == st3m_mode_kind_disk_sd) {
        _diskmode_sd();
    }
    xSemaphoreGive(_mu);
}

void st3m_mode_update_display(bool *restartable) {
    if (restartable != NULL) *restartable = false;

    xSemaphoreTake(_mu, portMAX_DELAY);
    switch (_mode.kind) {
        case st3m_mode_kind_app:
        case st3m_mode_kind_invalid:
            // Nothing to do.
            break;
        case st3m_mode_kind_starting: {
            const char *lines[] = {
                _mode.message,
                NULL,
            };
            st3m_gfx_textview_t tv = {
                .title = "Starting...",
                .lines = lines,
            };
            st3m_gfx_show_textview(&tv);
            break;
        }
        case st3m_mode_kind_disk_sd:
        case st3m_mode_kind_disk_flash: {
            const char *lines[] = {
                "Press right shoulder button",
                "to exit.",
                NULL,
            };
            st3m_gfx_textview_t tv = {
                .title = "Disk Mode",
                .lines = lines,
            };
            st3m_gfx_show_textview(&tv);
            if (restartable != NULL) *restartable = true;
        } break;
        case st3m_mode_kind_repl:
            if (!_mode.shown) {
                _mode.shown = true;
                const char *lines[] = {
                    "Send Ctrl-D over USB",
                    "or press right shoulder button",
                    "to restart.",
                    NULL,
                };
                st3m_gfx_textview_t tv = {
                    .title = "In REPL",
                    .lines = lines,
                };
                st3m_gfx_show_textview(&tv);
            }
            if (restartable != NULL) *restartable = true;
            break;
        case st3m_mode_kind_fatal: {
            const char *msg = _mode.message;
            if (msg == NULL) {
                msg = "";
            }
            const char *lines[] = {
                msg,
                "Press right shoulder button",
                "to restart.",
                NULL,
            };
            st3m_gfx_textview_t tv = {
                .title = "Fatal error",
                .lines = lines,
            };
            st3m_gfx_show_textview(&tv);
            if (restartable != NULL) *restartable = true;
            break;
        }
    }
    xSemaphoreGive(_mu);
}

static void _task(void *arg) {
    (void)arg;
    TickType_t last_wake;

    last_wake = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&last_wake, 100 / portTICK_PERIOD_MS);

        bool restartable = false;
        st3m_mode_update_display(&restartable);

        if (restartable) {
            st3m_tripos tp = st3m_io_right_button_get();
            if (tp == st3m_tripos_mid) {
                st3m_gfx_textview_t tv = {
                    .title = "Restarting...",
                };
                st3m_gfx_show_textview(&tv);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                esp_restart();
            }
        }
    }
}

void st3m_mode_init(void) {
    assert(_mu == NULL);
    _mu = xSemaphoreCreateMutex();
    assert(_mu != NULL);

    xTaskCreate(_task, "mode", 4096, NULL, 1, NULL);
}