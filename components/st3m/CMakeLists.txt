set(SYS_C_PATH "${CMAKE_CURRENT_BINARY_DIR}/include/st3m_sys_data.c")

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
        st3m_tar.c
        "${SYS_C_PATH}"
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

# Generate st3m_sys_data.c from python_payload.
idf_build_get_property(python PYTHON)
file(GLOB_RECURSE payload_files "${PROJECT_DIR}/python_payload/*.py")
add_custom_command(
    OUTPUT
        "${SYS_C_PATH}"
    COMMAND
        "${python}" "${CMAKE_CURRENT_SOURCE_DIR}/host-tools/pack-sys.py" "${PROJECT_DIR}/python_payload" "${SYS_C_PATH}"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/host-tools/pack-sys.py" 
        "${payload_files}"
        
)
add_custom_target(generate_st3m_sys_c ALL DEPENDS "${SYS_C_PATH}")

idf_component_get_property(st3m_lib st3m COMPONENT_LIB)
add_dependencies("${st3m_lib}" generate_st3m_sys_c)
