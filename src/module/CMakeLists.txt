set(MODULE_NAME dmod_module)
set(DMOD_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "Path to the module header template")

add_library(${MODULE_NAME} STATIC
    dmod_module.c
    string.c
    signal.c
)

# Module targets default to -O2 even when no CMake build type is selected.
# Apply that default to their runtime too, notably memcpy/memset in string.c.
# Named and multi-config builds retain their configured optimization flags.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    target_compile_options(${MODULE_NAME} PRIVATE -O2)
endif()

target_include_directories(${MODULE_NAME} 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
)   

# Link libraries if needed
target_link_libraries(${MODULE_NAME} 
    PRIVATE    
        dmod_inc
    )

create_library_makefile(${MODULE_NAME})
