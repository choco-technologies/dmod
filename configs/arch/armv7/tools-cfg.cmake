#================================================================================================================================
# 	Default tools configuration
#================================================================================================================================

#
#	Toolchain configuration
#
set(CROSS_COMPILE arm-none-eabi-)
set(CPUCONFIG_CFLAGS "-mcpu=cortex-m7 -mthumb -mno-unaligned-access -DGCC_ARMCM7 -mpic-data-is-text-relative")
set(CPUCONFIG_CXXFLAGS "-mcpu=cortex-m7 -mthumb -mno-unaligned-access -DGCC_ARMCM7 -mpic-data-is-text-relative")
set(CMAKE_C_COMPILER "${CROSS_COMPILE}gcc")
set(CMAKE_CXX_COMPILER "${CROSS_COMPILE}g++")
set(CMAKE_LINKER "${CROSS_COMPILE}ld")
set(CMAKE_OBJDUMP "${CROSS_COMPILE}objdump")
set(CMAKE_OBJCOPY "${CROSS_COMPILE}objcopy")
set(CMAKE_AR "${CROSS_COMPILE}ar")
set(MAKE make)
set(MKDIR mkdir)
set(RM rm)
set(CMAKE_C_FLAGS "-Wall -std=c11 ${CPUCONFIG_CFLAGS}")
set(CMAKE_CXX_FLAGS "-Wall -std=c++17 ${CPUCONFIG_CXXFLAGS}")
set(CMAKE_LFLAGS "${CPUCONFIG_LDFLAGS}")