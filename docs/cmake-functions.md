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

If `DMOD_DMR_PATH` is set before calling `dmod_add_library`, a release package will be created in `build/packages/` after the build completes:

```
build/
└── packages/
      ├── my_lib/
      │     ├── my_lib.dmf
      │     └── LICENSE.md
      └── my_lib.zip
```

### `dmod_add_test(moduleName version sources...)`

Creates a DMOD test module.

The test runner `main()` is provided automatically by the framework — do **not**
define `main()` in your test sources.  Test steps are registered with the
`DMOD_TEST_STEP()` macro from `dmod_test.h` and are discovered and executed
automatically at runtime.

The exit code of the resulting binary equals the number of failed steps, making
it suitable for use in CI pipelines.

**Parameters:**
- `moduleName` - Name of the test module
- `version` - Module version (e.g., "1.0")
- `sources...` - List of test source files (must **not** define `main()`)

**Example:**
```cmake
set(DMOD_MODULE_NAME        my_module_tests)
set(DMOD_MODULE_VERSION     "1.0")
set(DMOD_AUTHOR_NAME        "Jane Smith")
set(DMOD_STACK_SIZE         2048)

dmod_add_test(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION}
    test_feature_a.c
    test_feature_b.c
)
```

**Test source example:**
```c
#include "dmod_test.h"

/* Optional lifecycle hooks */
void dmod_test_setup(void)    { /* reset state */ }
void dmod_test_teardown(void) { /* cleanup    */ }

DMOD_TEST_STEP(addition_works)
{
    DMOD_TEST_EXPECT_EQ(1 + 1, 2);
}

DMOD_TEST_STEP(null_pointer_check)
{
    void* ptr = get_something();
    DMOD_TEST_EXPECT_NOT_NULL(ptr);
}
```

**Output example:**
```
=== DMOD Test Runner ===
[ RUN  ] addition_works
[  OK  ] addition_works
[ RUN  ] null_pointer_check
[  OK  ] null_pointer_check

=== Results: 2/2 passed ===
```

**Available assertion macros (from `dmod_test.h`):**

| Macro | Description |
|-------|-------------|
| `DMOD_TEST_EXPECT(cond)` | Fail if condition is false |
| `DMOD_TEST_EXPECT_TRUE(cond)` | Alias for `DMOD_TEST_EXPECT` |
| `DMOD_TEST_EXPECT_FALSE(cond)` | Fail if condition is true |
| `DMOD_TEST_EXPECT_EQ(a, b)` | Fail if `a != b` |
| `DMOD_TEST_EXPECT_NE(a, b)` | Fail if `a == b` |
| `DMOD_TEST_EXPECT_NULL(ptr)` | Fail if `ptr != NULL` |
| `DMOD_TEST_EXPECT_NOT_NULL(ptr)` | Fail if `ptr == NULL` |
| `DMOD_TEST_FAIL()` | Unconditionally fail |
| `DMOD_TEST_FAIL_MSG(msg, ...)` | Unconditionally fail with message |

## Dependency Management Functions

### `dmod_link_modules(targetName [PRIVATE|PUBLIC|INTERFACE] modules...)`

Downloads headers for external DMOD modules and links them to your target. This function uses the `dmf-get` tool to download module headers at CMake configuration time and automatically adds them to the target's include directories.

**Parameters:**
- `targetName` - Name of the target (must be created with `dmod_add_executable` or `dmod_add_library` first)
- `PRIVATE|PUBLIC|INTERFACE` - Optional visibility scope (default: PRIVATE if not specified)
  - `PRIVATE` - Headers available only to this target
  - `PUBLIC` - Headers available to this target and targets that link to it
  - `INTERFACE` - Headers available only to targets that link to this target
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

# Link external modules with default PRIVATE visibility
dmod_link_modules(${DMOD_MODULE_NAME}
    dmlink@1.0      # Specific version
    dmffs           # Latest version
)

# Or specify visibility scope explicitly
dmod_link_modules(${DMOD_MODULE_NAME}
    PUBLIC
        driver@2.3      # Public headers (available to dependents)
    PRIVATE
        internal_lib    # Private headers (only for this module)
)
```

**How It Works:**

1. **Finds dmf-get tool**: The function searches for the `dmf-get` executable in `${DMOD_TOOLS_BIN_DIR}` or system PATH
2. **Saves version requirements**: Creates a version requirements file (e.g., `<target_name>_version_requirements.txt`) containing all module version specifications
3. **Downloads headers**: For each module, it executes `dmf-get headers <module_spec> -o <output_dir>`
4. **Passes configuration**: Automatically passes `-t ${DMOD_TOOLS_NAME}` if defined and `-m ${DMOD_DMM_URL}` if defined
5. **Adds include directories**: The downloaded headers (located at `${DMOD_DMF_DIR}/<module_name>/inc`) are automatically added to the target's include directories with the specified visibility
6. **Integration with todmd**: When the module is built, the version requirements file is automatically passed to `todmd` when generating the `.dmd` dependencies file. This ensures that version constraints specified in `dmod_link_modules` are included in the generated `.dmd` file.

**Output Directory:**

Headers are downloaded to: `${DMOD_DMF_DIR}/inc/<module_name>/include/`

For example, for the `dmini` module, headers are located at: `${DMOD_DMF_DIR}/inc/dmini/include/dmini.h`

The include directory added to the target is: `${DMOD_DMF_DIR}/inc/<module_name>/include`, allowing you to use `#include <module_header.h>` directly.

By default, `DMOD_DMF_DIR` is set to `${CMAKE_BINARY_DIR}/dmf`.

**Requirements:**

- The `dmf-get` tool must be built and available (build with `-DDMOD_BUILD_TOOLS=ON`)
- A valid manifest file must be accessible (either specified via environment variables or using the default registry)
- The modules must exist in the configured registry

**Environment Variables (for dmf-get):**

The `dmf-get` tool (used internally) respects the following environment variables:
- `DMOD_MANIFEST` - Path or URL to the manifest file
- `DMOD_TOOLS_NAME` - Tools name for platform-specific modules
- `DMOD_INC_DIR` - Default include directory (overridden by the `-o` flag used by this function)

**CMake Variables:**

- `DMOD_DMF_DIR` - CMake variable controlling the base directory for downloaded headers (default: `${CMAKE_BINARY_DIR}/dmf`)
- `DMOD_TOOLS_BIN_DIR` - CMake variable specifying where to find the dmf-get tool
- `DMOD_TOOLS_NAME` - CMake variable for tools configuration name (e.g., "arch/x86_64"). If defined, passed to dmf-get via `-t` flag
- `DMOD_DMM_URL` - CMake variable for manifest URL. If defined, passed to dmf-get via `-m` flag

**Error Handling:**

- If `dmf-get` is not found, a warning is issued and the function returns without error
- If a module download fails, a warning is issued and the function continues with remaining modules
- The build continues even if some headers cannot be downloaded

**Notes:**

- This function should be called **after** `dmod_add_executable` or `dmod_add_library`
- Headers are downloaded during CMake configuration, not during build
- If you need to update headers, re-run CMake configuration
- The function only downloads headers (not the full module files)
- Visibility scopes work the same way as `target_include_directories`

### `dmod_link_builtin(targetName [PRIVATE|PUBLIC|INTERFACE] modules...)`

Downloads headers **and** a precompiled static library for "system modules" and statically links them into your target. Uses the `dmf-get` tool's `headers` and `lib` commands.

A "system module" (e.g. `dmlist`, `dmosi-posix`) is a DMOD module that is never turned into a loadable `.dmf` file - it is distributed as a static library (the `lib` resource in its `.dmr`, see [dmf-get Tool](dmf-get-tool.md) and [DMR File Format](dmr-file-format.md)) meant to be built directly into a system-side binary: the loader itself, `dmod-boot`, or host tools/tests.

**Parameters:**
- `targetName` - Name of the target to link into (must already exist)
- `PRIVATE|PUBLIC|INTERFACE` - Optional visibility scope (default: PRIVATE if not specified), same meaning as for `dmod_link_modules`
- `modules...` - List of module specifications with optional versions (`module_name` or `module_name@version`)

**Example:**
```cmake
add_executable(dmod_loader main.c)
target_link_libraries(dmod_loader dmod pthread)

dmod_link_builtin(dmod_loader
    dmlist
    dmosi-posix
)
```

**How It Works:**

1. Finds `dmf-get`, same as `dmod_link_modules`
2. For each module, runs `dmf-get headers <module_spec> -o ${DMOD_DMF_DIR}/inc` and adds the resulting `${DMOD_DMF_DIR}/inc/<module_name>/include` directory to the target's include directories
3. For each module, runs `dmf-get lib <module_spec> -o ${DMOD_DMF_DIR}/lib` to download the precompiled static library
4. Statically links `${DMOD_DMF_DIR}/lib/<module_name>/lib/lib<module_name_snake_case>.a` into `targetName` (hyphens in the module name are converted to underscores, e.g. `dmosi-posix` -> `libdmosi_posix.a`, matching the module's own `.dmr`/CMake target naming)

These libraries define their DMOD API functions in a way that always marks them as "used" to the compiler/linker, so simply linking the archive is enough to make their Built-in API available - no explicit symbol references are needed at the call site.

**Important:** do **not** use this for regular DMOD modules created with `dmod_add_executable`/`dmod_add_library` - those must always link other modules dynamically via `dmod_link_modules`, since DMOD modules are loaded/unloaded independently at runtime and statically linking a dependency into one defeats that. `dmod_link_builtin` is only for system-side binaries that are not themselves loaded as `.dmf` files.

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
- `DMOD_DMR_PATH` - Path to the `.dmr` resource file. When set, a release package is created in `build/packages/<moduleName>/` and zipped to `build/packages/<moduleName>.zip` after the build. Requires the `mkdmrpkg` tool (build with `-DDMOD_BUILD_TOOLS=ON`).

## See Also

- [DMF-GET Tool Documentation](dmf-get-tool.md) - Package manager for DMOD modules
- [Module Debugging](module-debugging.md) - Debugging DMOD modules
- [Memory Analysis](memory-analysis.md) - Analyzing module memory usage
