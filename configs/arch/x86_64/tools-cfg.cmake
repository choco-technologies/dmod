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



#
#	Toolchain configuration
#
if(NOT DEFINED CROSS_COMPILE)
	set(CROSS_COMPILE "")
endif()

find_program(GCC ${CROSS_COMPILE}gcc)
if(NOT GCC)
    message(FATAL_ERROR "GCC compiler not found")
endif()

find_program(GXX ${CROSS_COMPILE}g++)
if(NOT GXX)
    message(FATAL_ERROR "G++ compiler not found")
endif()

find_program(LD ${CROSS_COMPILE}ld)
if(NOT LD)
    message(FATAL_ERROR "Linker not found")
endif()

find_program(OBJDUMP ${CROSS_COMPILE}objdump)
if(NOT OBJDUMP)
    message(FATAL_ERROR "objdump not found")
endif()

find_program(OBJCOPY ${CROSS_COMPILE}objcopy)
if(NOT OBJCOPY)
    message(FATAL_ERROR "objcopy not found")
endif()

find_program(AR ${CROSS_COMPILE}ar)
if(NOT AR)
    message(FATAL_ERROR "ar not found")
endif()

find_program(SIZE ${CROSS_COMPILE}size)
if(NOT SIZE)
    message(FATAL_ERROR "size not found")
endif()

# ==============================================================================
#                         CMake Configuration
# ==============================================================================
set(CMAKE_C_COMPILER "${GCC}" CACHE STRING "C compiler")
set(CMAKE_CXX_COMPILER "${GXX}" CACHE STRING "C++ compiler")
set(CMAKE_LINKER "${LD}" CACHE STRING "Linker")
set(CMAKE_OBJDUMP "${OBJDUMP}" CACHE STRING "Objdump")
set(CMAKE_OBJCOPY "${OBJCOPY}" CACHE STRING "Objcopy")
set(CMAKE_SIZE "${SIZE}" CACHE STRING "Size")
set(CMAKE_AR "${AR}" CACHE STRING "Archiver")
set(CMAKE_GDB "${GDB}" CACHE STRING "GDB")
set(MAKE make CACHE STRING "Make")
set(MKDIR mkdir CACHE STRING "Mkdir")
set(RM rm CACHE STRING "Rm")
set(CMAKE_C_FLAGS "-Wall -std=c11 ${CPUCONFIG_CFLAGS}")
set(CMAKE_CXX_FLAGS "-Wall -std=c++17 ${CPUCONFIG_CXXFLAGS}")
set(CMAKE_LFLAGS "${CPUCONFIG_LDFLAGS}")