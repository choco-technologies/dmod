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

set(DMOD_PATHS "${DMOD_DIR}/paths.cmake")

# ===========================================================================
# 						Include the configurations
# ===========================================================================
include(${DMOD_PATHS})
message(STATUS "Using DMOD_TOOLS: ${DMOD_TOOLS}")
include(${DMOD_TOOLS})

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
	set(EXCLUDED_DIRS "tests" "examples" "scripts" "src/system/if")
	set(EXCLUDE_DIRS "")

	foreach(dir ${EXCLUDED_DIRS})
		list(APPEND EXCLUDE_DIRS "--exclude ${DMOD_DIR}/${dir}")
	endforeach()

	add_custom_target(coverage ALL
		COMMAND ${GCOVR} 
			-r "${DMOD_DIR}"
			--exclude "${DMOD_DIR}/lib"
			--exclude "${DMOD_DIR}/tests/.*"
			--exclude "${DMOD_DIR}/examples"
			--exclude "${DMOD_DIR}/scripts"
			--exclude "${DMOD_BUILD_DIR}"
			--exclude "${DMOD_DIR}/src/system/if"
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
			--exclude "${DMOD_DIR}/lib"
			--exclude "${DMOD_DIR}/tests/.*"
			--exclude "${DMOD_DIR}/examples"
			--exclude "${DMOD_DIR}/scripts"
			--exclude "${DMOD_BUILD_DIR}"
			--exclude "${DMOD_DIR}/src/system/if"

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
if (DMOD_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

# Create a simple configuration header
configure_file(dmod-config.h.in dmod-config.h)
# Prepare the info file for the makefiles
configure_file(scripts/dmod-info.mk.in ${DMOD_DIR}/dmod-info.mk)

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