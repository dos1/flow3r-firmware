#include "st3m_mode.h"

#include <string.h>

#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "st3m_gfx.h"
#include "st3m_io.h"

static st3m_mode_t _mode = {
    .kind = st3m_mode_kind_starting,
    .message = NULL,
};
static SemaphoreHandle_t _mu;

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
        case st3m_mode_kind_disk:
            st3m_gfx_splash("Disk Mode");
            break;
        case st3m_mode_kind_repl: {
            const char *lines[] = {
                "Send Ctrl-D over USB",
                "or press left shoulder button",
                "to restart.",
                NULL,
            };
            st3m_gfx_textview_t tv = {
                .title = "In REPL",
                .lines = lines,
            };
            st3m_gfx_show_textview(&tv);
            if (restartable != NULL) *restartable = true;
            break;
        }
        case st3m_mode_kind_fatal: {
            const char *msg = _mode.message;
            if (msg == NULL) {
                msg = "";
            }
            const char *lines[] = {
                msg,
                "Press left shoulder button",
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
            st3m_tripos tp = st3m_io_left_button_get();
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