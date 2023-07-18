
idf_component_register(
    SRCS
        st3m_audio.c
        st3m_gfx.c
        st3m_counter.c
        st3m_scope.c
        st3m_leds.c
        st3m_colors.c
        st3m_io.c
        st3m_usb_cdc.c
        st3m_usb_descriptors.c
        st3m_usb_msc.c
        st3m_usb.c
        st3m_console.c
    INCLUDE_DIRS
        .
    REQUIRES
        flow3r_bsp
        ctx
        fatfs
        tinyusb
        esp_ringbuf
        usb
        esp_timer
        badge23
)

idf_component_get_property(tusb_lib tinyusb COMPONENT_LIB)
target_include_directories(${tusb_lib} PRIVATE .)
