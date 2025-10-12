cmake_minimum_required(VERSION 3.18)

set(DMOD_MODULE_NAME vfs)
set(DMOD_MODULE_VERSION "1.0")
set(DMOD_AUTHOR_NAME "DMOD Team")
set(DMOD_STACK_SIZE 2048)
set(DMOD_PRIORITY 1)
set(DMOD_MANUAL_LOAD OFF)

dmod_add_library(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION}
    vfs.c
)

# Link to DIFS interface
target_link_libraries(${DMOD_MODULE_NAME} difs_if)
