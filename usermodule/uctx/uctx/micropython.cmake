add_library(usermod_ctx INTERFACE)

target_sources(usermod_ctx INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/uctx.c
)

target_include_directories(usermod_ctx INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../fonts
)

target_link_libraries(usermod INTERFACE usermod_ctx)
