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
    find_program(ARM_GDB gdb-multiarch)
    if(NOT ARM_GDB)
        message(FATAL_ERROR "ARM GDB not found (tried arm-none-eabi-gdb and gdb-multiarch)")
    else()
        message(STATUS "Using gdb-multiarch for ARM debugging")
    endif()
endif()

# ==============================================================================
#                         CMake Configuration
# ==============================================================================
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(OPTIMIZATION_FLAGS "-Og" CACHE STRING "Optimization flags" FORCE)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(OPTIMIZATION_FLAGS "-O0" CACHE STRING "Optimization flags" FORCE)
else()
    set(OPTIMIZATION_FLAGS "-O0" CACHE STRING "Optimization flags" FORCE)
endif()

set(DMOD_ARCH "armv7-cortex-m7" CACHE STRING "Target architecture")
set(DMOD_CPU "cortex-m7" CACHE STRING "Target CPU")
set(COMMON_DEFINE_FLAGS "-DDMOD_ARCH=\\\"${DMOD_ARCH}\\\" -DDMOD_CPU=\\\"${DMOD_CPU}\\\"")
set(FPU_FLAGS "-mfpu=fpv5-sp-d16 -mfloat-abi=hard" CACHE STRING "FPU configuration flags")
set(CPUCONFIG_CFLAGS "-mcpu=cortex-m7 -mthumb -mno-unaligned-access -DGCC_ARMCM7 ${COMMON_DEFINE_FLAGS} -mpic-data-is-text-relative -mabi=aapcs ${FPU_FLAGS}" CACHE STRING "C compiler flags")
set(CPUCONFIG_CXXFLAGS "-mcpu=cortex-m7 -mthumb -mno-unaligned-access -DGCC_ARMCM7 ${COMMON_DEFINE_FLAGS} -mpic-data-is-text-relative -mabi=aapcs ${FPU_FLAGS}" CACHE STRING "C++ compiler flags")
set(CPUCONFIG_ASMFLAGS "-mcpu=cortex-m7 -mthumb -mno-unaligned-access -DGCC_ARMCM7 ${COMMON_DEFINE_FLAGS} -mpic-data-is-text-relative -mabi=aapcs ${FPU_FLAGS}" CACHE STRING "ASM compiler flags")
set(CPUCONFIG_LDFLAGS "-mcpu=cortex-m7 -mthumb -mno-unaligned-access -Wl,--gc-sections -Wl,-static -mabi=aapcs ${FPU_FLAGS}" CACHE STRING "Linker flags")
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
set(CMAKE_C_FLAGS "${OPTIMIZATION_FLAGS} -Wall -std=c11 ${CPUCONFIG_CFLAGS}" CACHE STRING "C compiler flags")
set(CMAKE_C_FLAGS_DEBUG "-Og -g" CACHE STRING "Flags used by the C compiler during DEBUG builds." FORCE)
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "Flags used by the C compiler during RELEASE builds." FORCE)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG" CACHE STRING "Flags used by the C compiler during RELWITHDEBINFO builds." FORCE)
set(CMAKE_CXX_FLAGS "${OPTIMIZATION_FLAGS} -Wall -std=c++17 ${CPUCONFIG_CXXFLAGS}" CACHE STRING "C++ compiler flags")
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g" CACHE STRING "Flags used by the CXX compiler during DEBUG builds." FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "Flags used by the CXX compiler during RELEASE builds." FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG" CACHE STRING "Flags used by the CXX compiler during RELWITHDEBINFO builds." FORCE)
set(CMAKE_ASM_FLAGS "${OPTIMIZATION_FLAGS} ${CPUCONFIG_ASMFLAGS}" CACHE STRING "ASM compiler flags")
set(CMAKE_ASM_FLAGS_DEBUG "${CPUCONFIG_ASMFLAGS} -Og -g" CACHE STRING "ASM compiler flags for Debug" FORCE)
set(CMAKE_ASM_FLAGS_RELEASE "${CPUCONFIG_ASMFLAGS} -O2" CACHE STRING "ASM compiler flags for Release" FORCE)
set(CMAKE_ASM_FLAGS_RELWITHDEBINFO "${CPUCONFIG_ASMFLAGS} -O2 -g" CACHE STRING "ASM compiler flags for RelWithDebInfo" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "${CPUCONFIG_LDFLAGS}" CACHE STRING "Linker flags")

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY" CACHE STRING "Try compile target type")