cmake_minimum_required(VERSION 3.18)

# Set some basic project attributes
project (dmod
	VERSION 0.1
	DESCRIPTION "Library for managing of dynamic modules")

set(DMOD_MODE  "DMOD_SYSTEM" CACHE STRING "Type of the module: DMOD_SYSTEM or DMOD_MODULE")

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


