# If you want to add dependencies on other ESP-IDF modules, add them to
# IDF_COMPONENTS in components/micropython/CMakeLists.txt, that is the component
# that the usermodules are actually part of.

add_library(usermod_badge23 INTERFACE)

target_sources(usermod_badge23 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/mp_audio.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_badgelink.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_badgenet.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_captouch.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_imu.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_leds.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_sys_bl00mbox.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_sys_buttons.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_sys_display.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_sys_kernel.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_uctx.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_media.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_sys_mode.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_sys_colors.c
)

target_include_directories(usermod_badge23 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(usermod INTERFACE usermod_badge23)
