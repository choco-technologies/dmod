cmake_minimum_required(VERSION 3.18)

# ===========================================================================
# 						Configuration options
# ===========================================================================

option(DMOD_USE_STDLIB 	"Use stdlib for memory allocations" 	ON)
option(DMOD_USE_STDIO  	"Use stdio for file operations" 		ON)
option(DMOD_USE_ASSERT 	"Use assert for error handling" 		ON)
option(DMOD_USE_PTHREAD "Use pthread for threading" 			ON)

set(DMOD_MAX_MODULES 			30            CACHE STRING "Maximum number of modules")
set(DMOD_MAX_REQUIRED_MODULES 	10            CACHE STRING "Maximum number of required modules")
set(DMOD_MODE  			        "DMOD_SYSTEM" CACHE STRING "Type of the module: DMOD_SYSTEM or DMOD_MODULE")
set(DMOD_SYSTEM_VERSION_MAJOR 	0             CACHE STRING "Version major")
set(DMOD_SYSTEM_VERSION_MINOR 	1             CACHE STRING "Version minor")
set(DMOD_BUILD_TESTS 			ON            CACHE BOOL   "Build tests")
set(DMOD_BUILD_EXAMPLES 		ON            CACHE BOOL   "Build examples")
set(DMOD_DMF_DIR                "${CMAKE_CURRENT_BINARY_DIR}/dmf" CACHE STRING "Directory for DMF files")
set(DMOD_REPO_DIR               "${DMOD_DMF_DIR}" 				  CACHE STRING "Directory for DMF files inside the system")
set(DMOD_CPU_NAME			    "" 						  	      CACHE STRING "Name of the target cpu, if empty, the target is generic")

# ===========================================================================
# 						Project definition
# ===========================================================================
# Set some basic project attributes
project (dmod
	VERSION 0.1
	DESCRIPTION "Library for managing of dynamic modules")

set(DMOD_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

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

# Create a simple configuration header
configure_file(config.h.in config.h)

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
