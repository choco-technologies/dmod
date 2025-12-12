cmake_minimum_required(VERSION 3.18)

# This is a test to verify version requirements are transferred to .dmd files
project(test_version_requirements)

# Set the DMOD directory
if(NOT DEFINED DMOD_DIR)
    set(DMOD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
endif()

# Include DMOD paths
include(${DMOD_DIR}/paths.cmake)

# Setup DMOD for external module
dmod_setup_external_module()

# Set module properties
set(DMOD_MODULE_NAME        test_version_reqs)
set(DMOD_MODULE_VERSION     "0.1")
set(DMOD_AUTHOR_NAME        "CI Test")
set(DMOD_STACK_SIZE         1024)
set(DMOD_PRIORITY           0)

# Create the test module
dmod_add_executable(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION} 
    main.c
)

# Test dmod_link_modules with version specifications
# This should save version requirements and pass them to todmd
dmod_link_modules(${DMOD_MODULE_NAME}
    dmini@1.0
    dmodex@0.1
)

# Verify that the version requirements file was created
get_target_property(VERSION_REQS_FILE ${DMOD_MODULE_NAME} DMOD_VERSION_REQUIREMENTS_FILE)
if(VERSION_REQS_FILE)
    message(STATUS "Version requirements file: ${VERSION_REQS_FILE}")
else()
    message(WARNING "Version requirements file property not set!")
endif()
