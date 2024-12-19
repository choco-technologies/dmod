cmake_minimum_required(VERSION 3.10)

# Set some basic project attributes
project (dmod
	VERSION 0.1
	DESCRIPTION "Library for managing of dynamic modules")


if(DMOD_SYSTEM)
	add_definitions(-DDMOD_SYSTEM)	
elseif(DMOD_MODULE)
	add_definitions(-DDMOD_MODULE)
endif()

add_subdirectory(inc)
add_subdirectory(src)

# Create a simple configuration header
configure_file(config.h.in config.h)


