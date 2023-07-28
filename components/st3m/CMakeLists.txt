idf_component_register(
    SRCS
        st3m_audio.c
        st3m_board_startup.c
        st3m_gfx.c
        st3m_counter.c
        st3m_fs.c
        st3m_scope.c
        st3m_leds.c
        st3m_colors.c
        st3m_io.c
        st3m_usb_cdc.c
        st3m_usb_descriptors.c
        st3m_usb_msc.c
        st3m_usb.c
        st3m_console.c
        st3m_mode.c
        st3m_captouch.c
        st3m_ringbuffer.c
    INCLUDE_DIRS
        .
    REQUIRES
        flow3r_bsp
        bl00mbox
        ctx
        fatfs
        tinyusb
        esp_ringbuf
        usb
)

idf_component_get_property(tusb_lib tinyusb COMPONENT_LIB)
target_include_directories(${tusb_lib} PRIVATE .)