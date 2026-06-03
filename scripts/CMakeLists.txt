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
    
    # ===========================================================================
    # Memory Analysis for Static Libraries
    # ===========================================================================
    # Collect object files for memory analysis
    get_target_property(LIB_SOURCES ${targetName} SOURCES)
    set(LIB_OBJECT_FILES "")
    foreach(SOURCE_FILE ${LIB_SOURCES})
        get_filename_component(SOURCE_EXT "${SOURCE_FILE}" EXT)
        # Handle both .c and .cpp files
        if(SOURCE_EXT STREQUAL ".c")
            set(OBJ_FILE "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${targetName}.dir/${SOURCE_FILE}.o")
        elseif(SOURCE_EXT STREQUAL ".cpp")
            set(OBJ_FILE "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${targetName}.dir/${SOURCE_FILE}.o")
        else()
            continue()
        endif()
        list(APPEND LIB_OBJECT_FILES "${OBJ_FILE}")
    endforeach()
    
    # Convert list to string for passing to script
    string(REPLACE ";" "@@" LIB_OBJECT_FILES_STR "${LIB_OBJECT_FILES}")
    
    # Add custom command to show memory analysis after build
    add_custom_command(
        TARGET ${targetName} POST_BUILD
        COMMAND ${CMAKE_COMMAND} 
            -DOBJECT_FILES=${LIB_OBJECT_FILES_STR}
            -DMODULE_NAME=${targetName}
            -DANALYSIS_TYPE=summary
            -P ${DMOD_SCRIPTS_DIR}/memory_analysis.cmake
        COMMENT "Analyzing memory usage for ${targetName}"
    )
    
    # Add custom target for detailed symbol analysis
    add_custom_target(${targetName}_memory_detailed
        COMMAND ${CMAKE_COMMAND} 
            -DOBJECT_FILES=${LIB_OBJECT_FILES_STR}
            -DMODULE_NAME=${targetName}
            -DANALYSIS_TYPE=detailed
            -P ${DMOD_SCRIPTS_DIR}/memory_analysis.cmake
        COMMENT "Detailed memory analysis for ${targetName}"
    )
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

    # Link against libgcc explicitly since -nostdlib is used.
    # This provides ARM runtime helpers like __aeabi_uldivmod (64-bit division),
    # __aeabi_fadd, __aeabi_f2iz (float operations), etc.
    separate_arguments(LIBGCC_QUERY_FLAGS UNIX_COMMAND "${CMAKE_C_FLAGS}")
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} ${LIBGCC_QUERY_FLAGS} -print-libgcc-file-name
        OUTPUT_VARIABLE LIBGCC_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(LIBGCC_PATH AND EXISTS "${LIBGCC_PATH}")
        target_link_libraries(${moduleName} ${LIBGCC_PATH})
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(OPTIMIZATION "-Og")
    else()
        set(OPTIMIZATION "-O2")
    endif()
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
        # Check if version requirements file was created by dmod_link_modules
        get_target_property(VERSION_REQS_FILE ${moduleName} DMOD_VERSION_REQUIREMENTS_FILE)
        if(VERSION_REQS_FILE AND EXISTS "${VERSION_REQS_FILE}")
            # Pass version requirements file to todmd
            add_custom_command(
                TARGET ${moduleName} POST_BUILD
                COMMAND ${TODMD} ${DMF_OUTPUT} ${DMD_OUTPUT} -r ${VERSION_REQS_FILE}
                COMMENT "Generating dependencies file with version requirements"
            )
            message(STATUS "Will use version requirements from: ${VERSION_REQS_FILE}")
        else()
            # No version requirements file, use original command
            add_custom_command(
                TARGET ${moduleName} POST_BUILD
                COMMAND ${TODMD} ${DMF_OUTPUT} ${DMD_OUTPUT}
                COMMENT "Generating dependencies file"
            )
        endif()
    endif()

    # Print the source files
    message(STATUS "SOURCES for module ${moduleName}: ${SOURCES}")

    # ===========================================================================
    # Memory Analysis
    # ===========================================================================
    # Collect object files for memory analysis
    set(MODULE_OBJECT_FILES "")
    foreach(SOURCE_FILE ${SOURCES})
        # Preserve the source file's relative path structure for the object file
        set(OBJ_FILE "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${moduleName}.dir/${SOURCE_FILE}.o")
        list(APPEND MODULE_OBJECT_FILES "${OBJ_FILE}")
    endforeach()
    # Add the generated header object file
    list(APPEND MODULE_OBJECT_FILES "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${moduleName}.dir/${DMOD_MODULE_NAME_SNAKE_CASE}_header.c.o")
    
    # Convert list to string for passing to script (semicolon separated)
    string(REPLACE ";" "@@" MODULE_OBJECT_FILES_STR "${MODULE_OBJECT_FILES}")
    
    # Add custom command to show memory analysis after build
    add_custom_command(
        TARGET ${moduleName} POST_BUILD
        COMMAND ${CMAKE_COMMAND} 
            -DOBJECT_FILES=${MODULE_OBJECT_FILES_STR}
            -DMODULE_NAME=${moduleName}
            -DDMOD_MODULE_NAME="${moduleName}"
            -DANALYSIS_TYPE=summary
            -P ${DMOD_SCRIPTS_DIR}/memory_analysis.cmake
        COMMENT "Analyzing memory usage for ${moduleName}"
    )
    
    # Add custom target for detailed symbol analysis
    add_custom_target(${moduleName}_memory_detailed
        COMMAND ${CMAKE_COMMAND} 
            -DOBJECT_FILES=${MODULE_OBJECT_FILES_STR}
            -DMODULE_NAME=${moduleName}
            -DDMOD_MODULE_NAME="${moduleName}"
            -DANALYSIS_TYPE=detailed
            -P ${DMOD_SCRIPTS_DIR}/memory_analysis.cmake
        COMMENT "Detailed memory analysis for ${moduleName}"
    )

    # ===========================================================================
    # Install target for modules
    # ===========================================================================
    # Determine installation directory for module files
    # Priority: Environment variable > Configuration variable > DMF_DIR default
    if(DEFINED ENV{DMOD_REPO_DIR})
        set(INSTALL_REPO_DIR $ENV{DMOD_REPO_DIR})
        message(STATUS "Module ${moduleName}: Will install to DMOD_REPO_DIR from environment: ${INSTALL_REPO_DIR}")
    elseif(DEFINED ENV{DMOD_DMF_DIR})
        set(INSTALL_REPO_DIR $ENV{DMOD_DMF_DIR})
        message(STATUS "Module ${moduleName}: Will install to DMOD_DMF_DIR from environment: ${INSTALL_REPO_DIR}")
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

    # ===========================================================================
    # Release package creation from .dmr file
    # ===========================================================================
    if(DEFINED DMOD_DMR_PATH)
        # Resolve the dmr path relative to the caller's source directory
        if(NOT IS_ABSOLUTE "${DMOD_DMR_PATH}")
            set(DMOD_DMR_ABS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${DMOD_DMR_PATH}")
        else()
            set(DMOD_DMR_ABS_PATH "${DMOD_DMR_PATH}")
        endif()

        # Find mkdmrpkg tool
        find_program(MKDMRPKG
            mkdmrpkg
            HINTS ${DMOD_TOOLS_BIN_DIR}
        )

        if(MKDMRPKG)
            set(DMOD_PACKAGES_DIR "${CMAKE_BINARY_DIR}/packages")
            set(DMOD_PKG_OUT_DIR "${DMOD_PACKAGES_DIR}/${moduleName}")
            set(DMOD_PKG_ZIP_PATH "${DMOD_PACKAGES_DIR}/${moduleName}.zip")
            set(DMOD_LOCAL_MANIFEST_PATH "${DMOD_PACKAGES_DIR}/manifest-local.dmm")

            # Create the packages directory
            file(MAKE_DIRECTORY "${DMOD_PACKAGES_DIR}")

            # Run mkdmrpkg to populate the package directory, then create zip archive
            add_custom_command(
                TARGET ${moduleName} POST_BUILD
                COMMAND ${MKDMRPKG} "${DMOD_DMR_ABS_PATH}"
                    -m "${moduleName}"
                    -r "${CMAKE_SOURCE_DIR}"
                    -b "${CMAKE_BINARY_DIR}"
                    --dmf-dir "${DMOD_DMF_DIR}"
                    -o "${DMOD_PKG_OUT_DIR}"
                COMMAND ${CMAKE_COMMAND} -E chdir "${DMOD_PKG_OUT_DIR}"
                    ${CMAKE_COMMAND} -E tar "cf" "${DMOD_PKG_ZIP_PATH}" --format=zip -- .
                WORKING_DIRECTORY "${DMOD_PACKAGES_DIR}"
                COMMENT "Creating release package and zip archive for ${moduleName}"
            )

            # Find todmm tool and generate local manifest from all zip packages
            find_program(TODMM
                todmm
                HINTS ${DMOD_TOOLS_BIN_DIR}
            )

            if(TODMM)
                # Create a shared target (once per project) that regenerates the local
                # manifest from ALL package zips after every module has been built.
                # Using a dedicated target (rather than per-module POST_BUILD) guarantees
                # that todmm runs only after ALL modules have created their zip archives,
                # so the manifest is always complete when local .dmd files are generated.
                if(NOT TARGET dmod_generate_manifest)
                    add_custom_target(dmod_generate_manifest ALL)
                    add_custom_command(
                        TARGET dmod_generate_manifest POST_BUILD
                        # todmm <folder> <base_url> --zip -o <output>
                        # For a local manifest, base_url is the local packages directory path
                        COMMAND ${TODMM} "${DMOD_PACKAGES_DIR}" "${DMOD_PACKAGES_DIR}" --zip -o "${DMOD_LOCAL_MANIFEST_PATH}"
                        COMMENT "Generating local manifest from all packages: ${DMOD_LOCAL_MANIFEST_PATH}"
                    )
                endif()

                # This module's zip must exist before the manifest is generated
                add_dependencies(dmod_generate_manifest ${moduleName})

                # Generate <module_name>-local.dmd using todmd AFTER the complete manifest
                # is ready (i.e., after all modules' zips have been collected by todmm).
                if(TODMD)
                    set(LOCAL_DMD_OUTPUT "${DMOD_DMF_DIR}/${moduleName}-local.dmd")

                    # A per-module ALL target that runs todmd once the manifest is complete
                    add_custom_target(${moduleName}_local_dmd ALL)
                    add_dependencies(${moduleName}_local_dmd dmod_generate_manifest)

                    get_target_property(VERSION_REQS_FILE ${moduleName} DMOD_VERSION_REQUIREMENTS_FILE)
                    if(VERSION_REQS_FILE AND EXISTS "${VERSION_REQS_FILE}")
                        add_custom_command(
                            TARGET ${moduleName}_local_dmd POST_BUILD
                            COMMAND ${TODMD} ${DMF_OUTPUT} ${DMD_OUTPUT} -r ${VERSION_REQS_FILE} --local-manifest "${DMOD_LOCAL_MANIFEST_PATH}"
                            COMMENT "Generating local dependencies file: ${LOCAL_DMD_OUTPUT}"
                        )
                    else()
                        add_custom_command(
                            TARGET ${moduleName}_local_dmd POST_BUILD
                            COMMAND ${TODMD} ${DMF_OUTPUT} ${DMD_OUTPUT} --local-manifest "${DMOD_LOCAL_MANIFEST_PATH}"
                            COMMENT "Generating local dependencies file: ${LOCAL_DMD_OUTPUT}"
                        )
                    endif()
                endif()
            else()
                message(WARNING "todmm not found - local manifest for packages will not be generated. Build the tools first with -DDMOD_BUILD_TOOLS=ON")
            endif()
        else()
            message(WARNING "mkdmrpkg not found - release package for ${moduleName} will not be created. Build the tools first with -DDMOD_BUILD_TOOLS=ON")
        endif()
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
# Function to create a DMOD test module
#
# Usage:
#   dmod_add_test(moduleName version sources...)
#
# Creates an Application-type module with a pre-provided main() that
# automatically discovers and runs all test steps registered via
# DMOD_TEST_STEP().  Do NOT define main() yourself in the test sources.
#
# Lifecycle hooks dmod_test_setup() and dmod_test_teardown() are called
# before/after each step; define them in your test sources to override
# the default empty implementations.
#
function(dmod_add_test moduleName version)
    set(SOURCES ${ARGN})

    # Inject the pre-built test runner as an additional source file
    list(APPEND SOURCES ${DMOD_SRC_DIR}/module/dmod_test_main.c)

    dmod_create_module(${moduleName} ${version} Application ${SOURCES})
endfunction()

#
# Function to link external modules and download their headers
#
# Usage:
#   dmod_link_modules(target_name [PRIVATE|PUBLIC|INTERFACE]
#       module1@1.0
#       module2
#       module3@2.1
#   )
#
# This function downloads headers for the specified modules using dmf-get
# and adds them to the target's include directories.
#
# The visibility scope (PRIVATE, PUBLIC, or INTERFACE) can be specified and
# works the same way as target_include_directories. If not specified, PRIVATE is used.
#
function(dmod_link_modules targetName)
    # Parse arguments for visibility scope
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs PRIVATE PUBLIC INTERFACE)
    cmake_parse_arguments(DMOD_LINK "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # Determine visibility scope and module list
    set(VISIBILITY "PRIVATE")
    set(MODULE_SPECS "")
    
    if(DMOD_LINK_PRIVATE)
        set(VISIBILITY "PRIVATE")
        set(MODULE_SPECS ${DMOD_LINK_PRIVATE})
    elseif(DMOD_LINK_PUBLIC)
        set(VISIBILITY "PUBLIC")
        set(MODULE_SPECS ${DMOD_LINK_PUBLIC})
    elseif(DMOD_LINK_INTERFACE)
        set(VISIBILITY "INTERFACE")
        set(MODULE_SPECS ${DMOD_LINK_INTERFACE})
    else()
        # No visibility keyword specified, treat all unparsed args as module specs with PRIVATE visibility
        set(MODULE_SPECS ${DMOD_LINK_UNPARSED_ARGUMENTS})
    endif()
    
    # Check if target exists
    if(NOT TARGET ${targetName})
        message(FATAL_ERROR "dmod_link_modules: Target '${targetName}' does not exist")
    endif()
    
    # Find dmf-get tool
    find_program(DMF_GET 
        dmf-get
        HINTS ${DMOD_TOOLS_BIN_DIR}
        PATHS ENV PATH
    )
    
    # Save version requirements for this target to be used by todmd
    # This file will be passed to todmd when generating .dmd files
    set(VERSION_REQS_FILE "${CMAKE_CURRENT_BINARY_DIR}/${targetName}_version_requirements.txt")
    file(WRITE "${VERSION_REQS_FILE}" "# Version requirements for ${targetName}\n")
    file(APPEND "${VERSION_REQS_FILE}" "# Generated by dmod_link_modules\n")
    file(APPEND "${VERSION_REQS_FILE}" "# Format: module_name@version\n\n")
    
    # Store the version requirements file path as a target property
    # This will be used by dmod_create_module to pass to todmd
    set_target_properties(${targetName} PROPERTIES
        DMOD_VERSION_REQUIREMENTS_FILE "${VERSION_REQS_FILE}"
    )
    
    if(NOT DMF_GET)
        message(WARNING "dmf-get tool not found. Skipping module headers download for ${targetName}. Build the tools first with -DDMOD_BUILD_TOOLS=ON")
        # Still save version requirements even if dmf-get is not available
        foreach(MODULE_SPEC ${MODULE_SPECS})
            # Parse module name and version
            string(REGEX MATCH "^([^@]+)(@(.+))?$" MATCH_RESULT "${MODULE_SPEC}")
            set(MODULE_NAME "${CMAKE_MATCH_1}")
            set(MODULE_VERSION "${CMAKE_MATCH_3}")
            
            if(NOT MODULE_NAME)
                message(WARNING "Invalid module specification: ${MODULE_SPEC}")
                continue()
            endif()
            
            # Save version requirement to file if version is specified
            if(MODULE_VERSION)
                file(APPEND "${VERSION_REQS_FILE}" "${MODULE_NAME}@${MODULE_VERSION}\n")
                message(STATUS "Saved version requirement: ${MODULE_NAME}@${MODULE_VERSION}")
            else()
                file(APPEND "${VERSION_REQS_FILE}" "${MODULE_NAME}\n")
            endif()
        endforeach()
        return()
    endif()
    
    message(STATUS "Using dmf-get: ${DMF_GET}")
    
    # Get the default DMF directory for headers
    if(NOT DEFINED DMOD_DMF_DIR)
        set(DMOD_DMF_DIR "${CMAKE_BINARY_DIR}/dmf")
    endif()
    
    # Prepare dmf-get command options
    set(DMF_GET_OPTIONS "")
    
    # Add tools name if defined
    if(DEFINED DMOD_TOOLS_NAME)
        list(APPEND DMF_GET_OPTIONS "-t" "${DMOD_TOOLS_NAME}")
        message(STATUS "Using tools name: ${DMOD_TOOLS_NAME}")
    endif()
    
    # Add manifest URL if DMOD_DMM_URL is defined
    if(DEFINED DMOD_DMM_URL)
        list(APPEND DMF_GET_OPTIONS "-m" "${DMOD_DMM_URL}")
        message(STATUS "Using manifest: ${DMOD_DMM_URL}")
    endif()
    
    # Process each module specification
    foreach(MODULE_SPEC ${MODULE_SPECS})
        # Parse module name and version
        string(REGEX MATCH "^([^@]+)(@(.+))?$" MATCH_RESULT "${MODULE_SPEC}")
        set(MODULE_NAME "${CMAKE_MATCH_1}")
        set(MODULE_VERSION "${CMAKE_MATCH_3}")
        
        if(NOT MODULE_NAME)
            message(WARNING "Invalid module specification: ${MODULE_SPEC}")
            continue()
        endif()
        
        # Save version requirement to file if version is specified
        if(MODULE_VERSION)
            file(APPEND "${VERSION_REQS_FILE}" "${MODULE_NAME}@${MODULE_VERSION}\n")
            message(STATUS "Saved version requirement: ${MODULE_NAME}@${MODULE_VERSION}")
        else()
            file(APPEND "${VERSION_REQS_FILE}" "${MODULE_NAME}\n")
        endif()
        
        # Construct the header output directory
        # Note: dmf-get creates a subdirectory with the module name, so we don't add it here
        set(HEADER_OUTPUT_DIR "${DMOD_DMF_DIR}/inc")
        
        message(STATUS "Downloading headers for module: ${MODULE_NAME} -> ${HEADER_OUTPUT_DIR}")
        
        # Create the output directory
        file(MAKE_DIRECTORY "${HEADER_OUTPUT_DIR}")
        
        # Run dmf-get headers command with options
        execute_process(
            COMMAND ${DMF_GET} headers ${MODULE_SPEC} ${DMF_GET_OPTIONS} -o ${HEADER_OUTPUT_DIR}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            RESULT_VARIABLE DMF_GET_RESULT
            ERROR_VARIABLE DMF_GET_ERROR
        )
        
        if(NOT DMF_GET_RESULT EQUAL 0)
            message(WARNING "Failed to download headers for ${MODULE_SPEC}: ${DMF_GET_ERROR}")
            continue()
        endif()
        
        # Add the header directory to the target's include directories with specified visibility
        # dmf-get creates MODULE_NAME subdirectory with an 'include' subdirectory inside
        # So the actual headers are in ${HEADER_OUTPUT_DIR}/${MODULE_NAME}/include
        set(ACTUAL_HEADER_DIR "${HEADER_OUTPUT_DIR}/${MODULE_NAME}/include")
        if(EXISTS "${ACTUAL_HEADER_DIR}")
            target_include_directories(${targetName} ${VISIBILITY} ${ACTUAL_HEADER_DIR})
            message(STATUS "Added include directory for ${MODULE_NAME} (${VISIBILITY}): ${ACTUAL_HEADER_DIR}")
        else()
            message(WARNING "Header directory does not exist after download: ${ACTUAL_HEADER_DIR}")
        endif()
    endforeach()
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