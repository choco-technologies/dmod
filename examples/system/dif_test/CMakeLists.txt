cmake_minimum_required(VERSION 3.18)

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
