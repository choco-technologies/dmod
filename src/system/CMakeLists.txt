set(MODULE_NAME dmod_system)
set(DMOD_MODE "DMOD_SYSTEM")

# Prepare source list
set(SYSTEM_SOURCES
    private/dmod_vars.c
    private/dmod_ctx.c
    private/dmod_hlp.c
    private/dmod_ldr.c
    private/dmod_mgr.c
    private/dmod_rmod.c
    private/dmod_pck.c
    public/dmod_dmf_cb.c
    public/dmod_dmf_api.c
    public/dmod_dmfc_api.c
    public/dmod_dmp_api.c
    dmod_system.c
    if/dmod_if_file.c
    if/dmod_if_mem.c
    if/dmod_if_rawmem.c
    if/dmod_if_dbg.c
    if/dmod_if_irq.c
    if/dmod_if_events.c
    if/dmod_if_rtos.c
    if/dmod_if_comp.c
    if/dmod_if_env.c
    if/dmod_if_str.c
    if/dmod_if_input.c
    Dmod.cpp
)

# Add custom printf implementation if enabled
if(DMOD_IMPLEMENT_PRINTF)
    list(APPEND SYSTEM_SOURCES if/dmod_prf.c)
endif()

# Add custom scanf implementation if enabled
if(DMOD_IMPLEMENT_SCANF)
    list(APPEND SYSTEM_SOURCES if/dmod_scf.c)
endif()

add_library(${MODULE_NAME} STATIC ${SYSTEM_SOURCES})

# Add DMOD_EXTERNAL_REGISTRATION definition when enabled
if(DMOD_EXTERNAL_REGISTRATION)
    target_compile_definitions(${MODULE_NAME} PRIVATE DMOD_EXTERNAL_REGISTRATION)
endif()

target_include_directories(${MODULE_NAME} 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
)   

# Link libraries if needed
target_link_libraries(${MODULE_NAME} 
    PUBLIC    
        dmod_inc
        dmod_common
        dmod_fastlz
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