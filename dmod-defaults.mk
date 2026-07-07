ifndef DMOD_USE_STDLIB
    DMOD_USE_STDLIB = ON
endif
ifndef DMOD_USE_GETENV
    DMOD_USE_GETENV = ON
endif
ifndef DMOD_USE_ENVIRON
    DMOD_USE_ENVIRON = ON
endif

ifndef DMOD_USE_STDIO
    DMOD_USE_STDIO = ON
endif

ifndef DMOD_IMPLEMENT_PRINTF
    # Use custom printf implementation when STDIO is not available
    ifneq ($(DMOD_USE_STDIO), ON)
        DMOD_IMPLEMENT_PRINTF = ON
    else
        DMOD_IMPLEMENT_PRINTF = OFF
    endif
endif

ifndef DMOD_IMPLEMENT_SCANF
    # Use custom scanf implementation when STDIO is not available
    ifneq ($(DMOD_USE_STDIO), ON)
        DMOD_IMPLEMENT_SCANF = ON
    else
        DMOD_IMPLEMENT_SCANF = OFF
    endif
endif

ifndef DMOD_USE_DIRENT
    DMOD_USE_DIRENT = ON
endif

ifndef DMOD_USE_ASSERT
    DMOD_USE_ASSERT = ON
endif

ifndef DMOD_USE_PTHREAD
    DMOD_USE_PTHREAD = ON
endif

ifndef DMOD_USE_MMAN
    DMOD_USE_MMAN = ON
endif

ifndef DMOD_USE_ALIGNED_ALLOC
    DMOD_USE_ALIGNED_ALLOC = ON
endif
ifndef DMOD_USE_ALIGNED_MALLOC_MOCK
    ifneq ($(DMOD_USE_ALIGNED_ALLOC), ON)
        DMOD_USE_ALIGNED_MALLOC_MOCK=ON
    else
        DMOD_USE_ALIGNED_MALLOC_MOCK=OFF
    endif
endif

ifndef DMOD_USE_REALLOC
    DMOD_USE_REALLOC = ON
endif

ifndef DMOD_USE_TERMIOS
    DMOD_USE_TERMIOS = ON
endif

ifndef DMOD_USE_TIME
    DMOD_USE_TIME = ON
endif

ifndef DMOD_USE_FASTLZ
    DMOD_USE_FASTLZ = ON
endif

ifndef DMOD_MAX_MODULES
    DMOD_MAX_MODULES = 30
endif

ifndef DMOD_MAX_REQUIRED_MODULES
    DMOD_MAX_REQUIRED_MODULES = 10
endif

ifndef DMOD_VFPRINTF_STACK_BUFFER_SIZE
    # Above this size, Dmod_VFPrintf falls back to a heap allocation instead of a stack buffer
    DMOD_VFPRINTF_STACK_BUFFER_SIZE = 100
endif

ifndef DMOD_MODE
    DMOD_MODE = DMOD_SYSTEM
endif

ifndef DMOD_SYSTEM_VERSION_MAJOR
    DMOD_SYSTEM_VERSION_MAJOR = 0
endif

ifndef DMOD_SYSTEM_VERSION_MINOR
    DMOD_SYSTEM_VERSION_MINOR = 1
endif

ifndef DMOD_BUILD_TESTS
    DMOD_BUILD_TESTS = ON
endif

ifndef DMOD_BUILD_EXAMPLES
    DMOD_BUILD_EXAMPLES = ON
endif

ifndef DMOD_BUILD_TOOLS
    DMOD_BUILD_TOOLS = ON
endif

ifndef DMOD_USE_EXCEPTIONS
    DMOD_USE_EXCEPTIONS = OFF
endif