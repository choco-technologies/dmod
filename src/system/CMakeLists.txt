set(MODULE_NAME dmod_system)
set(DMOD_MODE "DMOD_SYSTEM")
add_library(${MODULE_NAME} STATIC
    dmod_system.c
    dmod_if_file.c
    dmod_if_mem.c
    dmod_if_dbg.c
    dmod_if_irq.c
    dmod_if_events.c
    dmod_if_rtos.c
    Dmod.cpp
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

if (DMOD_USE_PTHREAD)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(${MODULE_NAME} 
        PRIVATE
        Threads::Threads
    )
endif()

create_library_makefile(${MODULE_NAME})