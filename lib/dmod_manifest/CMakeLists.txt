cmake_minimum_required(VERSION 3.18)

# Create static library for manifest parsing
add_library(dmod_manifest STATIC
    dmod_manifest.c
)

target_include_directories(dmod_manifest PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Install headers
install(FILES dmod_manifest.h
    DESTINATION include/dmod
)
