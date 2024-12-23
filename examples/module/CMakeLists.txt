cmake_minimum_required(VERSION 3.10)

# Set the project name
project(dmod_example VERSION 1.0)

# Add the executable
add_executable(${PROJECT_NAME} main.c)

# Include directories
include_directories(${PROJECT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} dmod)

target_compile_options(${PROJECT_NAME} PRIVATE -fPIC -fPIE -ffunction-sections -fdata-sections)

target_link_options(${PROJECT_NAME} PRIVATE -L ${DMOD_PROJECT_DIR}/scripts -pie -nostartfiles -nostdlib -Xlinker --discard-all )
target_link_options(${PROJECT_NAME} PRIVATE -T ${DMOD_PROJECT_DIR}/scripts/module.ld  -static)

set(ELF_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}")
set(BIN_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.dmf")

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${ELF_OUTPUT} ${BIN_OUTPUT}
    COMMENT "Converting ELF to binary"
)