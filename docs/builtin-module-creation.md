# Creating New Builtin Modules

## Overview

This document explains how to create new builtin API functions in the DMOD system. Builtin APIs are functions that are part of the core system and are available to all modules loaded into the system.

When creating builtin modules, you need to understand three key macros:
- `DMOD_BUILTIN_API` - Declares a builtin API function
- `DMOD_ENABLE_REGISTRATION` - Enables API registration in source files
- `DMOD_EXTERNAL_REGISTRATION` - Indicates that registration is handled externally

## The `DMOD_BUILTIN_API` Macro

The `DMOD_BUILTIN_API` macro is used to declare API functions that are part of the DMOD system. The behavior of this macro depends on the build mode:

- **SYSTEM mode** (`DMOD_SYSTEM_EN == ON`): The macro expands to `DMOD_INPUT_API`, which registers the function as an input API that modules can call.
- **MODULE mode** (`DMOD_MODULE_EN == ON`): The macro expands to `DMOD_OUTPUT_API`, which creates a function pointer that will be resolved at runtime when the module connects to the system.

### Syntax

```c
DMOD_BUILTIN_API( MODULE, VERSION, RET, NAME, PARAMS )
```

### Parameters

| Parameter | Description |
|-----------|-------------|
| `MODULE` | Module name/group the API function belongs to (e.g., `Dmod`, `MyModule`) |
| `VERSION` | API version for compatibility management (e.g., `1.0`) |
| `RET` | Return type of the function |
| `NAME` | Function name (will be prefixed with module name) |
| `PARAMS` | Function parameters enclosed in parentheses |

### Example

```c
// Declare a builtin API function
DMOD_BUILTIN_API( MyModule, 1.0, bool, _DoSomething, (int arg1, const char* arg2) );
```

This creates a function accessible as `MyModule_DoSomething(int arg1, const char* arg2)`.

## The `DMOD_ENABLE_REGISTRATION` Macro

The `DMOD_ENABLE_REGISTRATION` macro must be defined **before** including any DMOD headers in source files that implement builtin API functions. This macro enables the actual registration of API functions in the `.dmod.inputs` linker section.

### How It Works

When `DMOD_ENABLE_REGISTRATION` is defined:
- The `_DMOD_API_REGISTRATION` macro creates actual registration entries
- These entries are placed in the `.dmod.inputs` linker section
- The DMOD system can discover and connect these APIs at runtime

When `DMOD_ENABLE_REGISTRATION` is **not** defined:
- The registration macro only creates external declarations
- No actual registration entries are created
- This is useful for header files that declare but don't implement APIs

### Usage

```c
// IMPORTANT: Define BEFORE including any DMOD headers
#define DMOD_ENABLE_REGISTRATION    ON

#include "dmod.h"
#include "my_custom_api.h"  // Your custom API declarations

// Now implement your API functions...
```

## The `DMOD_EXTERNAL_REGISTRATION` Macro

The `DMOD_EXTERNAL_REGISTRATION` macro controls whether `dmod_system.c` automatically enables API registration internally.

### How It Works

By default, when you compile the DMOD system, `dmod_system.c` contains this logic at the top:

```c
#ifndef DMOD_EXTERNAL_REGISTRATION
#   define DMOD_ENABLE_REGISTRATION
#endif
```

This means:
- If `DMOD_EXTERNAL_REGISTRATION` is **not defined**: `dmod_system.c` will define `DMOD_ENABLE_REGISTRATION` and register all the standard DMOD builtin APIs (like `Dmod_LoadModule`, `Dmod_EnableModule`, etc.)
- If `DMOD_EXTERNAL_REGISTRATION` **is defined**: `dmod_system.c` will **not** define `DMOD_ENABLE_REGISTRATION`, meaning the standard APIs won't be registered in `dmod_system.c`. You must then handle registration in your own source files.

### When to Use

Use `DMOD_EXTERNAL_REGISTRATION` when:
- You want to implement builtin APIs in your own source files
- You need to control exactly where API registrations occur
- You're extending the system with custom builtin modules

## Step-by-Step Guide

### Step 1: Create Your API Header File

Create a header file that declares your builtin API functions:

```c
// my_builtin_api.h
#ifndef MY_BUILTIN_API_H
#define MY_BUILTIN_API_H

#include "dmod.h"

// Declare your builtin API functions
DMOD_BUILTIN_API( MyApi, 1.0, bool, _Initialize, (void) );
DMOD_BUILTIN_API( MyApi, 1.0, void, _Process, (const void* data, size_t size) );
DMOD_BUILTIN_API( MyApi, 1.0, int, _GetStatus, (void) );

#endif // MY_BUILTIN_API_H
```

### Step 2: Create Your Implementation File

Create a source file that implements your API functions. **Important**: Define `DMOD_ENABLE_REGISTRATION` before any includes!

**Note on macros:**
- `DMOD_BUILTIN_API` is used in **header files** to declare API functions
- `DMOD_INPUT_API_DECLARATION` is used in **source files** to implement those API functions

```c
// my_builtin_api.c

// CRITICAL: Define this BEFORE including any headers!
#define DMOD_ENABLE_REGISTRATION    ON

// Now include your headers
#include "dmod.h"
#include "my_builtin_api.h"

// Implement your API functions using DMOD_INPUT_API_DECLARATION
// This macro creates the function definition with the proper naming
DMOD_INPUT_API_DECLARATION( MyApi, 1.0, bool, _Initialize, (void) )
{
    // Your initialization code here
    return true;
}

DMOD_INPUT_API_DECLARATION( MyApi, 1.0, void, _Process, (const void* data, size_t size) )
{
    // Your processing code here
}

DMOD_INPUT_API_DECLARATION( MyApi, 1.0, int, _GetStatus, (void) )
{
    // Return current status
    return 0;
}
```

### Step 3: Configure Your Build System

Add your source file to the DMOD system build. If building with CMake:

```cmake
# Add your builtin API implementation to the dmod library
target_sources(dmod PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/my_builtin_api.c
)

# Add include directory if needed
target_include_directories(dmod PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
```

### Step 4: Use Your API in Modules

Once the system is built with your builtin APIs, modules can use them:

```c
// In a module source file
#include "dmod.h"
#include "my_builtin_api.h"

int main(int argc, char* argv[])
{
    // Call your builtin API functions
    if (MyApi_Initialize())
    {
        MyApi_Process(data, sizeof(data));
        int status = MyApi_GetStatus();
    }
    return 0;
}
```

## Important Considerations

### Order of Definitions

The order of macro definitions is critical:

```c
// CORRECT ORDER:
#define DMOD_ENABLE_REGISTRATION    ON
#include "dmod.h"
#include "my_api.h"

// INCORRECT ORDER (will not work):
#include "dmod.h"
#define DMOD_ENABLE_REGISTRATION    ON  // Too late!
#include "my_api.h"
```

### Single Registration Point

Each API function should be registered in exactly **one** source file. If you define `DMOD_ENABLE_REGISTRATION` in multiple source files that include the same API declaration header (e.g., both `file_a.c` and `file_b.c` include `my_builtin_api.h` with `DMOD_ENABLE_REGISTRATION` defined), you'll get linker errors due to duplicate symbols in the `.dmod.inputs` section.

**Solution**: Only define `DMOD_ENABLE_REGISTRATION` in the source file that provides the actual function implementations.

### Using DMOD_EXTERNAL_REGISTRATION

If you're adding builtin APIs to an existing system and want to handle all registration externally (outside of `dmod_system.c`), you have several options:

**Option 1: Define as a compiler flag in your build system**

```cmake
# CMake
target_compile_definitions(dmod PRIVATE DMOD_EXTERNAL_REGISTRATION)
```

```makefile
# Makefile
CFLAGS += -DDMOD_EXTERNAL_REGISTRATION
```

**Option 2: Define in a common configuration header that's included before `dmod_system.c` is compiled**

Create a project-wide configuration header (e.g., `project_config.h`) and ensure it's included:

```c
// project_config.h
#define DMOD_EXTERNAL_REGISTRATION
```

**Then in your source file that implements the APIs:**

```c
#define DMOD_ENABLE_REGISTRATION    ON
#include "dmod.h"
// ... your API implementations
```

## Example: Complete Builtin Module

Here's a complete example of a builtin logging module:

### Header (builtin_log.h)

```c
#ifndef BUILTIN_LOG_H
#define BUILTIN_LOG_H

#include "dmod.h"

// Log levels
typedef enum {
    LogLevel_Debug = 0,
    LogLevel_Info,
    LogLevel_Warning,
    LogLevel_Error
} BuiltinLogLevel_t;

// Builtin logging API
DMOD_BUILTIN_API( Log, 1.0, void, _SetLevel, (BuiltinLogLevel_t level) );
DMOD_BUILTIN_API( Log, 1.0, void, _Debug, (const char* message) );
DMOD_BUILTIN_API( Log, 1.0, void, _Info, (const char* message) );
DMOD_BUILTIN_API( Log, 1.0, void, _Warning, (const char* message) );
DMOD_BUILTIN_API( Log, 1.0, void, _Error, (const char* message) );

#endif // BUILTIN_LOG_H
```

### Implementation (builtin_log.c)

```c
// Define BEFORE any includes!
#define DMOD_ENABLE_REGISTRATION    ON

#include "dmod.h"
#include "builtin_log.h"

// Internal state
static BuiltinLogLevel_t currentLevel = LogLevel_Info;

DMOD_INPUT_API_DECLARATION( Log, 1.0, void, _SetLevel, (BuiltinLogLevel_t level) )
{
    currentLevel = level;
}

DMOD_INPUT_API_DECLARATION( Log, 1.0, void, _Debug, (const char* message) )
{
    if (currentLevel <= LogLevel_Debug)
    {
        Dmod_Printf("[DEBUG] %s\n", message);
    }
}

DMOD_INPUT_API_DECLARATION( Log, 1.0, void, _Info, (const char* message) )
{
    if (currentLevel <= LogLevel_Info)
    {
        Dmod_Printf("[INFO] %s\n", message);
    }
}

DMOD_INPUT_API_DECLARATION( Log, 1.0, void, _Warning, (const char* message) )
{
    if (currentLevel <= LogLevel_Warning)
    {
        Dmod_Printf("[WARNING] %s\n", message);
    }
}

DMOD_INPUT_API_DECLARATION( Log, 1.0, void, _Error, (const char* message) )
{
    if (currentLevel <= LogLevel_Error)
    {
        Dmod_Printf("[ERROR] %s\n", message);
    }
}
```

### Usage in Module

```c
#include "dmod.h"
#include "builtin_log.h"

int main(int argc, char* argv[])
{
    Log_SetLevel(LogLevel_Debug);
    Log_Info("Application started");
    Log_Debug("Debug information");
    Log_Warning("A warning message");
    Log_Error("An error occurred");
    return 0;
}
```

## See Also

- [README.md](../README.md) - Main DMOD documentation
- [MEMORY_ACCESS.md](MEMORY_ACCESS.md) - Memory access functions documentation
- [tools-installation.md](tools-installation.md) - DMOD tools installation guide
