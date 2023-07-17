set(SYS_C_PATH "${CMAKE_CURRENT_BINARY_DIR}/include/st3m_sys_data.c")

idf_component_register(
    SRCS
        "main.c"
        "st3m_fs.c"
        "st3m_mode.c"
        "st3m_tar.c"
        "${SYS_C_PATH}"
    INCLUDE_DIRS
        .
)

# Generate st3m_sys_data.c from python_payload.
idf_build_get_property(python PYTHON)
file(GLOB_RECURSE payload_files "${PROJECT_DIR}/python_payload_factory/*.py")
add_custom_command(
    OUTPUT
        "${SYS_C_PATH}"
    COMMAND
        "${python}" "${CMAKE_CURRENT_SOURCE_DIR}/host-tools/pack-sys.py" "${PROJECT_DIR}/python_payload_factory" "${SYS_C_PATH}"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/host-tools/pack-sys.py" 
        "${payload_files}"
        
)
add_custom_target(generate_st3m_sys_c ALL DEPENDS "${SYS_C_PATH}")

idf_component_get_property(st3m_lib main COMPONENT_LIB)
add_dependencies("${st3m_lib}" generate_st3m_sys_c)
