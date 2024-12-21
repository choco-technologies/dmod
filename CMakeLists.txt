cmake_minimum_required(VERSION 3.18)

# ===========================================================================
# 						Configuration options
# ===========================================================================

option(DMOD_USE_STDLIB 	"Use stdlib for memory allocations" 	ON)
option(DMOD_USE_STDIO  	"Use stdio for file operations" 		ON)
option(DMOD_USE_ASSERT 	"Use assert for error handling" 		ON)

set(DMOD_MAX_MODULES 	30 		  	  CACHE STRING "Maximum number of modules")
set(DMOD_MODE  			"DMOD_SYSTEM" CACHE STRING "Type of the module: DMOD_SYSTEM or DMOD_MODULE")

# ===========================================================================
# 						Project definition
# ===========================================================================
# Set some basic project attributes
project (dmod
	VERSION 0.1
	DESCRIPTION "Library for managing of dynamic modules")


if(DMOD_MODE STREQUAL "DMOD_SYSTEM")
	add_definitions(-DDMOD_SYSTEM)	
elseif(DMOD_MODE STREQUAL "DMOD_MODULE")
	add_definitions(-DDMOD_MODULE)
else()
	message(FATAL_ERROR "Unknown DMOD_MODE: ${DMOD_MODE}")
endif()

add_subdirectory(inc)
add_subdirectory(src)

# Create a simple configuration header
configure_file(config.h.in config.h)


