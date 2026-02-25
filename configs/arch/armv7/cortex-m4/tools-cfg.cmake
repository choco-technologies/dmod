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
set(DMOD_USE_DIRENT             OFF )
set(DMOD_USE_TIME   	        OFF )
set(DMOD_BUILD_TESTS            OFF )
set(DMOD_BUILD_TOOLS            OFF )
if (DMOD_MODE STREQUAL "DMOD_SYSTEM")
    set(DMOD_BUILD_EXAMPLES         OFF )
endif()

#
#	Toolchain configuration
#
if(NOT DEFINED CROSS_COMPILE)
	set(CROSS_COMPILE arm-none-eabi-)
endif()

find_program(ARM_GCC arm-none-eabi-gcc)
if(NOT ARM_GCC)
    message(FATAL_ERROR "ARM GCC compiler not found")
endif()

find_program(ARM_GXX arm-none-eabi-g++)
if(NOT ARM_GXX)
    message(FATAL_ERROR "ARM G++ compiler not found")
endif()

find_program(ARM_LD arm-none-eabi-ld)
if(NOT ARM_LD)
    message(FATAL_ERROR "ARM linker not found")
endif()

find_program(ARM_OBJDUMP arm-none-eabi-objdump)
if(NOT ARM_OBJDUMP)
    message(FATAL_ERROR "ARM objdump not found")
endif()

find_program(ARM_OBJCOPY arm-none-eabi-objcopy)
if(NOT ARM_OBJCOPY)
    message(FATAL_ERROR "ARM objcopy not found")
endif()

find_program(ARM_AR arm-none-eabi-ar)
if(NOT ARM_AR)
    message(FATAL_ERROR "ARM ar not found")
endif()

find_program(ARM_SIZE arm-none-eabi-size)
if(NOT ARM_SIZE)
    message(FATAL_ERROR "ARM size not found")
endif()

find_program(ARM_GDB arm-none-eabi-gdb)
if(NOT ARM_GDB)
    message(FATAL_ERROR "ARM GDB not found")
endif()

# ==============================================================================
#                         CMake Configuration
# ==============================================================================
set(DMOD_ARCH "armv7-cortex-m4" CACHE STRING "Target architecture")
set(DMOD_CPU "cortex-m4" CACHE STRING "Target CPU")
set(COMMON_DEFINE_FLAGS "-DDMOD_ARCH=\\\"${DMOD_ARCH}\\\" -DDMOD_CPU=\\\"${DMOD_CPU}\\\"")
set(FPU_FLAGS "-mfpu=fpv4-sp-d16 -mfloat-abi=hard" CACHE STRING "FPU configuration flags")
set(CPUCONFIG_CFLAGS "-mcpu=cortex-m4 -mthumb -mno-unaligned-access -DGCC_ARMCM4 ${COMMON_DEFINE_FLAGS} -mpic-data-is-text-relative -mabi=aapcs ${FPU_FLAGS}" CACHE STRING "C compiler flags")
set(CPUCONFIG_CXXFLAGS "-mcpu=cortex-m4 -mthumb -mno-unaligned-access -DGCC_ARMCM4 ${COMMON_DEFINE_FLAGS} -mpic-data-is-text-relative -mabi=aapcs ${FPU_FLAGS}" CACHE STRING "C++ compiler flags")
set(CPUCONFIG_ASMFLAGS "-mcpu=cortex-m4 -mthumb -mno-unaligned-access -DGCC_ARMCM4 ${COMMON_DEFINE_FLAGS} -mpic-data-is-text-relative -mabi=aapcs ${FPU_FLAGS}" CACHE STRING "ASM compiler flags")
set(CPUCONFIG_LDFLAGS "-mcpu=cortex-m4 -mthumb -mno-unaligned-access -Wl,--gc-sections -Wl,-static -mabi=aapcs ${FPU_FLAGS}" CACHE STRING "Linker flags")
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