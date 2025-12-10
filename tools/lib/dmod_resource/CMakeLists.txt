cmake_minimum_required(VERSION 3.10)

project(dmod_resource)

# Source files
set(SOURCES
    dmod_resource.c
)

# Header files
set(HEADERS
    dmod_resource.h
)

# Create static library
add_library(dmod_resource STATIC ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(dmod_resource PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/inc
)

# Link to dmod library
target_link_libraries(dmod_resource dmod)

# Installation
install(TARGETS dmod_resource
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${HEADERS}
    DESTINATION include
)
