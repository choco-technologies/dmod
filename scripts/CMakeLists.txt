if(NOT DEFINED DMOD_SCRIPTS_DIR)
    set(DMOD_SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "Path to the scripts directory")
endif()

#
#   Creates makefile for the given library.
#
function( create_library_makefile targetName )
    # Check if DMOD_DIR is defined
    if(NOT DEFINED DMOD_DIR)
        message(FATAL_ERROR "DMOD_DIR is not defined. Please set DMOD_DIR variable before calling create_library_makefile function.")
    endif()
    
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
    file(RELATIVE_PATH DMOD_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" "${DMOD_DIR}")
    string(REPLACE "${DMOD_DIR}" "$(DMOD_DIR)" DMOD_PROJECT_DIR "${DMOD_PROJECT_DIR}")
    
    set(DMOD_LIB_NAME ${targetName})
    file(RELATIVE_PATH DMOD_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
    set(DMOD_OUTPUT_DIR ${DMOD_OUTPUT_DIR})
    
    set(DMOD_LIB_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR})
    string(REPLACE "${DMOD_DIR}" "$(DMOD_DIR)" DMOD_LIB_SRC_PATH "${DMOD_LIB_SRC_PATH}")
    
    set(DMOD_LIBS_DIR ${CMAKE_BINARY_DIR}/libs)
    string(REPLACE "${DMOD_BUILD_DIR}" "$(DMOD_BUILD_DIR)" DMOD_LIBS_DIR "${DMOD_LIBS_DIR}")
    
    get_target_property(DMOD_SOURCES ${targetName} SOURCES)
    get_target_property(DMOD_INC_DIRS ${targetName} INCLUDE_DIRECTORIES)
    get_target_property(DMOD_LINKED_LIBRARIES ${targetName} LINK_LIBRARIES)
    get_target_property(DMOD_COMPILE_DEFINITIONS ${targetName} COMPILE_DEFINITIONS)
    # Check if the library has any linked libraries
    if(NOT DMOD_LINKED_LIBRARIES)
        set(DMOD_LINKED_LIBRARIES "")
    endif()

    foreach(LIB ${DMOD_LINKED_LIBRARIES})
        message(STATUS "LIB: ${LIB}")
        
        # Check if LIB is a target
        if(TARGET ${LIB})
            get_target_property(LIB_INC_DIRS ${LIB} INTERFACE_INCLUDE_DIRECTORIES)
            if(LIB_INC_DIRS)
                foreach(LIB_INC_DIR ${LIB_INC_DIRS})
                    message(STATUS "LIB_INC_DIR: ${LIB_INC_DIR}")
                    list(APPEND DMOD_INC_DIRS ${LIB_INC_DIR})
                endforeach()
            endif()
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
        # Check if path is absolute
        if(IS_ABSOLUTE "${HEADER_PATH}")
            # First try to replace with dmod variables
            string(REPLACE "${DMOD_BUILD_DIR}" "$(DMOD_BUILD_DIR)" HEADER_PATH "${HEADER_PATH}")
            string(REPLACE "${DMOD_DIR}" "$(DMOD_DIR)" HEADER_PATH "${HEADER_PATH}")
            
            # If still absolute, convert to relative path relative to CMAKE_CURRENT_SOURCE_DIR
            if(IS_ABSOLUTE "${HEADER_PATH}")
                file(RELATIVE_PATH RELATIVE_HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${HEADER_PATH}")
                set(HEADER_PATH "${RELATIVE_HEADER_PATH}")
            endif()
        endif()
        
        message(STATUS "HEADER_PATH: ${HEADER_PATH}")
        list(APPEND DMOD_RELATIVE_HEADER_PATHS "${HEADER_PATH}")
    endforeach()
    string(REPLACE ";" "\\\n\t\t" DMOD_SOURCES "${DMOD_SOURCES}")
    string(REPLACE ";" "\\\n\t\t" DMOD_INC_DIRS "${DMOD_RELATIVE_HEADER_PATHS}")
    string(REPLACE ";" "\\\n\t\t" DMOD_LIBS "${DMOD_LINKED_LIBRARIES}")
    string(REPLACE "Threads::Threads" "pthread" DMOD_LIBS "${DMOD_LIBS}")
    configure_file(${DMOD_DIR}/scripts/Makefile-lib.in ${CMAKE_CURRENT_SOURCE_DIR}/Makefile)
endfunction()

#
#   Creates makefile for the given executable.
#   
#   This function generates a Makefile for an executable target using the Makefile-exe.in template.
#   It extracts build information from the CMake target including sources, include directories,
#   linked libraries, and compile definitions.
#
#   Parameters:
#       targetName - Name of the executable target (created with add_executable)
#
#   Example usage:
#       add_executable(my_app main.c)
#       target_link_libraries(my_app dmod)
#       create_executable_makefile(my_app)
#
function( create_executable_makefile targetName )
    # Check if DMOD_DIR is defined
    if(NOT DEFINED DMOD_DIR)
        message(FATAL_ERROR "DMOD_DIR is not defined. Please set DMOD_DIR variable before calling create_executable_makefile function.")
    endif()
    
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
    file(RELATIVE_PATH DMOD_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" "${DMOD_DIR}")
    string(REPLACE "${DMOD_DIR}" "$(DMOD_DIR)" DMOD_PROJECT_DIR "${DMOD_PROJECT_DIR}")
    
    set(DMOD_EXE_NAME ${targetName})
    # Use DMOD_BUILD_DIR variable instead of relative path
    set(DMOD_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    string(REPLACE "${DMOD_BUILD_DIR}" "$(DMOD_BUILD_DIR)" DMOD_OUTPUT_DIR "${DMOD_OUTPUT_DIR}")
    string(REPLACE "${DMOD_DIR}" "$(DMOD_DIR)" DMOD_OUTPUT_DIR "${DMOD_OUTPUT_DIR}")
    
    set(DMOD_EXE_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR})
    string(REPLACE "${DMOD_DIR}" "$(DMOD_DIR)" DMOD_EXE_SRC_PATH "${DMOD_EXE_SRC_PATH}")
    
    set(DMOD_LIBS_DIR ${CMAKE_BINARY_DIR}/libs)
    string(REPLACE "${DMOD_BUILD_DIR}" "$(DMOD_BUILD_DIR)" DMOD_LIBS_DIR "${DMOD_LIBS_DIR}")
    
    get_target_property(DMOD_SOURCES ${targetName} SOURCES)
    get_target_property(DMOD_INC_DIRS ${targetName} INCLUDE_DIRECTORIES)
    get_target_property(DMOD_LINKED_LIBRARIES ${targetName} LINK_LIBRARIES)
    get_target_property(DMOD_COMPILE_DEFINITIONS ${targetName} COMPILE_DEFINITIONS)
    
    # Initialize lists for collecting all libraries and include directories
    set(ALL_LIBS "")
    set(ALL_INC_DIRS "")
    
    # Check if the executable has any linked libraries
    if(NOT DMOD_LINKED_LIBRARIES)
        set(DMOD_LINKED_LIBRARIES "")
    endif()

    # Recursively collect libraries and include directories
    set(LIBS_TO_PROCESS ${DMOD_LINKED_LIBRARIES})
    set(PROCESSED_LIBS "")
    
    while(LIBS_TO_PROCESS)
        list(POP_FRONT LIBS_TO_PROCESS CURRENT_LIB)
        
        # Skip if already processed
        if(CURRENT_LIB IN_LIST PROCESSED_LIBS)
            continue()
        endif()
        list(APPEND PROCESSED_LIBS ${CURRENT_LIB})
        
        # Special case: if the library is "dmod" INTERFACE library and it's not a target yet
        # (due to forward reference), expand it manually to its constituent libraries
        if(CURRENT_LIB STREQUAL "dmod" AND NOT TARGET ${CURRENT_LIB})
            # Add the standard include directories
            list(APPEND ALL_INC_DIRS "${DMOD_DIR}/inc")
            list(APPEND ALL_INC_DIRS "${DMOD_BUILD_DIR}")
            
            # Add the constituent libraries that dmod links to
            list(APPEND ALL_LIBS "dmod_common")
            if(DMOD_USE_FASTLZ)
                list(APPEND ALL_LIBS "dmod_fastlz")
            endif()
            if(DMOD_SYSTEM)
                list(APPEND ALL_LIBS "dmod_system")
            elseif(DMOD_MODULE)
                list(APPEND ALL_LIBS "dmod_module")
            endif()
            list(APPEND ALL_LIBS "pthread")
            continue()
        endif()
        
        # Check if CURRENT_LIB is a target
        if(NOT TARGET ${CURRENT_LIB})
            # Not a target, add to libs as-is (e.g., system libraries like pthread)
            list(APPEND ALL_LIBS ${CURRENT_LIB})
            continue()
        endif()
        
        get_target_property(LIB_TYPE ${CURRENT_LIB} TYPE)
        if(LIB_TYPE)
            
            # Get include directories
            get_target_property(LIB_INC_DIRS ${CURRENT_LIB} INTERFACE_INCLUDE_DIRECTORIES)
            if(LIB_INC_DIRS)
                foreach(LIB_INC_DIR ${LIB_INC_DIRS})
                    if(NOT LIB_INC_DIR IN_LIST ALL_INC_DIRS)
                        list(APPEND ALL_INC_DIRS ${LIB_INC_DIR})
                    endif()
                endforeach()
            endif()
            
            # For INTERFACE libraries, recursively get their linked libraries
            if(LIB_TYPE STREQUAL "INTERFACE_LIBRARY")
                # Try both INTERFACE_LINK_LIBRARIES and LINK_LIBRARIES
                get_target_property(INTERFACE_LINK_LIBS ${CURRENT_LIB} INTERFACE_LINK_LIBRARIES)
                if(NOT INTERFACE_LINK_LIBS OR INTERFACE_LINK_LIBS STREQUAL "INTERFACE_LINK_LIBRARIES-NOTFOUND")
                    get_target_property(INTERFACE_LINK_LIBS ${CURRENT_LIB} LINK_LIBRARIES)
                endif()
                if(INTERFACE_LINK_LIBS AND NOT INTERFACE_LINK_LIBS STREQUAL "INTERFACE_LINK_LIBRARIES-NOTFOUND" AND NOT INTERFACE_LINK_LIBS STREQUAL "LINK_LIBRARIES-NOTFOUND")
                    foreach(INTERFACE_LIB ${INTERFACE_LINK_LIBS})
                        if(NOT INTERFACE_LIB IN_LIST PROCESSED_LIBS)
                            list(APPEND LIBS_TO_PROCESS ${INTERFACE_LIB})
                        endif()
                    endforeach()
                endif()
            else()
                # For non-interface libraries, add to the list
                list(APPEND ALL_LIBS ${CURRENT_LIB})
            endif()
        else()
            # Could not get type, add to libs as-is
            list(APPEND ALL_LIBS ${CURRENT_LIB})
        endif()
    endwhile()
    
    # Combine with target's own include directories
    if(DMOD_INC_DIRS)
        list(APPEND ALL_INC_DIRS ${DMOD_INC_DIRS})
    endif()

    set(DMOD_DEFINITIONS "")
    if(DMOD_COMPILE_DEFINITIONS)
        foreach(DEFINITION ${DMOD_COMPILE_DEFINITIONS})
            list(APPEND DMOD_DEFINITIONS ${DEFINITION})
        endforeach()
    endif()

    set(DMOD_RELATIVE_HEADER_PATHS "")
    foreach(HEADER_PATH ${ALL_INC_DIRS})
        # Check if path is absolute
        if(IS_ABSOLUTE "${HEADER_PATH}")
            # First try to replace with dmod variables
            string(REPLACE "${DMOD_BUILD_DIR}" "$(DMOD_BUILD_DIR)" HEADER_PATH "${HEADER_PATH}")
            string(REPLACE "${DMOD_DIR}" "$(DMOD_DIR)" HEADER_PATH "${HEADER_PATH}")
            
            # If still absolute, convert to relative path relative to CMAKE_CURRENT_SOURCE_DIR
            if(IS_ABSOLUTE "${HEADER_PATH}")
                file(RELATIVE_PATH RELATIVE_HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${HEADER_PATH}")
                set(HEADER_PATH "${RELATIVE_HEADER_PATH}")
            endif()
        endif()
        
        list(APPEND DMOD_RELATIVE_HEADER_PATHS "${HEADER_PATH}")
    endforeach()
    string(REPLACE ";" "\\\n\t\t" DMOD_SOURCES "${DMOD_SOURCES}")
    string(REPLACE ";" "\\\n\t\t" DMOD_INC_DIRS "${DMOD_RELATIVE_HEADER_PATHS}")
    string(REPLACE ";" "\\\n\t\t" DMOD_LIBS "${ALL_LIBS}")
    string(REPLACE "Threads::Threads" "pthread" DMOD_LIBS "${DMOD_LIBS}")
    configure_file(${DMOD_DIR}/scripts/Makefile-exe.in ${CMAKE_CURRENT_SOURCE_DIR}/Makefile)
endfunction()


function(to_snake_case INPUT_STRING OUTPUT_VARIABLE)
    # Replace hyphens with underscores
    string(REPLACE "-" "_" INTERMEDIATE_STRING "${INPUT_STRING}")
    
    # Replace spaces with underscores
    string(REPLACE " " "_" INTERMEDIATE_STRING "${INTERMEDIATE_STRING}")
    
    # Convert to lowercase
    string(TOLOWER "${INTERMEDIATE_STRING}" SNAKE_CASE_STRING)
    
    # Set the result to the output variable
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

    # check if todmfc tool is available
    find_program(TODMFC 
            todmfc
            HINTS ${DMOD_TOOLS_BIN_DIR}
            )
    if(NOT TODMFC)
        if(DEFINED DMOD_COMPRESSION_METHOD)
            message(WARNING "todmfc not found, compression method will be set to none")
        endif()
        set(DMOD_COMPRESSION_METHOD "none")
    else()
        message(STATUS "todmfc found: ${TODMFC}")
        if(NOT DEFINED DMOD_COMPRESSION_METHOD)
            set(DMOD_COMPRESSION_METHOD "fastlz")
        endif()
    endif()

    configure_file(${DMOD_SCRIPTS_DIR}/api.h.in ${CMAKE_CURRENT_BINARY_DIR}/${DMOD_MODULE_NAME_SNAKE_CASE}_defs.h)
    configure_file(${DMOD_MODULE_HEADER_SOURCE_IN_FILE_PATH} ${CMAKE_CURRENT_BINARY_DIR}/${DMOD_MODULE_NAME_SNAKE_CASE}_header.c)
    
    add_executable(${moduleName} 
        ${SOURCES}
        ${CMAKE_CURRENT_BINARY_DIR}/${DMOD_MODULE_NAME_SNAKE_CASE}_header.c
        )
    
    add_library(${moduleName}_if INTERFACE
        )
    
    # target_precompile_headers(${moduleName} 
    #     PRIVATE 
    #         ${CMAKE_CURRENT_BINARY_DIR}/${DMOD_MODULE_NAME_SNAKE_CASE}_defs.h
    #     )

    foreach(MAL ${DMOD_MAL_IMPLS})
        target_compile_definitions(${moduleName} 
            PRIVATE
                DMOD_MAL_${MAL}
                )
    endforeach()

    foreach(DIF ${DMOD_DIF_IMPLS})
        target_compile_definitions(${moduleName} 
            PRIVATE
                DMOD_DIF_${DIF}
                )
    endforeach()

    target_compile_definitions(${moduleName} 
        PUBLIC
            DMOD_${moduleName}
            DMOD_MODE="DMOD_MODULE"
            DMOD_MODULE_NAME="${moduleName}"
            DMOD_MODULE_VERSION="${version}"
            DMOD_MODULE=1
            DMOD_SYSTEM=0
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
    
    set(OPTIMIZATION "-O2")
    if(DMOD_USE_EXCEPTIONS)
        set(EXCEPTION_FLAGS "")
    else()
        set(EXCEPTION_FLAGS -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables)
    endif()

    target_compile_options(${moduleName} PRIVATE -fPIC -fPIE -ffunction-sections ${EXCEPTION_FLAGS} -fdata-sections ${OPTIMIZATION} -fno-stack-protector -fno-builtin )
    target_compile_options(${moduleName} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-fpermissive>
    )
    target_link_options(${moduleName} PRIVATE -L ${DMOD_SCRIPTS_DIR} -pie -nostartfiles -nostdlib -Xlinker --discard-all -Wl,--gc-sections,--undefined=ModuleHeader )
    target_link_options(${moduleName} PRIVATE -T ${DMOD_SCRIPTS_DIR}/module.ld  -static)
    
    set(ELF_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${moduleName}")
    set(DMF_OUTPUT "${DMOD_DMF_DIR}/${moduleName}.dmf")
    set(DMFC_OUTPUT "${DMOD_DMFC_DIR}/${moduleName}.dmfc")
    
    file(MAKE_DIRECTORY ${DMOD_DMF_DIR})
    file(MAKE_DIRECTORY ${DMOD_DMFC_DIR})

    add_custom_command(
        TARGET ${moduleName} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary ${ELF_OUTPUT} ${DMF_OUTPUT}
        COMMENT "Converting ELF to binary"
    )

    if(TODMFC)
        add_custom_command(
            TARGET ${moduleName} POST_BUILD
            COMMAND ${TODMFC} ${DMF_OUTPUT} ${DMFC_OUTPUT} ${DMOD_COMPRESSION_METHOD}
            COMMENT "Compressing the binary"
        )
    endif()

    # Print the source files
    message(STATUS "SOURCES for module ${moduleName}: ${SOURCES}")
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

#
# Function to create a DMOD system
#
function(dmod_add_tool toolName)
    set(SOURCES ${ARGN})

    add_executable(${toolName} ${SOURCES})

    target_include_directories(${toolName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(${toolName} dmod)
    target_link_options(${toolName} PRIVATE -L ${DMOD_SCRIPTS_DIR} -T ${DMOD_SCRIPTS_DIR}/main.ld)

    # Move the binary to the tools directory
    add_custom_command(
        TARGET ${toolName} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${toolName}> ${DMOD_TOOLS_BIN_DIR}/${toolName}
        COMMENT "Copying the binary to the tools directory"
    )
    
    # Generate Makefile for the tool (only for tools in the tools/ directory)
    string(FIND "${CMAKE_CURRENT_SOURCE_DIR}" "/tools/" TOOLS_DIR_POS)
    if(TOOLS_DIR_POS GREATER -1)
        create_executable_makefile(${toolName})
    endif()
endfunction()