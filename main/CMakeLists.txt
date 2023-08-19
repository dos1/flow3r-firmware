set(SYS_C_PATH "${CMAKE_CURRENT_BINARY_DIR}/include/sys_data.c")

if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/include/")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/")
endif()

idf_component_register(
    SRCS
        main.c
        fs.c
        "${SYS_C_PATH}"
)
idf_component_get_property(freertos_lib freertos COMPONENT_LIB)
target_compile_options(${freertos_lib} PRIVATE -Wno-format)

# Generate sys_data.c from python_payload.
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
add_custom_target(generate_sys_c ALL DEPENDS "${SYS_C_PATH}")

idf_component_get_property(main_lib main COMPONENT_LIB)
add_dependencies("${main_lib}" generate_sys_c)
