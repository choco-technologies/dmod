cmake_minimum_required(VERSION 3.18)

# Check if the difs module has been built (difs_defs.h exists)
set(DIFS_DEFS_HEADER "${CMAKE_BINARY_DIR}/examples/dif/difs/difs_defs.h")

if(NOT EXISTS "${DIFS_DEFS_HEADER}")
    message(WARNING "Skipping dif_test and test_direct_access: difs module not built yet. "
                    "Build with DMOD_MODE=DMOD_MODULE first to generate required files.")
    return()
endif()

dmod_add_tool(dif_test main.c)

# Add include directories for difs.h and difs_defs.h
target_include_directories(dif_test PRIVATE 
    ${DMOD_DIR}/examples/dif/difs
    ${CMAKE_BINARY_DIR}/examples/dif/difs
)

# Add test_direct_access tool
dmod_add_tool(test_direct_access test_direct_access.c)

# Add include directories for test_direct_access as well
target_include_directories(test_direct_access PRIVATE 
    ${DMOD_DIR}/examples/dif/difs
    ${CMAKE_BINARY_DIR}/examples/dif/difs
)
