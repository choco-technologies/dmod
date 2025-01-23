set(MODULE_NAME dmod_system)
set(DMOD_MODE "DMOD_SYSTEM")
add_library(${MODULE_NAME} STATIC
    dmod_vars.c
    dmod_ctx.c
    dmod_hlp.c
    dmod_ldr.c
    dmod_mgr.c
    dmod_system.c
    if/dmod_if_file.c
    if/dmod_if_mem.c
    if/dmod_if_dbg.c
    if/dmod_if_irq.c
    if/dmod_if_events.c
    if/dmod_if_rtos.c
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