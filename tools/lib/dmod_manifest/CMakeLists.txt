set(MODULE_NAME dmod_manifest)

# Create static library for manifest parsing
add_library(${MODULE_NAME} STATIC
    dmod_manifest.c
)

target_include_directories(${MODULE_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link to dmod_inc for SAL API
target_link_libraries(${MODULE_NAME}
    PUBLIC
        dmod_inc
        dmod_common
)

# Install headers
install(FILES dmod_manifest.h
    DESTINATION include/dmod
)

# Create Makefile for Make build system
create_library_makefile(${MODULE_NAME})
