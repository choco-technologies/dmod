#================================================================================================================================
# 	Default tools configuration
#================================================================================================================================

#
#   Default configuration options
#
set(DMOD_USE_STDLIB 	        ON )
set(DMOD_USE_STDIO  	        ON )
set(DMOD_USE_ASSERT 	        ON )
set(DMOD_USE_PTHREAD            ON )
set(DMOD_USE_MMAN   	        ON )
set(DMOD_BUILD_TESTS            ON )
set(DMOD_BUILD_EXAMPLES         ON )
set(DMOD_BUILD_TOOLS            ON )

#
#	Toolchain configuration
#
if(NOT DEFINED COMPILER_PATH)
	set(COMPILER_PATH "")
endif()
if(NOT DEFINED CROSS_COMPILE)
	set(CROSS_COMPILE "")
endif()
set(CMAKE_C_COMPILER "${CROSS_COMPILE}gcc")
set(CMAKE_CXX_COMPILER "${CROSS_COMPILE}g++")
set(CMAKE_LINKER "${CROSS_COMPILE}ld")
set(CMAKE_OBJDUMP "${CROSS_COMPILE}objdump")
set(CMAKE_OBJCOPY "${CROSS_COMPILE}objcopy")
set(CMAKE_AR "${CROSS_COMPILE}ar")
set(MAKE "make")
set(MKDIR "mkdir")
set(RM "rm")
set(CMAKE_C_FLAGS "-Wall -std=c11 ${CPUCONFIG_CFLAGS}")
set(CMAKE_CXX_FLAGS "-Wall -std=c++17 ${CPUCONFIG_CXXFLAGS}")
set(CMAKE_LFLAGS "${CPUCONFIG_LDFLAGS}")