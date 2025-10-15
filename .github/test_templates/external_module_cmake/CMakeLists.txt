cmake_minimum_required(VERSION 3.18)

set(DMOD_DIR "@DMOD_DIR@")
set(DMOD_MODE "DMOD_MODULE" CACHE STRING "Mode" FORCE)
set(DMOD_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}")

include(${DMOD_DIR}/paths.cmake)

project(external_test_module)

dmod_setup_external_module()

set(DMOD_MODULE_NAME external_test)
set(DMOD_MODULE_VERSION "1.0")
set(DMOD_AUTHOR_NAME "CI Test")

dmod_add_executable(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION}
    external_test.c
)
