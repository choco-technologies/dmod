cmake_minimum_required(VERSION 3.18)

set(DMOD_MODULE_NAME difs)
set(DMOD_MODULE_VERSION "1.0")
set(DMOD_AUTHOR_NAME "DMOD Team")
set(DMOD_STACK_SIZE 1024)
set(DMOD_PRIORITY 1)
set(DMOD_MANUAL_LOAD OFF)

dmod_add_library(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION}
    difs.c
)

# Make the interface available to other modules
target_include_directories(${DMOD_MODULE_NAME}_if 
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
