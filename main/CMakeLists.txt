# This copies micropython/ports/esp32/CMakeLists.txt but modifies it to remove
# the concept of 'boards' as it's not conducive to a sensible
# directory/project structure for our badge project:
#
# 1. We don't want to have stuff deeply inside micropython/ports/esp/boards.
#    Ideally, the micropython directory will be a submodule that's tracking
#    upstream without any modifications.
# 2. We don't want to have to call cmake with -DMICROPY_BOARD. Calling plain
#    `cmake` or `idf.py` should just work and do the thing we want.
# 3. We don't target multiple boards anyways, so all that board functionality
#    is just dead weight.

set(MICROPY_DIR "${PROJECT_DIR}/micropython")
set(MICROPY_PORT_DIR "${MICROPY_DIR}/ports/esp32")
set(MICROPY_QSTRDEFS_PORT "${MICROPY_PORT_DIR}/qstrdefsport.h")

include("${MICROPY_DIR}/py/py.cmake")

set(USER_C_MODULES "${PROJECT_DIR}/usermodule/micropython.cmake")

if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    # Enable extmod components that will be configured by extmod.cmake.
    # A board may also have enabled additional components.
    set(MICROPY_PY_BTREE ON)

    include(${MICROPY_DIR}/py/usermod.cmake)
    include(${MICROPY_DIR}/extmod/extmod.cmake)
endif()

set(MICROPY_SOURCE_SHARED
    ${MICROPY_DIR}/shared/readline/readline.c
    ${MICROPY_DIR}/shared/netutils/netutils.c
    ${MICROPY_DIR}/shared/timeutils/timeutils.c
    ${MICROPY_DIR}/shared/runtime/interrupt_char.c
    ${MICROPY_DIR}/shared/runtime/stdout_helpers.c
    ${MICROPY_DIR}/shared/runtime/sys_stdio_mphal.c
    ${MICROPY_DIR}/shared/runtime/pyexec.c
)

set(MICROPY_SOURCE_LIB
    ${MICROPY_DIR}/lib/littlefs/lfs1.c
    ${MICROPY_DIR}/lib/littlefs/lfs1_util.c
    ${MICROPY_DIR}/lib/littlefs/lfs2.c
    ${MICROPY_DIR}/lib/littlefs/lfs2_util.c
    ${MICROPY_DIR}/lib/mbedtls_errors/mp_mbedtls_errors.c
    ${MICROPY_DIR}/lib/oofatfs/ff.c
    ${MICROPY_DIR}/lib/oofatfs/ffunicode.c
)

set(MICROPY_SOURCE_DRIVERS
    ${MICROPY_DIR}/drivers/bus/softspi.c
    ${MICROPY_DIR}/drivers/dht/dht.c
)

set(MICROPY_SOURCE_PORT
    ${MICROPY_PORT_DIR}/main.c
    ${MICROPY_PORT_DIR}/uart.c
    ${MICROPY_PORT_DIR}/usb.c
    ${MICROPY_PORT_DIR}/usb_serial_jtag.c
    ${MICROPY_PORT_DIR}/gccollect.c
    ${MICROPY_PORT_DIR}/mphalport.c
    ${MICROPY_PORT_DIR}/fatfs_port.c
    ${MICROPY_PORT_DIR}/help.c
    ${MICROPY_PORT_DIR}/modutime.c
    ${MICROPY_PORT_DIR}/machine_bitstream.c
    ${MICROPY_PORT_DIR}/machine_timer.c
    ${MICROPY_PORT_DIR}/machine_pin.c
    ${MICROPY_PORT_DIR}/machine_touchpad.c
    ${MICROPY_PORT_DIR}/machine_adc.c
    ${MICROPY_PORT_DIR}/machine_adcblock.c
    ${MICROPY_PORT_DIR}/machine_dac.c
    ${MICROPY_PORT_DIR}/machine_i2c.c
    ${MICROPY_PORT_DIR}/machine_i2s.c
    ${MICROPY_PORT_DIR}/machine_uart.c
    ${MICROPY_PORT_DIR}/modmachine.c
    ${MICROPY_PORT_DIR}/network_common.c
    ${MICROPY_PORT_DIR}/network_lan.c
    ${MICROPY_PORT_DIR}/network_ppp.c
    ${MICROPY_PORT_DIR}/network_wlan.c
    ${MICROPY_PORT_DIR}/mpnimbleport.c
    ${MICROPY_PORT_DIR}/modsocket.c
    ${MICROPY_PORT_DIR}/modesp.c
    ${MICROPY_PORT_DIR}/esp32_nvs.c
    ${MICROPY_PORT_DIR}/esp32_partition.c
    ${MICROPY_PORT_DIR}/esp32_rmt.c
    ${MICROPY_PORT_DIR}/esp32_ulp.c
    ${MICROPY_PORT_DIR}/modesp32.c
    ${MICROPY_PORT_DIR}/machine_hw_spi.c
    ${MICROPY_PORT_DIR}/machine_wdt.c
    ${MICROPY_PORT_DIR}/mpthreadport.c
    ${MICROPY_PORT_DIR}/machine_rtc.c
    ${MICROPY_PORT_DIR}/machine_sdcard.c
)

set(MICROPY_SOURCE_QSTR
    "${MICROPY_SOURCE_PY}"
    "${MICROPY_SOURCE_EXTMOD}"
    "${MICROPY_SOURCE_USERMOD}"
    "${MICROPY_SOURCE_SHARED}"
    "${MICROPY_SOURCE_LIB}"
    "${MICROPY_SOURCE_PORT}"
)

set(IDF_COMPONENTS
    app_update
    badge23
    badge23_hwconfig
    bootloader_support
    bt
    driver
    esp_adc_cal
    esp_common
    esp_eth
    esp_event
    esp_hw_support
    esp_ringbuf
    esp_rom
    esp_pm
    esp_wifi
    esp_system
    esp_timer
    esp_netif
    esp32s3
    freertos
    hal
    heap
    log
    lwip
    mbedtls
    mdns
    newlib
    nvs_flash
    sdmmc
    soc
    spi_flash
    tcpip_adapter
    ulp
    vfs
    xtensa
)

set(MICROPY_FROZEN_MANIFEST ${PROJECT_DIR}/manifest.py)
set(MICROPY_CROSS_FLAGS -march=xtensawin)

idf_component_register(
    SRCS
        ${MICROPY_SOURCE_PY}
        ${MICROPY_SOURCE_EXTMOD}
        ${MICROPY_SOURCE_USERMOD}
        ${MICROPY_SOURCE_SHARED}
        ${MICROPY_SOURCE_LIB}
        ${MICROPY_SOURCE_DRIVERS}
        ${MICROPY_SOURCE_PORT}
    INCLUDE_DIRS
        # Actual micropython include paths.
        ${MICROPY_INC_CORE}
        ${MICROPY_INC_USERMOD}
        ${MICROPY_PORT_DIR}
        # Needed for genhdr/* which for some reason is placed directly into
        # the output dir. Gross.
        ${CMAKE_BINARY_DIR}
        # Needed for include/mpconfigboard.h.
        "include"
    REQUIRES
        ${IDF_COMPONENTS}
)

# micropython machinery uses this to inspect include directories and compile
# definitions. We're effectively looping back MICROPY_{SOURCE/INC} through
# COMPONENT_TARGET (as generated by idf_component_register) back into
# micropython.
set(MICROPY_TARGET ${COMPONENT_TARGET})

# Feed information from IDF component targets into micropy qstr machinery.
foreach(comp ${IDF_COMPONENTS})
    micropy_gather_target_properties(__idf_${comp})
endforeach()

# micropython/qstr wants to intern strings from nimble private headers. Tell
# them how to find them.
target_include_directories(${COMPONENT_TARGET} PUBLIC
    "${IDF_PATH}/components/bt/host/nimble/nimble"
)

target_compile_definitions(${COMPONENT_TARGET} PUBLIC
    # micropython includes FatFS which is configurable via its own
    # configuration file, which is expected to be dfined as FFCONF_H.
    # micropython also ships such a configuration file. I don't know why
    # micropython doesn't just hardcode this...
    FFCONF_H=\"${MICROPY_OOFATFS_DIR}/ffconf.h\"
)

include("${MICROPY_DIR}/py/mkrules.cmake")
