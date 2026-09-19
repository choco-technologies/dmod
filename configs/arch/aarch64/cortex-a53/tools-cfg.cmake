#================================================================================================================================
# 	Default tools configuration
#================================================================================================================================
# Raspberry Pi 3B (Broadcom BCM2837, Cortex-A53) running a 64-bit (aarch64) Linux userspace,
# e.g. the 64-bit Raspberry Pi OS or Ubuntu Server for Raspberry Pi. Defaults to all standard
# library facilities enabled, matching other full-OS targets (see arch/x86_64).

#
#	Toolchain configuration
#
if(NOT DEFINED CROSS_COMPILE)
	set(CROSS_COMPILE aarch64-linux-gnu-)
endif()

find_program(AARCH64_GCC ${CROSS_COMPILE}gcc)
if(NOT AARCH64_GCC)
    message(FATAL_ERROR "AArch64 GCC compiler not found")
endif()

find_program(AARCH64_GXX ${CROSS_COMPILE}g++)
if(NOT AARCH64_GXX)
    message(FATAL_ERROR "AArch64 G++ compiler not found")
endif()

find_program(AARCH64_LD ${CROSS_COMPILE}ld)
if(NOT AARCH64_LD)
    message(FATAL_ERROR "AArch64 linker not found")
endif()

find_program(AARCH64_OBJDUMP ${CROSS_COMPILE}objdump)
if(NOT AARCH64_OBJDUMP)
    message(FATAL_ERROR "AArch64 objdump not found")
endif()

find_program(AARCH64_OBJCOPY ${CROSS_COMPILE}objcopy)
if(NOT AARCH64_OBJCOPY)
    message(FATAL_ERROR "AArch64 objcopy not found")
endif()

find_program(AARCH64_AR ${CROSS_COMPILE}ar)
if(NOT AARCH64_AR)
    message(FATAL_ERROR "AArch64 ar not found")
endif()

find_program(AARCH64_SIZE ${CROSS_COMPILE}size)
if(NOT AARCH64_SIZE)
    message(FATAL_ERROR "AArch64 size not found")
endif()

find_program(AARCH64_GDB ${CROSS_COMPILE}gdb)
if(NOT AARCH64_GDB)
    find_program(AARCH64_GDB gdb-multiarch)
    if(NOT AARCH64_GDB)
        message(FATAL_ERROR "AArch64 GDB not found (tried ${CROSS_COMPILE}gdb and gdb-multiarch)")
    else()
        message(STATUS "Using gdb-multiarch for AArch64 debugging")
    endif()
endif()

# ==============================================================================
#                         CMake Configuration
# ==============================================================================
set(DMOD_ARCH "aarch64-cortex-a53" CACHE STRING "Target architecture")
set(DMOD_CPU "cortex-a53" CACHE STRING "Target CPU")
set(COMMON_DEFINE_FLAGS "-DDMOD_ARCH=\\\"${DMOD_ARCH}\\\" -DDMOD_CPU=\\\"${DMOD_CPU}\\\"")
set(CPUCONFIG_CFLAGS "-mcpu=cortex-a53 ${COMMON_DEFINE_FLAGS}" CACHE STRING "C compiler flags")
set(CPUCONFIG_CXXFLAGS "-mcpu=cortex-a53 ${COMMON_DEFINE_FLAGS}" CACHE STRING "C++ compiler flags")
set(CPUCONFIG_ASMFLAGS "-mcpu=cortex-a53 ${COMMON_DEFINE_FLAGS}" CACHE STRING "ASM compiler flags")
set(CPUCONFIG_LDFLAGS "-mcpu=cortex-a53 -Wl,--gc-sections" CACHE STRING "Linker flags")
set(CMAKE_C_COMPILER "${AARCH64_GCC}" CACHE STRING "C compiler")
set(CMAKE_CXX_COMPILER "${AARCH64_GXX}" CACHE STRING "C++ compiler")
set(CMAKE_LINKER "${AARCH64_LD}" CACHE STRING "Linker")
set(CMAKE_OBJDUMP "${AARCH64_OBJDUMP}" CACHE STRING "Objdump")
set(CMAKE_OBJCOPY "${AARCH64_OBJCOPY}" CACHE STRING "Objcopy")
set(CMAKE_SIZE "${AARCH64_SIZE}" CACHE STRING "Size")
set(CMAKE_AR "${AARCH64_AR}" CACHE STRING "Archiver")
set(CMAKE_GDB "${AARCH64_GDB}" CACHE STRING "GDB")
set(MAKE make CACHE STRING "Make")
set(MKDIR mkdir CACHE STRING "Mkdir")
set(RM rm CACHE STRING "Rm")
set(CMAKE_C_FLAGS "-Wall -std=c11 ${CPUCONFIG_CFLAGS}" CACHE STRING "C compiler flags")
set(CMAKE_CXX_FLAGS "-Wall -std=c++17 ${CPUCONFIG_CXXFLAGS}" CACHE STRING "C++ compiler flags")
set(CMAKE_ASM_FLAGS "${CPUCONFIG_ASMFLAGS}" CACHE STRING "ASM compiler flags")
set(CMAKE_ASM_FLAGS_DEBUG "${CPUCONFIG_ASMFLAGS} -g" CACHE STRING "ASM compiler flags for Debug")
set(CMAKE_EXE_LINKER_FLAGS "${CPUCONFIG_LDFLAGS}" CACHE STRING "Linker flags")

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY" CACHE STRING "Try compile target type")
