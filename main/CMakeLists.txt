set(IDF_COMPONENTS
    badge23_bsp
    bootloader_support
    doomgeneric
    driver
    esp_common
    esp_event
    esp_hw_support
    esp_ringbuf
    esp_rom
    esp_pm
    esp_system
    esp_timer
    esp32s3
    freertos
    hal
    heap
    log
    newlib
    nvs_flash
    sdmmc
    spiffs
    soc
    spi_flash
    ulp
    vfs
    xtensa
)

idf_component_register(
    SRCS
        main.c
    INCLUDE_DIRS
        "include"
    REQUIRES
        ${IDF_COMPONENTS}
)

#spiffs_create_partition_image(spiffs ../spiffs FLASH_IN_PROJECT)
get_filename_component(image_path "${PROJECT_DIR}/doom1.wad" ABSOLUTE)
idf_component_get_property(main_args esptool_py FLASH_ARGS)
idf_component_get_property(sub_args esptool_py FLASH_SUB_ARGS)
esptool_py_flash_target(wad-flash "${main_args}" "${sub_args}" ALWAYS_PLAINTEXT)
esptool_py_flash_to_partition(wad-flash "wad" "${image_path}")
#esptool_py_flash_to_partition(flash "wad" "${image_path}")

