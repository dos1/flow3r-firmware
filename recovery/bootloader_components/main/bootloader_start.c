/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include "bootloader_common.h"
#include "bootloader_init.h"
#include "bootloader_utility.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "esp_rom_gpio.h"
#include "hal/gpio_ll.h"

static const char *TAG = "boot";

static int select_partition_number(bootloader_state_t *bs, bool want_recovery);

#define PIN_RIGHT_SHOULDER 3

/*
 * We arrive here after the ROM bootloader finished loading this second stage
 * bootloader from flash. The hardware is mostly uninitialized, flash cache is
 * down and the app CPU is in reset. We do have a stack, so we can do the
 * initialization in C.
 */
void __attribute__((noreturn)) call_start_cpu0(void) {
    // 1. Hardware initialization
    if (bootloader_init() != ESP_OK) {
        bootloader_reset();
    }

    esp_rom_gpio_pad_select_gpio(PIN_RIGHT_SHOULDER);
    PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[PIN_RIGHT_SHOULDER]);
    bool right_shoulder_pressed =
        gpio_ll_get_level(&GPIO, PIN_RIGHT_SHOULDER) == 0;
    esp_rom_printf("flow3r bootloader right shoulder: %d\n",
                   right_shoulder_pressed);

#ifdef CONFIG_BOOTLOADER_SKIP_VALIDATE_IN_DEEP_SLEEP
    // If this boot is a wake up from the deep sleep then go to the short way,
    // try to load the application which worked before deep sleep.
    // It skips a lot of checks due to it was done before (while first boot).
    bootloader_utility_load_boot_image_from_deep_sleep();
    // If it is not successful try to load an application as usual.
#endif

    // 2. Select the number of boot partition
    bootloader_state_t bs = { 0 };
    int boot_index = select_partition_number(&bs, right_shoulder_pressed);
    if (boot_index == INVALID_INDEX) {
        bootloader_reset();
    }

    esp_rom_printf("flow3r bootloader selected partition: %d\n", boot_index);

    // 3. Load the app image for booting
    bootloader_utility_load_boot_image(&bs, boot_index);
}

// Select the number of boot partition
static int select_partition_number(bootloader_state_t *bs, bool want_recovery) {
    if (!bootloader_utility_load_partition_table(bs)) {
        ESP_LOGE(TAG, "load partition table error!");
        return INVALID_INDEX;
    }
    esp_rom_printf(
        "flow3r bootloader partition state: ota_info %p, factory %p, ota[0] "
        "%p\n",
        bs->ota_info.offset, bs->factory.offset, bs->ota[0].offset);

    bool have_recovery = bs->factory.offset != 0;
    bool have_main = bs->ota[0].offset != 0;

    const int index_recovery = -1;  // FACTORY
    const int index_main = 0;       // OTA[0]

    if (want_recovery && have_recovery) {
        return index_recovery;
    }
    if (have_main) {
        return index_main;
    }
    return index_recovery;
}

// Return global reent struct if any newlib functions are linked to bootloader
struct _reent *__getreent(void) { return _GLOBAL_REENT; }
