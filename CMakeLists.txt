cmake_minimum_required(VERSION 3.18)

# ===========================================================================
# 						Paths and directories
# ===========================================================================

if (NOT DEFINED DMOD_DIR)
	set(DMOD_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if (NOT DEFINED DMOD_BUILD_DIR)
	set(DMOD_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}")
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DNDEBUG)
endif()

set(DMOD_PATHS "${DMOD_DIR}/paths.cmake")

# ===========================================================================
# 						Include the configurations
# ===========================================================================
include(${DMOD_PATHS})
message(STATUS "Using DMOD_TOOLS: ${DMOD_TOOLS}")
if (DEFINED DMOD_TOOLS)
	include(${DMOD_TOOLS})
else()
	message(WARNING "DMOD_TOOLS not defined, using default values")
endif()

# ===========================================================================
# 						Project definition
# ===========================================================================
# Set some basic project attributes
project (dmod
	VERSION ${dmod_VERSION_MAJOR}.${dmod_VERSION_MINOR}
	DESCRIPTION "Library for managing of dynamic modules")

# ===========================================================================
# 						Configuration file
# ===========================================================================
if(DMOD_MODE STREQUAL "DMOD_SYSTEM")
	set(DMOD_SYSTEM ON)
	set(DMOD_MODULE OFF)
	add_definitions(-DDMOD_SYSTEM)	
elseif(DMOD_MODE STREQUAL "DMOD_MODULE")
	set(DMOD_SYSTEM OFF)
	set(DMOD_MODULE ON)
	set(DMOD_BUILD_TESTS OFF)
	add_definitions(-DDMOD_MODULE -DDMOD_BUILD_DIR="${DMOD_BUILD_DIR}")
else()
	message(FATAL_ERROR "Unknown DMOD_MODE: ${DMOD_MODE}")
endif()

add_definitions(-DON=1 -DOFF=0)

# ===========================================================================
# 						Mock Memory Configuration
# ===========================================================================
if (DEFINED DMOD_MEMORY)
	# Split by semicolon to support multiple memory regions
	string(REPLACE ";" "|" DMOD_MEMORY_LIST "${DMOD_MEMORY}")
	string(REPLACE "|" ";" DMOD_MEMORY_LIST "${DMOD_MEMORY_LIST}")
	
	set(DMOD_MEMORY_COUNT 0)
	foreach(MEMORY_REGION ${DMOD_MEMORY_LIST})
		string(REGEX MATCH "^([0-9a-fA-F]+):(.+)$" DMOD_MEMORY_MATCH "${MEMORY_REGION}")
		if (DMOD_MEMORY_MATCH)
			set(ADDR ${CMAKE_MATCH_1})
			set(FILE ${CMAKE_MATCH_2})
			message(STATUS "Mock memory region ${DMOD_MEMORY_COUNT}: address=0x${ADDR}, file=${FILE}")
			
			add_definitions(-DDMOD_MEMORY_MOCK_ADDRESS_${DMOD_MEMORY_COUNT}=0x${ADDR})
			add_definitions(-DDMOD_MEMORY_MOCK_FILE_${DMOD_MEMORY_COUNT}="${FILE}")
			
			math(EXPR DMOD_MEMORY_COUNT "${DMOD_MEMORY_COUNT} + 1")
			
			if (DMOD_MEMORY_COUNT GREATER 5)
				message(WARNING "Maximum 5 mock memory regions supported. Additional regions will be ignored.")
				break()
			endif()
		else()
			message(WARNING "Invalid memory region format: ${MEMORY_REGION}. Expected format: address:filepath")
		endif()
	endforeach()
	
	if (DMOD_MEMORY_COUNT GREATER 0)
		add_definitions(-DDMOD_MEMORY_MOCK_COUNT=${DMOD_MEMORY_COUNT})
		add_definitions(-DDMOD_MEMORY_MOCK_ENABLED=1)
	else()
		message(WARNING "No valid memory regions found in DMOD_MEMORY")
	endif()
endif()

# ===========================================================================
# 						Checks if code coverage is possible (if gcovr is installed)
# ===========================================================================
if (DMOD_BUILD_TESTS)
	find_program(GCOVR gcovr)

	# Check if GCOVR was found and if compiler id is GNU or Clang
	if (GCOVR AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
		message(STATUS "gcovr found: ${GCOVR}")
		set(DMOD_COVERAGE ON)

		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -O0 -g")
    	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -O0 -g")
    	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
	else()
		message(WARNING "gcovr not found or compiler is not GNU/Clang, code coverage will not be available")
		set(DMOD_COVERAGE OFF)
	endif()
endif()

# ===========================================================================
# 						Unit Tests Coverage
# ===========================================================================
if (DMOD_COVERAGE)
	# Add the coverage target
	set(EXCLUDED_DIRS "tests" "examples" "scripts" "src/system/if" "lib" "templates")
	set(EXCLUDE_DIRS --exclude "${DMOD_BUILD_DIR}")

	foreach(dir ${EXCLUDED_DIRS})
		list(APPEND EXCLUDE_DIRS --exclude ${DMOD_DIR}/${dir})
	endforeach()

	add_custom_target(coverage ALL
		COMMAND ${GCOVR} 
			-r "${DMOD_DIR}"
			${EXCLUDE_DIRS}
			--sort-percentage

			--print-summary
			--html --html-details -o ${DMOD_COVERAGE_FILE_PATH}
		WORKING_DIRECTORY ${DMOD_BUILD_DIR}
		COMMENT "Generating code coverage report"
	)

	# Verification of minimum coverage
	add_custom_target(coverage_check
		COMMAND ${GCOVR} 
			-r "${DMOD_DIR}"
			${EXCLUDE_DIRS}

			--fail-under-line ${DMOD_MIN_COVERAGE}
		WORKING_DIRECTORY ${DMOD_BUILD_DIR}
		COMMENT "Checking code coverage (Should be at least ${DMOD_MIN_COVERAGE}%)"
	)

endif()

# ===========================================================================
# 						Subdirectories
# ===========================================================================
add_subdirectory(scripts)
add_subdirectory(lib)
add_subdirectory(inc)
add_subdirectory(src)
if (DMOD_BUILD_TESTS)
	add_subdirectory(tests)
endif()
# if (DMOD_BUILD_EXAMPLES)
	# add_subdirectory(examples)
# endif()
if (DMOD_BUILD_TOOLS)
	add_subdirectory(tools)
endif()
if (DMOD_BUILD_TEMPLATES)
	add_subdirectory(templates)
endif()

# Create a simple configuration header
configure_file(dmod-config.h.in dmod-config.h)
# Prepare the info file for the makefiles
configure_file(scripts/dmod-info.mk.in ${DMOD_DIR}/dmod-info.mk)
# Prepare the version file
configure_file(${DMOD_VERSION_IN_FILE_PATH} ${DMOD_VERSION_FILE_PATH} @ONLY)

# ===========================================================================
#       				Main library
# ===========================================================================
add_library(dmod INTERFACE)

target_link_libraries(dmod INTERFACE dmod_inc dmod_common)
if (DMOD_USE_FASTLZ)
	target_link_libraries(dmod INTERFACE dmod_fastlz)
endif()
if (DMOD_SYSTEM)
	target_link_libraries(dmod INTERFACE dmod_system)
elseif (DMOD_MODULE)
	target_link_libraries(dmod INTERFACE dmod_module)
endif()

# ===========================================================================
#       			DMP Package Creation
# ===========================================================================
# Check if we should create DMP packages (only in MODULE mode)
if(DMOD_MODULE AND DEFINED DMOD_PACKAGE_NAME AND NOT "${DMOD_PACKAGE_NAME}" STREQUAL "")
    # Find todmp tool
    find_program(TODMP 
            todmp
            HINTS ${DMOD_TOOLS_BIN_DIR}
            )
    
    if(TODMP)
        message(STATUS "todmp found: ${TODMP}")
        message(STATUS "Will create DMP packages for package name: ${DMOD_PACKAGE_NAME}")
        
        # Set main module name if provided
        set(MAIN_MODULE_ARG "")
        if(DEFINED DMOD_MAIN_MODULE_NAME AND NOT "${DMOD_MAIN_MODULE_NAME}" STREQUAL "")
            set(MAIN_MODULE_ARG ${DMOD_MAIN_MODULE_NAME})
            message(STATUS "  Main module will be: ${DMOD_MAIN_MODULE_NAME}")
        endif()
        
        # Create custom target for DMP package creation
        set(DMP_DMF_OUTPUT "${DMOD_BUILD_DIR}/dmp/${DMOD_PACKAGE_NAME}_dmf.dmp")
        set(DMP_DMFC_OUTPUT "${DMOD_BUILD_DIR}/dmp/${DMOD_PACKAGE_NAME}_dmfc.dmp")
        
        file(MAKE_DIRECTORY ${DMOD_BUILD_DIR}/dmp)
        
        add_custom_target(dmp_package ALL
            COMMAND ${TODMP} ${DMOD_PACKAGE_NAME} ${DMOD_DMF_DIR} ${DMP_DMF_OUTPUT} ${MAIN_MODULE_ARG}
            COMMENT "Creating DMP package from DMF modules: ${DMP_DMF_OUTPUT}"
            VERBATIM
        )
        
        # Create DMFC package if todmfc is available
        find_program(TODMFC 
                todmfc
                HINTS ${DMOD_TOOLS_BIN_DIR}
                )
        if(TODMFC)
            add_custom_target(dmp_package_dmfc ALL
                COMMAND ${TODMP} ${DMOD_PACKAGE_NAME} ${DMOD_DMFC_DIR} ${DMP_DMFC_OUTPUT} ${MAIN_MODULE_ARG}
                COMMENT "Creating DMP package from DMFC modules: ${DMP_DMFC_OUTPUT}"
                VERBATIM
            )
        endif()
    else()
        message(WARNING "todmp not found, DMP packages will not be created")
    endif()
endif()
