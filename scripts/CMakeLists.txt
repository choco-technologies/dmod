#
#   Creates makefile for the given library.
#
function( create_library_makefile targetName )
    # Configure the Makefile
    if(DMOD_MODE STREQUAL "DMOD_SYSTEM")
        set(DMOD_SYSTEM ON)
        set(DMOD_MODULE OFF)
    elseif(DMOD_MODE STREQUAL "DMOD_MODULE")
        set(DMOD_SYSTEM OFF)
        set(DMOD_MODULE ON)
    else()
        set(DMOD_SYSTEM OFF)
        set(DMOD_MODULE OFF)
    endif()
    file(RELATIVE_PATH DMOD_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}")
    set(DMOD_LIB_NAME ${targetName})
    file(RELATIVE_PATH DMOD_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
    set(DMOD_OUTPUT_DIR ${DMOD_OUTPUT_DIR})
    set(DMOD_LIB_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR})
    file(RELATIVE_PATH DMOD_LIB_SRC_PATH "${CMAKE_SOURCE_DIR}" "${DMOD_LIB_SRC_PATH}")
    set(DMOD_LIBS_DIR ${CMAKE_BINARY_DIR}/libs)
    file(RELATIVE_PATH DMOD_LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}" "${DMOD_LIBS_DIR}")
    get_target_property(DMOD_SOURCES ${targetName} SOURCES)
    get_target_property(DMOD_INC_DIRS ${targetName} INCLUDE_DIRECTORIES)
    get_target_property(DMOD_LINKED_LIBRARIES ${targetName} LINK_LIBRARIES)
    get_target_property(DMOD_COMPILE_DEFINITIONS ${targetName} COMPILE_DEFINITIONS)
    foreach(LIB ${DMOD_LINKED_LIBRARIES})
        message(STATUS "LIB: ${LIB}")
        
        get_target_property(LIB_INC_DIRS ${LIB} INTERFACE_INCLUDE_DIRECTORIES)
        if(LIB_INC_DIRS)
            foreach(LIB_INC_DIR ${LIB_INC_DIRS})
                message(STATUS "LIB_INC_DIR: ${LIB_INC_DIR}")
                list(APPEND DMOD_INC_DIRS ${LIB_INC_DIR})
            endforeach()
        endif()
    endforeach()

    set(DMOD_DEFINITIONS "")
    if(DMOD_COMPILE_DEFINITIONS)
        foreach(DEFINITION ${DMOD_COMPILE_DEFINITIONS})
            message(STATUS "DEFINITION: ${DEFINITION}")
            list(APPEND DMOD_DEFINITIONS ${DEFINITION})
        endforeach()
    endif()

    set(DMOD_RELATIVE_HEADER_PATHS "")
    foreach(HEADER_PATH ${DMOD_INC_DIRS})
        message(STATUS "HEADER_PATH: ${HEADER_PATH}")
        file(RELATIVE_PATH RELATIVE_PATH "${CMAKE_SOURCE_DIR}" "${HEADER_PATH}")
        list(APPEND DMOD_RELATIVE_HEADER_PATHS "${DMOD_PROJECT_DIR}/${RELATIVE_PATH}")
    endforeach()
    string(REPLACE ";" "\\\n\t\t" DMOD_SOURCES "${DMOD_SOURCES}")
    string(REPLACE ";" "\\\n\t\t" DMOD_INC_DIRS "${DMOD_RELATIVE_HEADER_PATHS}")
    string(REPLACE ";" "\\\n\t\t" DMOD_LIBS "${DMOD_LINKED_LIBRARIES}")
    configure_file(${CMAKE_SOURCE_DIR}/scripts/Makefile-lib.in ${CMAKE_CURRENT_SOURCE_DIR}/Makefile)
endfunction()


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