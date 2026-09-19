#================================================================================================================================
# 	Default tools configuration
#================================================================================================================================
# Raspberry Pi 3B (Broadcom BCM2837, Cortex-A53) running a 32-bit (armhf) Linux userspace,
# e.g. the 32-bit Raspberry Pi OS. Defaults to all standard library facilities enabled,
# matching other full-OS targets (see arch/x86_64).

#
#	Toolchain configuration
#
if(NOT DEFINED CROSS_COMPILE)
	set(CROSS_COMPILE arm-linux-gnueabihf-)
endif()

find_program(ARM_GCC ${CROSS_COMPILE}gcc)
if(NOT ARM_GCC)
    message(FATAL_ERROR "ARM GCC compiler not found")
endif()

find_program(ARM_GXX ${CROSS_COMPILE}g++)
if(NOT ARM_GXX)
    message(FATAL_ERROR "ARM G++ compiler not found")
endif()

find_program(ARM_LD ${CROSS_COMPILE}ld)
if(NOT ARM_LD)
    message(FATAL_ERROR "ARM linker not found")
endif()

find_program(ARM_OBJDUMP ${CROSS_COMPILE}objdump)
if(NOT ARM_OBJDUMP)
    message(FATAL_ERROR "ARM objdump not found")
endif()

find_program(ARM_OBJCOPY ${CROSS_COMPILE}objcopy)
if(NOT ARM_OBJCOPY)
    message(FATAL_ERROR "ARM objcopy not found")
endif()

find_program(ARM_AR ${CROSS_COMPILE}ar)
if(NOT ARM_AR)
    message(FATAL_ERROR "ARM ar not found")
endif()

find_program(ARM_SIZE ${CROSS_COMPILE}size)
if(NOT ARM_SIZE)
    message(FATAL_ERROR "ARM size not found")
endif()

find_program(ARM_GDB ${CROSS_COMPILE}gdb)
if(NOT ARM_GDB)
    find_program(ARM_GDB gdb-multiarch)
    if(NOT ARM_GDB)
        message(FATAL_ERROR "ARM GDB not found (tried ${CROSS_COMPILE}gdb and gdb-multiarch)")
    else()
        message(STATUS "Using gdb-multiarch for ARM debugging")
    endif()
endif()

# ==============================================================================
#                         CMake Configuration
# ==============================================================================
set(DMOD_ARCH "armv7-cortex-a53" CACHE STRING "Target architecture")
set(DMOD_CPU "cortex-a53" CACHE STRING "Target CPU")
set(COMMON_DEFINE_FLAGS "-DDMOD_ARCH=\\\"${DMOD_ARCH}\\\" -DDMOD_CPU=\\\"${DMOD_CPU}\\\"")
set(FPU_FLAGS "-mfpu=neon-fp-armv8 -mfloat-abi=hard" CACHE STRING "FPU configuration flags")
set(CPUCONFIG_CFLAGS "-mcpu=cortex-a53 -marm ${COMMON_DEFINE_FLAGS} ${FPU_FLAGS}" CACHE STRING "C compiler flags")
set(CPUCONFIG_CXXFLAGS "-mcpu=cortex-a53 -marm ${COMMON_DEFINE_FLAGS} ${FPU_FLAGS}" CACHE STRING "C++ compiler flags")
set(CPUCONFIG_ASMFLAGS "-mcpu=cortex-a53 -marm ${COMMON_DEFINE_FLAGS} ${FPU_FLAGS}" CACHE STRING "ASM compiler flags")
set(CPUCONFIG_LDFLAGS "-mcpu=cortex-a53 -marm -Wl,--gc-sections ${FPU_FLAGS}" CACHE STRING "Linker flags")
set(CMAKE_C_COMPILER "${ARM_GCC}" CACHE STRING "C compiler")
set(CMAKE_CXX_COMPILER "${ARM_GXX}" CACHE STRING "C++ compiler")
set(CMAKE_LINKER "${ARM_LD}" CACHE STRING "Linker")
set(CMAKE_OBJDUMP "${ARM_OBJDUMP}" CACHE STRING "Objdump")
set(CMAKE_OBJCOPY "${ARM_OBJCOPY}" CACHE STRING "Objcopy")
set(CMAKE_SIZE "${ARM_SIZE}" CACHE STRING "Size")
set(CMAKE_AR "${ARM_AR}" CACHE STRING "Archiver")
set(CMAKE_GDB "${ARM_GDB}" CACHE STRING "GDB")
set(MAKE make CACHE STRING "Make")
set(MKDIR mkdir CACHE STRING "Mkdir")
set(RM rm CACHE STRING "Rm")
set(CMAKE_C_FLAGS "-Wall -std=c11 ${CPUCONFIG_CFLAGS}" CACHE STRING "C compiler flags")
set(CMAKE_CXX_FLAGS "-Wall -std=c++17 ${CPUCONFIG_CXXFLAGS}" CACHE STRING "C++ compiler flags")
set(CMAKE_ASM_FLAGS "${CPUCONFIG_ASMFLAGS}" CACHE STRING "ASM compiler flags")
set(CMAKE_ASM_FLAGS_DEBUG "${CPUCONFIG_ASMFLAGS} -g" CACHE STRING "ASM compiler flags for Debug")
set(CMAKE_EXE_LINKER_FLAGS "${CPUCONFIG_LDFLAGS}" CACHE STRING "Linker flags")

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY" CACHE STRING "Try compile target type")
