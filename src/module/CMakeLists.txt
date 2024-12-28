set(MODULE_NAME dmod_module)
set(DMOD_MODE "DMOD_MODULE")
set(DMOD_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "Path to the module header template")

add_library(${MODULE_NAME} STATIC
    dmod_module.c
)

target_include_directories(${MODULE_NAME} 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
)   

# Link libraries if needed
target_link_libraries(${MODULE_NAME} 
    PRIVATE    
        dmod_inc
    )

target_compile_definitions(${MODULE_NAME} 
    PUBLIC
        DMOD_MODULE
    )

create_library_makefile(${MODULE_NAME})
