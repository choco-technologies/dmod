set(DMOD_MODULE_NAME        dmod_example)
set(DMOD_MODULE_VERSION     "1.0")
set(DMOD_AUTHOR_NAME        "Patryk Kubiak")

configure_file(${DMOD_HEADER_TEMPLATE_PATH} ${CMAKE_CURRENT_BINARY_DIR}/dmod_header.c)

# Add the executable
add_executable(${DMOD_MODULE_NAME} main.c dmod_header.c)

# Include directories
include_directories(${PROJECT_SOURCE_DIR})

target_link_libraries(${DMOD_MODULE_NAME} dmod)

target_compile_options(${DMOD_MODULE_NAME} PRIVATE -fPIC -fPIE -ffunction-sections -fdata-sections )

target_link_options(${DMOD_MODULE_NAME} PRIVATE -L ${DMOD_PROJECT_DIR}/scripts -pie -nostartfiles -nostdlib -Xlinker --discard-all )
target_link_options(${DMOD_MODULE_NAME} PRIVATE -T ${DMOD_PROJECT_DIR}/scripts/module.ld  -static)

set(ELF_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${DMOD_MODULE_NAME}")
set(BIN_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${DMOD_MODULE_NAME}.dmf")

add_custom_command(
    TARGET ${DMOD_MODULE_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${ELF_OUTPUT} ${BIN_OUTPUT}
    COMMENT "Converting ELF to binary"
)