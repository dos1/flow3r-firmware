#include "bl00mbox.h"
#include "flow3r_bsp.h"
#include "st3m_audio.h"
#include "st3m_captouch.h"
#include "st3m_console.h"
#include "st3m_gfx.h"
#include "st3m_imu.h"
#include "st3m_io.h"
#include "st3m_leds.h"
#include "st3m_mode.h"
#include "st3m_scope.h"
#include "st3m_usb.h"

#include "fs.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs_flash.h"

// Initialization is unfortunately quite complicated.
//
// In a perfect world, we'd just init everything in the main function in order.
// Unfortunately, the ESP32's interrupt routing is per-core, and the ESP-IDF has
// no API to configure interrupt handling to be performed by a core different
// than the one which performed the interrupt setup.
//
// Since calls to flow3r_bsp_*/st3m_* perform calls to initialize various ESP32
// peripherals via the ESP-IDF, it means these calls set up interrupts to be
// handled by whatever core is executing the initialization code.
//
// Thus, we must perform a complicated dance in which we set up parts of
// st3m/flow3r_bsp from one core, and part from another. Since flow3r_startup is
// called on core0, we set up another task whose only job is to initialize
// interrupts from core1.
//
// In the future we might refactor the codebase to use core pinning for all
// tasks, and then initiazlize interrupts from within tasks, effectively keeping
// interrupts local to handler tasks. But for now, we employ the strategy of
// generally trusting the scheduler and instead selecting which interrupts is
// handled by which core by performing initialization here.
//
// And we have to actually distribute these interrupts because each Xtensa core
// simply doesn't have enough interrupt slots to easily handle all of our
// external peripherals. Especially with the way the ESP-IDF interrupt
// allocation algorithm works. Core0 is already quite polluted by micropython
// doing its own initialization there, so we effectively push most interrupts to
// core1.
//
// TODO(q3k): revisit this mess.

// Populated by the core1 init task whenever core1 init is done.
static QueueHandle_t _core1_init_done_q;

void _init_core1(void *unused) {
    st3m_usb_init();
    st3m_console_init();
    st3m_usb_app_conf_t app = {
        .fn_rx = st3m_console_cdc_on_rx,
        .fn_txpoll = st3m_console_cdc_on_txpoll,
        .fn_detach = st3m_console_cdc_on_detach,
    };
    st3m_usb_mode_t usb_mode = {
        .kind = st3m_usb_mode_kind_app,
        .app = &app,
    };
    st3m_usb_mode_switch(&usb_mode);
    puts(" ___ _           ___     _         _");
    puts("|  _| |___ _ _ _|_  |___| |_ ___ _| |___ ___");
    puts("|  _| | . | | | |_  |  _| . | .'| . | . | -_|");
    puts("|_| |_|___|_____|___|_| |___|__,|___|_  |___|");
    puts("                                    |___|");

    // Load bearing delay. USB crashes otherwise?
    // TODO(q3k): debug this
    vTaskDelay(100 / portTICK_PERIOD_MS);

    flow3r_bsp_i2c_init();

    flow3r_fs_init();
    st3m_leds_init();
    st3m_io_init();
    st3m_captouch_init();
    st3m_imu_init();
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // Let the main startup code know that core1 init is done. Then sleep
    // forever. What a waste of stack space.
    bool done = true;
    xQueueSend(_core1_init_done_q, &done, portMAX_DELAY);
    for (;;) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

// Called by micropython via MICROPY_BOARD_STARTUP.
void flow3r_startup(void) {
    _core1_init_done_q = xQueueCreate(1, sizeof(bool));

    // Initialize display first as that gives us a nice splash screen.
    st3m_gfx_init();
    // Submit splash a couple of times to make sure we've fully flushed out the
    // initial framebuffer (both on-ESP and in-screen) noise before we turn on
    // the backlight.
    for (int i = 0; i < 4; i++) {
        st3m_gfx_splash("");
    }
    // Display should've flushed by now. Turn on backlight.
    flow3r_bsp_display_set_backlight(100);

    st3m_mode_init();
    st3m_mode_set(st3m_mode_kind_starting, "core1");
    st3m_mode_update_display(NULL);

    xTaskCreatePinnedToCore(_init_core1, "core1-init", 4096, NULL,
                            configMAX_PRIORITIES - 1, NULL, 1);

    bool done;
    xQueueReceive(_core1_init_done_q, &done, portMAX_DELAY);

    st3m_mode_set(st3m_mode_kind_starting, "core0");
    st3m_mode_update_display(NULL);

    st3m_scope_init();
    st3m_audio_init();
    bl00mbox_init();

    st3m_mode_set(st3m_mode_kind_starting, "micropython");
    st3m_mode_update_display(NULL);
}
