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

    # check if todmd tool is available
    find_program(TODMD 
            todmd
            HINTS ${DMOD_TOOLS_BIN_DIR}
            )

    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE DMOD_MODULE_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    configure_file(${DMOD_VERSION_IN_FILE_PATH} ${DMOD_DMF_DIR}/${DMOD_MODULE_NAME_SNAKE_CASE}_version.txt @ONLY)
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

    set(DMD_OUTPUT "${DMOD_DMF_DIR}/${moduleName}.dmd")
    if(TODMD)
        add_custom_command(
            TARGET ${moduleName} POST_BUILD
            COMMAND ${TODMD} ${DMF_OUTPUT} ${DMD_OUTPUT}
            COMMENT "Generating dependencies file"
        )
    endif()

    # Print the source files
    message(STATUS "SOURCES for module ${moduleName}: ${SOURCES}")

    # ===========================================================================
    # Install target for modules
    # ===========================================================================
    # Determine installation directory for module files
    # Priority: Environment variable > Configuration variable > DMF_DIR default
    if(DEFINED ENV{DMOD_REPO_DIR})
        set(INSTALL_REPO_DIR $ENV{DMOD_REPO_DIR})
        message(STATUS "Module ${moduleName}: Will install to DMOD_REPO_DIR from environment: ${INSTALL_REPO_DIR}")
    elseif(DEFINED DMOD_REPO_DIR)
        set(INSTALL_REPO_DIR ${DMOD_REPO_DIR})
        message(STATUS "Module ${moduleName}: Will install to DMOD_REPO_DIR from configuration: ${INSTALL_REPO_DIR}")
    else()
        set(INSTALL_REPO_DIR ${DMOD_DMF_DIR})
        message(STATUS "Module ${moduleName}: Will install to default location: ${INSTALL_REPO_DIR}")
    endif()

    # Install DMF file
    install(FILES ${DMF_OUTPUT}
        DESTINATION ${INSTALL_REPO_DIR}
        COMPONENT modules
        OPTIONAL
    )

    # Install DMFC file if compression is enabled
    if(TODMFC)
        install(FILES ${DMFC_OUTPUT}
            DESTINATION ${INSTALL_REPO_DIR}
            COMPONENT modules
            OPTIONAL
        )
    endif()
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

    # Install the tool to the system bin directory
    install(TARGETS ${toolName}
        RUNTIME DESTINATION bin
        COMPONENT tools
    )
endfunction()