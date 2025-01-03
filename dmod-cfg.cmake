# ===========================================================================
# 						Configuration options
# ===========================================================================

if (NOT DEFINED DMOD_USE_STDLIB)
    set(DMOD_USE_STDLIB ON)
endif()

if (NOT DEFINED DMOD_USE_STDIO)
    set(DMOD_USE_STDIO ON)
endif()
if (NOT DEFINED DMOD_USE_ASSERT)
    set(DMOD_USE_ASSERT ON)
endif()
if (NOT DEFINED DMOD_USE_PTHREAD)
    set(DMOD_USE_PTHREAD ON)
endif()
if (NOT DEFINED DMOD_USE_MMAN)
    set(DMOD_USE_MMAN ON)
endif()

if (NOT DEFINED DMOD_MAX_MODULES)
    set(DMOD_MAX_MODULES 30)
endif()

if (NOT DEFINED DMOD_MAX_REQUIRED_MODULES)
    set(DMOD_MAX_REQUIRED_MODULES 10)
endif()

if (NOT DEFINED DMOD_MODE)
    set(DMOD_MODE "DMOD_SYSTEM")
endif()

if (NOT DEFINED DMOD_SYSTEM_VERSION_MAJOR)
    set(DMOD_SYSTEM_VERSION_MAJOR 0)
endif()

if (NOT DEFINED DMOD_SYSTEM_VERSION_MINOR)
    set(DMOD_SYSTEM_VERSION_MINOR 1)
endif()

if (NOT DEFINED DMOD_BUILD_TESTS)
    set(DMOD_BUILD_TESTS ON)
endif()

if (NOT DEFINED DMOD_BUILD_EXAMPLES)
    set(DMOD_BUILD_EXAMPLES ON)
endif()

set(DMOD_DMF_DIR                "${CMAKE_CURRENT_BINARY_DIR}/dmf" CACHE STRING "Directory for DMF files")
set(DMOD_REPO_DIR               "${DMOD_DMF_DIR}" 				  CACHE STRING "Directory for DMF files inside the system")
set(DMOD_CPU_NAME			    "" 						  	      CACHE STRING "Name of the target cpu, if empty, the target is generic")
set(DMOD_TOOLS_NAME			    "arch/x86_64" 					  CACHE STRING "Name of the tools configuration")
