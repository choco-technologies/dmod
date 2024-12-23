set(MODULE_NAME dmod_module)

set(DMOD_HEADER_TEMPLATE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dmod_header.c.in CACHE STRING "Path to the module header template")

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