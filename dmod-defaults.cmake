if (NOT DEFINED DMOD_USE_STDLIB)
    set(DMOD_USE_STDLIB ON)
endif()
if (NOT DEFINED DMOD_USE_GETENV)
    set(DMOD_USE_GETENV ON)
endif()
if (NOT DEFINED DMOD_USE_ENVIRON)
    set(DMOD_USE_ENVIRON ON)
endif()

if (NOT DEFINED DMOD_USE_STDIO)
    set(DMOD_USE_STDIO ON)
endif()

if (NOT DEFINED DMOD_IMPLEMENT_PRINTF)
    # Use custom printf implementation when STDIO is not available
    if(NOT DMOD_USE_STDIO)
        set(DMOD_IMPLEMENT_PRINTF ON)
    else()
        set(DMOD_IMPLEMENT_PRINTF OFF)
    endif()
endif()

if (NOT DEFINED DMOD_IMPLEMENT_SCANF)
    # Use custom scanf implementation when STDIO is not available
    if(NOT DMOD_USE_STDIO)
        set(DMOD_IMPLEMENT_SCANF ON)
    else()
        set(DMOD_IMPLEMENT_SCANF OFF)
    endif()
endif()

if (NOT DEFINED DMOD_USE_DIRENT)
    set(DMOD_USE_DIRENT ON)
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
if (NOT DEFINED DMOD_USE_ALIGNED_ALLOC)
    set(DMOD_USE_ALIGNED_ALLOC ON)
endif()
if (NOT DEFINED DMOD_USE_ALIGNED_MALLOC_MOCK)
    if(NOT DMOD_USE_ALIGNED_ALLOC)
        set(DMOD_USE_ALIGNED_MALLOC_MOCK ON)
    else()
        set(DMOD_USE_ALIGNED_MALLOC_MOCK OFF)
    endif()
endif()
if (NOT DEFINED DMOD_USE_REALLOC)
    set(DMOD_USE_REALLOC ON)
endif()

if (NOT DEFINED DMOD_USE_TERMIOS)
    set(DMOD_USE_TERMIOS ON)
endif()

if (NOT DEFINED DMOD_USE_TIME)
    set(DMOD_USE_TIME ON)
endif()

if (NOT DEFINED DMOD_USE_FASTLZ)
    set(DMOD_USE_FASTLZ ON)
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

if (NOT DEFINED DMOD_BUILD_TOOLS)
    set(DMOD_BUILD_TOOLS ON)
endif()

if (NOT DEFINED DMOD_BUILD_TEMPLATES)
    set(DMOD_BUILD_TEMPLATES ON)
endif()

if (NOT DEFINED DMOD_USE_EXCEPTIONS)
    set(DMOD_USE_EXCEPTIONS OFF)
endif()

if (NOT DEFINED DMOD_EXTERNAL_REGISTRATION)
    set(DMOD_EXTERNAL_REGISTRATION OFF)
endif()

set(DMOD_MIN_COVERAGE 40 CACHE STRING "Minimum code coverage percentage")


# ===========================================================================
# 						    Built-in API Configuration
# ===========================================================================

if (NOT DEFINED DMOD_BUILTIN_COMPRESSION_API)
    set(DMOD_BUILTIN_COMPRESSION_API ON)
endif()