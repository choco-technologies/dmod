# Using dmod_link_modules - Example

This document provides a practical example of how to use the `dmod_link_modules` CMake function to link external module headers to your DMOD module.

## Overview

The `dmod_link_modules` function simplifies the process of using external DMOD modules in your project by automatically downloading their headers and adding them to your module's include path.

## Prerequisites

1. Build DMOD with tools enabled:
   ```bash
   cmake -DDMOD_MODE=DMOD_SYSTEM -DDMOD_BUILD_TOOLS=ON -B build -S .
   cmake --build build/
   ```

2. Ensure `dmf-get` tool is available in your PATH or `DMOD_TOOLS_BIN_DIR`

3. Configure your manifest file (or use the default registry)

## Basic Example

Let's say you're creating a module that needs to use functionality from two external modules:
- `dmffs` - A file system module
- `driver@1.0` - A specific version of a driver module

### CMakeLists.txt

```cmake
# Set module properties
set(DMOD_MODULE_NAME        my_application)
set(DMOD_MODULE_VERSION     "1.0")
set(DMOD_AUTHOR_NAME        "Your Name")
set(DMOD_STACK_SIZE         2048)
set(DMOD_PRIORITY           0)

# Create the module
dmod_add_executable(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION} 
    main.c
    app_logic.c
)

# Link external module headers - default PRIVATE visibility
dmod_link_modules(${DMOD_MODULE_NAME}
    dmffs           # Latest version
    driver@1.0      # Specific version
)

# Or specify visibility explicitly
dmod_link_modules(${DMOD_MODULE_NAME}
    PUBLIC
        base_types@1.0  # Public headers for interface
    PRIVATE
        internal_lib    # Private implementation headers
)
```

### main.c

```c
#include <dmod.h>
#include <dmffs.h>      // Now available thanks to dmod_link_modules
#include <driver.h>     // Now available thanks to dmod_link_modules

int main(int argc, char** argv)
{
    // Initialize DMOD
    Dmod_Init();
    
    // You can now use functions from the linked modules
    // (The actual modules will be loaded at runtime by DMOD)
    dmffs_init();
    driver_setup();
    
    // Your application logic here
    Dmod_Printf("Application running\n");
    
    return 0;
}
```

## What Happens Behind the Scenes

When you run CMake configuration:

1. **CMake finds dmf-get**: The function locates the `dmf-get` tool
   ```
   -- Using dmf-get: /path/to/build/bin/tools/dmf-get
   ```

2. **Downloads headers**: For each module, `dmf-get headers` is executed
   ```
   -- Downloading headers for module: dmffs -> /path/to/build/dmf/dmffs/inc
   -- Downloading headers for module: driver -> /path/to/build/dmf/driver/inc
   ```

3. **Adds include directories**: The header paths are added to your target
   ```
   -- Added include directory for dmffs (PRIVATE): /path/to/build/dmf/dmffs/inc
   -- Added include directory for driver (PRIVATE): /path/to/build/dmf/driver/inc
   ```

4. **Headers are available**: You can now `#include` files from the downloaded headers in your source code

## Visibility Scopes

The function supports three visibility scopes, similar to `target_include_directories`:

### PRIVATE (Default)
Headers are only available to the target itself:

```cmake
dmod_link_modules(${DMOD_MODULE_NAME}
    dmffs@1.0       # Implicitly PRIVATE
    driver
)
```

### PUBLIC
Headers are available to both the target and any targets that link to it:

```cmake
dmod_link_modules(${DMOD_MODULE_NAME}
    PUBLIC
        base_types@1.0      # Headers exposed to dependents
        common_utils
)
```

This is useful when your module's public API requires types or functions from the linked module.

### INTERFACE
Headers are only available to targets that link to this target (not the target itself):

```cmake
dmod_link_modules(${DMOD_MODULE_NAME}
    INTERFACE
        header_only_lib     # Only for consumers of this module
)
```

### Mixed Visibility
You can specify different visibility for different modules:

```cmake
dmod_link_modules(${DMOD_MODULE_NAME}
    PUBLIC
        api_types@2.0       # Exposed through your API
    PRIVATE
        internal_impl       # Implementation detail
        crypto_lib@1.5
)
```

## Advanced Example with Version Control

```cmake
# Create the module
dmod_add_executable(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION} 
    main.c
)

# Link multiple modules with different versioning strategies
dmod_link_modules(${DMOD_MODULE_NAME}
    dmffs               # Latest version - always get newest features
    core_lib@2.0        # Pinned major version - get 2.x updates but not 3.x
    stable_driver@1.2.3 # Exact version - never change
    beta_feature        # Latest version - for experimental features
)
```

## Environment Variables

You can control the behavior using environment variables:

```bash
# Set custom DMF directory
export DMOD_DMF_DIR=/path/to/my/modules

# Set custom manifest
export DMOD_MANIFEST=https://my-registry.com/manifest.dmm

# Build your module
cmake -DDMOD_MODE=DMOD_MODULE -B build -S .
cmake --build build/
```

## CMake Variables

You can also use CMake variables to configure the function's behavior:

### DMOD_TOOLS_NAME
Specifies the tools configuration name for platform-specific modules:

```bash
cmake -DDMOD_MODE=DMOD_MODULE \
      -DDMOD_TOOLS_NAME=arch/armv7/cortex-m7 \
      -B build -S .
```

This is automatically passed to `dmf-get` via the `-t` flag, allowing it to download platform-specific headers.

### DMOD_DMM_URL
Specifies a custom manifest URL:

```bash
cmake -DDMOD_MODE=DMOD_MODULE \
      -DDMOD_DMM_URL=https://my-company.com/dmod-manifest.dmm \
      -B build -S .
```

This is automatically passed to `dmf-get` via the `-m` flag, allowing you to use a custom module registry.

### Example with All Variables

```bash
cmake -DDMOD_MODE=DMOD_MODULE \
      -DDMOD_TOOLS_NAME=arch/armv7/cortex-m7 \
      -DDMOD_DMM_URL=https://registry.example.com/manifest.dmm \
      -DDMOD_DMF_DIR=/opt/dmod/modules \
      -B build -S .
```

## Using with Library Modules

The function works the same way with library modules:

```cmake
set(DMOD_MODULE_NAME        my_library)
set(DMOD_MODULE_VERSION     "1.0")
set(DMOD_AUTHOR_NAME        "Your Name")

# Create a library module
dmod_add_library(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION} 
    library.c
    utils.c
)

# Link external headers
dmod_link_modules(${DMOD_MODULE_NAME}
    base_types@1.0
    common_utils
)
```

## Troubleshooting

### dmf-get not found

**Error:**
```
CMake Warning: dmf-get tool not found. Skipping module headers download for my_application.
```

**Solution:**
Build DMOD with tools enabled:
```bash
cmake -DDMOD_MODE=DMOD_SYSTEM -DDMOD_BUILD_TOOLS=ON -B build -S .
cmake --build build/
```

### Module not found in registry

**Error:**
```
CMake Warning: Failed to download headers for mymodule: Error: Module not found: mymodule
```

**Solutions:**
1. Check if the module name is correct
2. Verify your manifest file contains the module
3. Check network connectivity if using a remote manifest
4. Use `dmf-get` directly to test: `dmf-get headers mymodule`

### Headers not updated

If you update a module version but CMake doesn't download new headers:

**Solution:**
Re-run CMake configuration:
```bash
rm -rf build/
cmake -DDMOD_MODE=DMOD_MODULE -B build -S .
```

Or manually clean the headers directory:
```bash
rm -rf build/dmf/module_name/
cmake -DDMOD_MODE=DMOD_MODULE -B build -S .
```

## Best Practices

1. **Pin versions for production**: Use specific versions (`module@1.0`) for stable builds
2. **Use latest for development**: Use version-less specs (`module`) during active development
3. **Document dependencies**: Comment why each module is needed
4. **Check headers exist**: Verify the downloaded headers have the functions you need
5. **Update regularly**: Periodically update to newer versions and test compatibility

## Complete Working Example

Here's a complete example project structure:

```
my_project/
├── CMakeLists.txt
├── main.c
└── README.md
```

**CMakeLists.txt:**
```cmake
cmake_minimum_required(VERSION 3.18)

# Include DMOD
set(DMOD_DIR "/path/to/dmod")
include(${DMOD_DIR}/paths.cmake)
dmod_setup_external_module()

# Configure module
set(DMOD_MODULE_NAME        my_app)
set(DMOD_MODULE_VERSION     "1.0")
set(DMOD_AUTHOR_NAME        "Developer")
set(DMOD_STACK_SIZE         2048)
set(DMOD_PRIORITY           0)

# Create module
dmod_add_executable(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION} 
    main.c
)

# Link external modules
dmod_link_modules(${DMOD_MODULE_NAME}
    dmffs@1.0       # File system module
    logger          # Logging utility (latest)
)
```

**main.c:**
```c
#include <dmod.h>
#include <dmffs.h>
#include <logger.h>

/**
 * @brief Module initialization function
 * 
 * This function is called when the module is loaded by the system.
 * 
 * @param Config Configuration parameters for the module
 * @return 0 on success, non-zero on error
 */
int dmod_init(const Dmod_Config_t *Config)
{
    // Module initialization code
    return 0;
}

/**
 * @brief Module deinitialization function
 * 
 * This function is called when the module is unloaded from the system.
 * 
 * @return 0 on success, non-zero on error
 */
int dmod_deinit(void)
{
    // Module cleanup code
    return 0;
}

int main(int argc, char** argv)
{
    logger_init();
    logger_log("Application starting");
    
    dmffs_init();
    logger_log("File system initialized");
    
    // Application logic here
    
    return 0;
}
```

Build and run:
```bash
# Build DMOD system first (with tools)
cd /path/to/dmod
cmake -DDMOD_MODE=DMOD_SYSTEM -DDMOD_BUILD_TOOLS=ON -B build -S .
cmake --build build/

# Build your module
cd /path/to/my_project
cmake -DDMOD_MODE=DMOD_MODULE -B build -S .
cmake --build build/

# The module file will be in build/dmf/my_app.dmf
```

## See Also

- [CMake Functions Reference](cmake-functions.md) - Complete reference for all DMOD CMake functions
- [DMF-GET Tool Documentation](dmf-get-tool.md) - Details about the dmf-get package manager
- [Module Debugging](module-debugging.md) - How to debug DMOD modules
