#include "bl00mbox.h"
#include "flow3r_bsp.h"
#include "st3m_audio.h"
#include "st3m_console.h"
#include "st3m_fs.h"
#include "st3m_gfx.h"
#include "st3m_io.h"
#include "st3m_leds.h"
#include "st3m_mode.h"
#include "st3m_scope.h"
#include "st3m_usb.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "st3m-board-startup";

// Called by micropython via MICROPY_BOARD_STARTUP.
void flow3r_main(void) {
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
    st3m_mode_set(st3m_mode_kind_starting, "st3m");
    st3m_mode_update_display(NULL);

    // Initialize USB and console so that we get logging as early as possible.
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
    vTaskDelay(100/portTICK_PERIOD_MS);


    flow3r_bsp_i2c_init();
    st3m_mode_set(st3m_mode_kind_starting, "fs");
    st3m_mode_update_display(NULL);
    st3m_fs_init();
    st3m_mode_set(st3m_mode_kind_starting, "audio");
    st3m_mode_update_display(NULL);
    st3m_scope_init();
    st3m_audio_init();
    st3m_audio_set_player_function(bl00mbox_player_function);
    st3m_mode_set(st3m_mode_kind_starting, "io");
    st3m_mode_update_display(NULL);
    st3m_leds_init();
    st3m_io_init();

    st3m_mode_set(st3m_mode_kind_starting, "micropython");
    st3m_mode_update_display(NULL);
}
