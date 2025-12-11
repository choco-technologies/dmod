# CMake Functions Reference

This document describes the CMake functions provided by DMOD for module development.

## Module Creation Functions

### `dmod_add_executable(moduleName version sources...)`

Creates a DMOD executable module.

**Parameters:**
- `moduleName` - Name of the module
- `version` - Module version (e.g., "1.0", "2.3.1")
- `sources...` - List of source files

**Example:**
```cmake
set(DMOD_MODULE_NAME        my_app)
set(DMOD_MODULE_VERSION     "1.0")
set(DMOD_AUTHOR_NAME        "John Doe")
set(DMOD_STACK_SIZE         2048)
set(DMOD_PRIORITY           0)

dmod_add_executable(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION} 
    main.c
    app_logic.c
)
```

### `dmod_add_library(moduleName version sources...)`

Creates a DMOD library module.

**Parameters:**
- `moduleName` - Name of the module
- `version` - Module version (e.g., "1.0", "2.3.1")
- `sources...` - List of source files

**Example:**
```cmake
set(DMOD_MODULE_NAME        my_lib)
set(DMOD_MODULE_VERSION     "2.0")
set(DMOD_AUTHOR_NAME        "Jane Smith")

dmod_add_library(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION} 
    library.c
    utils.c
)
```

## Dependency Management Functions

### `dmod_link_modules(targetName modules...)`

Downloads headers for external DMOD modules and links them to your target. This function uses the `dmf-get` tool to download module headers at CMake configuration time and automatically adds them to the target's include directories.

**Parameters:**
- `targetName` - Name of the target (must be created with `dmod_add_executable` or `dmod_add_library` first)
- `modules...` - List of module specifications with optional versions

**Module Specification Format:**
- `module_name` - Download the latest version of the module
- `module_name@version` - Download a specific version of the module (e.g., `dmlink@1.0`)

**Example:**
```cmake
# Create your module first
dmod_add_executable(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION} 
    main.c
)

# Link external modules - download their headers and add to include path
dmod_link_modules(${DMOD_MODULE_NAME}
    dmlink@1.0      # Specific version
    dmffs           # Latest version
    driver@2.3      # Another specific version
)
```

**How It Works:**

1. **Finds dmf-get tool**: The function searches for the `dmf-get` executable in `${DMOD_TOOLS_BIN_DIR}` or system PATH
2. **Downloads headers**: For each module, it executes `dmf-get headers <module_spec> -o <output_dir>`
3. **Adds include directories**: The downloaded headers (located at `${DMOD_DMF_DIR}/<module_name>/inc`) are automatically added to the target's private include directories

**Output Directory:**

Headers are downloaded to: `${DMOD_DMF_DIR}/<module_name>/inc`

By default, `DMOD_DMF_DIR` is set to `${CMAKE_BINARY_DIR}/dmf`.

**Requirements:**

- The `dmf-get` tool must be built and available (build with `-DDMOD_BUILD_TOOLS=ON`)
- A valid manifest file must be accessible (either specified via environment variables or using the default registry)
- The modules must exist in the configured registry

**Environment Variables:**

The function respects the following environment variables (same as `dmf-get`):
- `DMOD_DMF_DIR` - DMF directory for downloaded headers
- `DMOD_MANIFEST` - Path or URL to the manifest file
- `DMOD_TOOLS_NAME` - Tools name for platform-specific modules

**Error Handling:**

- If `dmf-get` is not found, a warning is issued and the function returns without error
- If a module download fails, a warning is issued and the function continues with remaining modules
- The build continues even if some headers cannot be downloaded

**Notes:**

- This function should be called **after** `dmod_add_executable` or `dmod_add_library`
- Headers are downloaded during CMake configuration, not during build
- If you need to update headers, re-run CMake configuration
- The function only downloads headers (not the full module files)

## Tool Creation Functions

### `dmod_add_tool(toolName sources...)`

Creates a system tool executable.

**Parameters:**
- `toolName` - Name of the tool
- `sources...` - List of source files

**Example:**
```cmake
dmod_add_tool(my_tool
    main.c
    tool_logic.c
)
```

## Utility Functions

### `create_library_makefile(targetName)`

Creates a Makefile for building a static library target. This is automatically called for library targets and is used for Make-based builds.

### `dmod_setup_external_module()`

Sets up DMOD libraries for external modules. Call this macro after `project()` in external module projects.

**Example:**
```cmake
cmake_minimum_required(VERSION 3.18)
project(my_external_module)

# Include DMOD paths
set(DMOD_DIR "/path/to/dmod")
include(${DMOD_DIR}/paths.cmake)

# Setup DMOD for external module
dmod_setup_external_module()

# Now you can use DMOD functions
dmod_add_executable(my_module "1.0"
    main.c
)
```

## Module Configuration Variables

The following variables should be set before calling `dmod_add_executable` or `dmod_add_library`:

### Required Variables

- `DMOD_MODULE_NAME` - Name of the module
- `DMOD_MODULE_VERSION` - Module version string
- `DMOD_AUTHOR_NAME` - Author name

### Optional Variables

- `DMOD_STACK_SIZE` - Stack size for the module (default: 1024)
- `DMOD_PRIORITY` - Module priority (default: 1)
- `DMOD_MANUAL_LOAD` - Manual load flag (default: OFF)
- `DMOD_MAL_IMPLS` - List of MAL (Module Abstraction Layer) interfaces implemented
- `DMOD_DIF_IMPLS` - List of DIF (Device Interface) implementations
- `DMOD_COMPRESSION_METHOD` - Compression method for DMFC files (default: "fastlz")

## See Also

- [DMF-GET Tool Documentation](dmf-get-tool.md) - Package manager for DMOD modules
- [Module Debugging](module-debugging.md) - Debugging DMOD modules
- [Memory Analysis](memory-analysis.md) - Analyzing module memory usage
