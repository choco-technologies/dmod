#================================================================================================================================
# 	Default tools configuration
#================================================================================================================================

#
#   Default configuration options
#
set(DMOD_USE_STDLIB 	        OFF )
set(DMOD_USE_STDIO  	        OFF )
set(DMOD_USE_ASSERT 	        OFF )
set(DMOD_USE_PTHREAD            OFF )
set(DMOD_USE_MMAN   	        OFF )
set(DMOD_BUILD_TESTS            OFF )
if (DMOD_MODE STREQUAL "DMOD_SYSTEM")
    set(DMOD_BUILD_EXAMPLES         OFF )
else()
    set(DMOD_BUILD_EXAMPLES         ON )
endif()

#
#	Toolchain configuration
#
if(NOT DEFINED COMPILER_PATH)
	set(COMPILER_PATH "")
endif()
if(NOT DEFINED CROSS_COMPILE)
	set(CROSS_COMPILE arm-none-eabi-)
endif()
set(CPUCONFIG_CFLAGS "-mcpu=cortex-m7 -mthumb -mno-unaligned-access -DGCC_ARMCM7 -mpic-data-is-text-relative")
set(CPUCONFIG_CXXFLAGS "-mcpu=cortex-m7 -mthumb -mno-unaligned-access -DGCC_ARMCM7 -mpic-data-is-text-relative")
set(CMAKE_C_COMPILER "${COMPILER_PATH}${CROSS_COMPILE}gcc")
set(CMAKE_CXX_COMPILER "${COMPILER_PATH}${CROSS_COMPILE}g++")
set(CMAKE_LINKER "${COMPILER_PATH}${CROSS_COMPILE}ld")
set(CMAKE_OBJDUMP "${COMPILER_PATH}${CROSS_COMPILE}objdump")
set(CMAKE_OBJCOPY "${COMPILER_PATH}${CROSS_COMPILE}objcopy")
set(CMAKE_AR "${COMPILER_PATH}${CROSS_COMPILE}ar")
set(MAKE make)
set(MKDIR mkdir)
set(RM rm)
set(CMAKE_C_FLAGS "-Wall -std=c11 ${CPUCONFIG_CFLAGS}")
set(CMAKE_CXX_FLAGS "-Wall -std=c++17 ${CPUCONFIG_CXXFLAGS}")
set(CMAKE_LFLAGS "${CPUCONFIG_LDFLAGS}")

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")