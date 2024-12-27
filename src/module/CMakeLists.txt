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

create_library_makefile(${MODULE_NAME})

function(to_snake_case INPUT_STRING OUTPUT_VARIABLE)
    # Zamiana myślników na podkreślenia
    string(REPLACE "-" "_" INTERMEDIATE_STRING "${INPUT_STRING}")
    
    # Zamiana spacji na podkreślenia
    string(REPLACE " " "_" INTERMEDIATE_STRING "${INTERMEDIATE_STRING}")
    
    # Konwersja na małe litery
    string(TOLOWER "${INTERMEDIATE_STRING}" SNAKE_CASE_STRING)
    
    # Przypisanie wyniku do zmiennej wyjściowej
    set(${OUTPUT_VARIABLE} "${SNAKE_CASE_STRING}" PARENT_SCOPE)
endfunction()


#
# Function to create a DMOD module
#
function(dmod_create_module moduleName version moduleType)    
    set(SOURCES ${ARGN})

    set(DMOD_MODULE_NAME ${moduleName})
    to_snake_case(${moduleName} DMOD_MODULE_NAME_SNAKE_CASE)
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
    if(NOT DEFINED DMOD_MANUAL_LOAD)
        set(DMOD_MANUAL_LOAD OFF)
    endif()
    set(DMOD_MODULE_TYPE ${moduleType})

    configure_file(${DMOD_HEADER_DIR}/dmod_mod_defs.h.in ${CMAKE_CURRENT_BINARY_DIR}/${DMOD_MODULE_NAME_SNAKE_CASE}_defs.h)
    
    add_executable(${moduleName} 
        ${SOURCES}
        ${DMOD_HEADER_DIR}/dmod_header.c
        )
    
    add_library(${moduleName}_if INTERFACE
        )
    
    target_precompile_headers(${moduleName} 
        PRIVATE 
            ${CMAKE_CURRENT_BINARY_DIR}/${DMOD_MODULE_NAME_SNAKE_CASE}_defs.h
        )
    target_compile_definitions(${moduleName} 
        PUBLIC
            DMOD_MODULE_NAME="${DMOD_MODULE_NAME}"
            DMOD_${moduleName}
        )

    target_include_directories(${moduleName} 
        PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    target_include_directories(${moduleName}_if 
        INTERFACE
            ${CMAKE_CURRENT_BINARY_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}
    )

    
    target_link_libraries(${moduleName} dmod ${moduleName}_if)
    
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

#
# Function to create a DMOD executable module
#
function(dmod_add_executable moduleName version)    
    set(SOURCES ${ARGN})

    dmod_create_module(${moduleName} ${version} Application ${SOURCES})
endfunction()

#
# Function to create a DMOD library module
#
function(dmod_add_library moduleName version)    
    set(SOURCES ${ARGN})

    dmod_create_module(${moduleName} ${version} Library ${SOURCES})
endfunction()