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
set(DMOD_BUILD_EXAMPLES         OFF )

if (DMOD_MODE STREQUAL "DMOD_SYSTEM")
    set(DMOD_BUILD_EXAMPLES     OFF )
endif()

#
#	Toolchain configuration
#
if(NOT DEFINED CROSS_COMPILE)
	set(CROSS_COMPILE xtensa-esp32s3-elf-)
endif()

find_program(ESP32S3_GCC xtensa-esp32s3-elf-gcc)
if(NOT ESP32S3_GCC)
    message(FATAL_ERROR "ESP32-S3 GCC compiler not found")
endif()

find_program(ESP32S3_GXX xtensa-esp32s3-elf-g++)
if(NOT ESP32S3_GXX)
    message(FATAL_ERROR "ESP32-S3 G++ compiler not found")
endif()

find_program(ESP32S3_LD xtensa-esp32s3-elf-ld)
if(NOT ESP32S3_LD)
    message(FATAL_ERROR "ESP32-S3 linker not found")
endif()

find_program(ESP32S3_OBJDUMP xtensa-esp32s3-elf-objdump)
if(NOT ESP32S3_OBJDUMP)
    message(FATAL_ERROR "ESP32-S3 objdump not found")
endif()

find_program(ESP32S3_OBJCOPY xtensa-esp32s3-elf-objcopy)
if(NOT ESP32S3_OBJCOPY)
    message(FATAL_ERROR "ESP32-S3 objcopy not found")
endif()

find_program(ESP32S3_AR xtensa-esp32s3-elf-ar)
if(NOT ESP32S3_AR)
    message(FATAL_ERROR "ESP32-S3 ar not found")
endif()

find_program(ESP32S3_SIZE xtensa-esp32s3-elf-size)
if(NOT ESP32S3_SIZE)
    message(FATAL_ERROR "ESP32-S3 size not found")
endif()

# ============================================================================== 
#                         CMake Configuration
# ==============================================================================
set(DMOD_ARCH "xtensa-esp32s3" CACHE STRING "Target architecture")
set(DMOD_CPU "esp32s3" CACHE STRING "Target CPU")

if(NOT DEFINED IDF_PATH)
    if(DEFINED ENV{IDF_PATH} AND NOT "$ENV{IDF_PATH}" STREQUAL "")
        set(IDF_PATH "$ENV{IDF_PATH}" CACHE PATH "ESP-IDF path")
    else()
        set(IDF_PATH "/tools/esp-idf" CACHE PATH "ESP-IDF path")
    endif()
endif()

set(ESP_IDF_INCLUDE_FLAGS "")
if(EXISTS "${IDF_PATH}")
    file(GLOB_RECURSE ESP_IDF_INCLUDE_DIRS LIST_DIRECTORIES true
        "${IDF_PATH}/components/*/include"
        "${IDF_PATH}/components/*/*/include"
        "${IDF_PATH}/components/*/*/*/include"
    )
    list(APPEND ESP_IDF_INCLUDE_DIRS "${IDF_PATH}/components/esp_hw_support/include")
    list(REMOVE_DUPLICATES ESP_IDF_INCLUDE_DIRS)

    foreach(ESP_IDF_INCLUDE_DIR ${ESP_IDF_INCLUDE_DIRS})
        string(APPEND ESP_IDF_INCLUDE_FLAGS " -I${ESP_IDF_INCLUDE_DIR}")
    endforeach()
else()
    message(WARNING "ESP-IDF directory not found: ${IDF_PATH}. ESP-IDF headers will not be available.")
endif()

set(COMMON_DEFINE_FLAGS "-DDMOD_ARCH=\\\"${DMOD_ARCH}\\\" -DDMOD_CPU=\\\"${DMOD_CPU}\\\"")
set(CPUCONFIG_CFLAGS "-mlongcalls -mtext-section-literals -fstrict-volatile-bitfields -Wno-frame-address ${COMMON_DEFINE_FLAGS}${ESP_IDF_INCLUDE_FLAGS}" CACHE STRING "C compiler flags")
set(CPUCONFIG_CXXFLAGS "-mlongcalls -mtext-section-literals -fstrict-volatile-bitfields -Wno-frame-address ${COMMON_DEFINE_FLAGS}${ESP_IDF_INCLUDE_FLAGS}" CACHE STRING "C++ compiler flags")
set(CPUCONFIG_ASMFLAGS "-mlongcalls -mtext-section-literals -fstrict-volatile-bitfields -Wno-frame-address ${COMMON_DEFINE_FLAGS}${ESP_IDF_INCLUDE_FLAGS}" CACHE STRING "ASM compiler flags")
set(CPUCONFIG_LDFLAGS "-Wl,--gc-sections -Wl,-static -mlongcalls -mtext-section-literals" CACHE STRING "Linker flags")
set(CMAKE_C_COMPILER "${ESP32S3_GCC}" CACHE STRING "C compiler")
set(CMAKE_CXX_COMPILER "${ESP32S3_GXX}" CACHE STRING "C++ compiler")
set(CMAKE_LINKER "${ESP32S3_LD}" CACHE STRING "Linker")
set(CMAKE_OBJDUMP "${ESP32S3_OBJDUMP}" CACHE STRING "Objdump")
set(CMAKE_OBJCOPY "${ESP32S3_OBJCOPY}" CACHE STRING "Objcopy")
set(CMAKE_SIZE "${ESP32S3_SIZE}" CACHE STRING "Size")
set(CMAKE_AR "${ESP32S3_AR}" CACHE STRING "Archiver")
set(MAKE make CACHE STRING "Make")
set(MKDIR mkdir CACHE STRING "Mkdir")
set(RM rm CACHE STRING "Rm")
set(CMAKE_C_FLAGS "-Wall -std=c11 ${CPUCONFIG_CFLAGS}" CACHE STRING "C compiler flags")
set(CMAKE_CXX_FLAGS "-Wall -std=c++17 ${CPUCONFIG_CXXFLAGS}" CACHE STRING "C++ compiler flags")
set(CMAKE_ASM_FLAGS "${CPUCONFIG_ASMFLAGS}" CACHE STRING "ASM compiler flags")
set(CMAKE_ASM_FLAGS_DEBUG "${CPUCONFIG_ASMFLAGS} -g" CACHE STRING "ASM compiler flags for Debug")
set(CMAKE_EXE_LINKER_FLAGS "${CPUCONFIG_LDFLAGS}" CACHE STRING "Linker flags")

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY" CACHE STRING "Try compile target type")