set(MODULE_NAME dmod_module)

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

function(dmod_add_executable moduleName version)    
    set(SOURCES ${ARGN})

    set(DMOD_MODULE_NAME ${moduleName})
    set(DMOD_MODULE_VERSION ${version})
    if(NOT DEFINED DMOD_AUTHOR_NAME)
        set(DMOD_AUTHOR_NAME "Unknown")
    endif()
    if(NOT DEFINED DMOD_STACK_SIZE)
        set(DMOD_STACK_SIZE 1024)
    endif()
    if(NOT DEFINED DMOD_PRIORITY)
        set(DMOD_PRIORITY 1)
    endif()
    set(DMOD_MODULE_TYPE Application)

    configure_file(${DMOD_HEADER_DIR}/dmod_mod_defs.h.in ${CMAKE_CURRENT_BINARY_DIR}/dmod_mod_defs.h)
    
    # Add the executable
    add_executable(${moduleName} 
        ${SOURCES}
        ${DMOD_HEADER_DIR}/dmod_header.c
    )

    target_include_directories(${moduleName} 
        PRIVATE 
            ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    # target_compile_definitions(${moduleName} PRIVATE 
    #     -DDMOD_MODULE_NAME="${DMOD_MODULE_NAME}"
    #     -DDMOD_MODULE_VERSION="${DMOD_MODULE_VERSION}"
    #     -DDMOD_AUTHOR_NAME="${DMOD_AUTHOR_NAME}"
    #     -DDMOD_MODULE_TYPE=${DMOD_MODULE_TYPE}
    #     -DDMOD_STACK_SIZE=${DMOD_STACK_SIZE}
    #     -DDMOD_PRIORITY=${DMOD_PRIORITY}
    #     )
    
    target_link_libraries(${moduleName} dmod)
    
    target_compile_options(${moduleName} PRIVATE -fPIC -fPIE -ffunction-sections -fdata-sections )
    
    target_link_options(${moduleName} PRIVATE -L ${DMOD_PROJECT_DIR}/scripts -pie -nostartfiles -nostdlib -Xlinker --discard-all )
    target_link_options(${moduleName} PRIVATE -T ${DMOD_PROJECT_DIR}/scripts/module.ld  -static)
    
    set(ELF_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${moduleName}")
    set(BIN_OUTPUT "${DMOD_DMF_DIR}/${moduleName}.dmf")
    
    file(MAKE_DIRECTORY ${DMOD_DMF_DIR})

    add_custom_command(
        TARGET ${moduleName} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary ${ELF_OUTPUT} ${BIN_OUTPUT}
        COMMENT "Converting ELF to binary"
    )
endfunction()