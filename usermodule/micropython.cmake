# If you want to add dependencies on other ESP-IDF modules, add them to
# IDF_COMPONENTS in main/CMakeLists, as main/ is the component that the
# usermodules are actually part of.

add_library(usermod_badge23 INTERFACE)

target_sources(usermod_badge23 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/mp_hardware.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_synth.c
    ${CMAKE_CURRENT_LIST_DIR}/mp_kernel.c
)

target_include_directories(usermod_badge23 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(usermod INTERFACE usermod_badge23)



#Also add uctx as a usermodule
include(${CMAKE_CURRENT_LIST_DIR}/uctx/uctx/micropython.cmake)
