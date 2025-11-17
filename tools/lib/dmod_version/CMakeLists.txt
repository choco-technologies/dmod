set(MODULE_NAME dmod_version)

# Create static library for version utilities
add_library(${MODULE_NAME} STATIC
    dmod_version.c
)

target_include_directories(${MODULE_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link to dmod_inc for SAL API
target_link_libraries(${MODULE_NAME}
    PUBLIC
        dmod_inc
        dmod_common
        dmod_system
)

# Install headers
install(FILES dmod_version.h
    DESTINATION include/dmod
)

# Create Makefile for Make build system
create_library_makefile(${MODULE_NAME})
