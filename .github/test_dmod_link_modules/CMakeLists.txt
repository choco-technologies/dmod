cmake_minimum_required(VERSION 3.18)

# This is a test project to verify dmod_link_modules functionality
project(test_dmod_link_modules)

# Set the DMOD directory
set(DMOD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# Include DMOD paths
include(${DMOD_DIR}/paths.cmake)

# Setup DMOD for external module
dmod_setup_external_module()

# Set module properties
set(DMOD_MODULE_NAME        test_link)
set(DMOD_MODULE_VERSION     "0.1")
set(DMOD_AUTHOR_NAME        "CI Test")
set(DMOD_STACK_SIZE         1024)
set(DMOD_PRIORITY           0)

# Create the test module
dmod_add_executable(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION} 
    main.c
)

# Test dmod_link_modules with dmini module
# This should download dmini headers and add them to include directories
dmod_link_modules(${DMOD_MODULE_NAME}
    dmini
)

# Verify that the headers were downloaded by checking if the target has the include directory
get_target_property(INCLUDE_DIRS ${DMOD_MODULE_NAME} INCLUDE_DIRECTORIES)
message(STATUS "Include directories for ${DMOD_MODULE_NAME}: ${INCLUDE_DIRS}")

# Check if dmini include directory is in the list
set(DMINI_INC_DIR "${DMOD_DMF_DIR}/dmini/inc")
if("${INCLUDE_DIRS}" MATCHES "${DMINI_INC_DIR}")
    message(STATUS "SUCCESS: dmini include directory found in target includes")
else()
    message(WARNING "dmini include directory not found in target includes")
endif()
