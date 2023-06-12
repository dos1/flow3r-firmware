# If you want to add dependencies on other ESP-IDF modules, add them to
# IDF_COMPONENTS in main/CMakeLists, as main/ is the component that the
# usermodules are actually part of.

add_library(usermod_badge23 INTERFACE)

target_sources(usermod_badge23 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/mp_hardware.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_leds.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_audio.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_badge_link.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_synth.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_kernel.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_uctx.c
)

target_include_directories(usermod_badge23 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(usermod INTERFACE usermod_badge23)