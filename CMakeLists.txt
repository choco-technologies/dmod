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

if (NOT DEFINED DMOD_TOOLS_NAME)
	message(STATUS "DMOD_TOOLS_NAME not defined, using default: arch/x86_64")
	set(DMOD_TOOLS_NAME "arch/x86_64")
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
	add_definitions(-DDMOD_MODULE)
else()
	message(FATAL_ERROR "Unknown DMOD_MODE: ${DMOD_MODE}")
endif()

add_definitions(-DON=1 -DOFF=0)

# ===========================================================================
# 						Subdirectories
# ===========================================================================
add_subdirectory(scripts)
add_subdirectory(inc)
add_subdirectory(src)
if (DMOD_BUILD_TESTS)
	add_subdirectory(tests)
endif()
if (DMOD_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

# Create a simple configuration header
configure_file(config.h.in config.h)
# Prepare the info file for the makefiles
configure_file(scripts/dmod-info.mk.in ${DMOD_DIR}/dmod-info.mk)

# ===========================================================================
#       				Main library
# ===========================================================================
add_library(dmod INTERFACE)

target_link_libraries(dmod INTERFACE dmod_inc dmod_common)
if (DMOD_SYSTEM)
	target_link_libraries(dmod INTERFACE dmod_system)
elseif (DMOD_MODULE)
	target_link_libraries(dmod INTERFACE dmod_module)
endif()