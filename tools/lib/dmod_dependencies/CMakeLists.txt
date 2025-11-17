# DMOD Dependencies Parser Library

# Create static library
add_library(dmod_dependencies STATIC
    dmod_dependencies.c
)

# Set include directories
target_include_directories(dmod_dependencies
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${DMOD_INC_DIRS}
        ${DMOD_SRC_COMMON_DIR}
        ${DMOD_SRC_SYSTEM_DIR}
)

# Link dependencies
target_link_libraries(dmod_dependencies
    PUBLIC
        dmod_version
    PRIVATE
        dmod_common
        dmod_system
)

# Set compile options
target_compile_options(dmod_dependencies PRIVATE
    -Wall
    -Wextra
)
