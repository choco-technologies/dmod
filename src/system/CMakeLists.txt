set(MODULE_NAME dmod_system)

add_library(${MODULE_NAME} STATIC
    dmod_system.c
    dmod_if_file.c
    dmod_if_mem.c
    dmod_if_dbg.c
    dmod_if_irq.c
    dmod_if_events.c
    dmod_if_rtos.c
)

target_include_directories(${MODULE_NAME} 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
)   

# Link libraries if needed
target_link_libraries(${MODULE_NAME} 
    PRIVATE    
        dmod_inc
        dmod_common
    )