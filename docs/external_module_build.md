# Building DMF Modules Outside the DMOD Repository Tree

## Issue Summary

This document describes the fix for the issue where DMF (Dynamic Module File) modules could not be built when located outside the dmod directory tree.

## Problem

When attempting to build a module outside the dmod repository using CMake, the build would fail because `DMOD_SCRIPTS_DIR` was being incorrectly set to the external module's directory instead of the dmod scripts directory.

### Root Cause

In `/scripts/CMakeLists.txt`, line 1 was unconditionally setting:
```cmake
set(DMOD_SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "Path to the scripts directory")
```

When this file was included from an external module's CMakeLists.txt, `CMAKE_CURRENT_SOURCE_DIR` would point to the external module's directory, not the dmod scripts directory. This caused subsequent `configure_file()` calls to fail because they couldn't find required template files like `api.h.in`.

## Solution

The fix was to make `DMOD_SCRIPTS_DIR` conditional:

```cmake
if(NOT DEFINED DMOD_SCRIPTS_DIR)
    set(DMOD_SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "Path to the scripts directory")
endif()
```

This way, when `paths.cmake` sets `DMOD_SCRIPTS_DIR` to `${DMOD_DIR}/scripts` before including `scripts/CMakeLists.txt`, the value is preserved.

## Usage

To build a module outside the dmod tree:

1. Create your module directory structure:
   ```
   your_module/
   ├── CMakeLists.txt
   └── your_module.c
   ```

2. Use the template provided in `examples/external_module/CMakeLists.txt.template`

3. Build your module:
   ```bash
   cd your_module
   mkdir build && cd build
   cmake .. -DDMOD_DIR=/path/to/dmod
   make
   ```

4. The generated `.dmf` file will be in `your_module/build/dmf/`

## Template Files

The following template files are provided in `examples/external_module/`:
- `CMakeLists.txt.template` - Complete CMake configuration template
- `external_example.c.template` - Example module source code
- `README.md` - Detailed documentation

## Testing

The fix has been tested with:
1. Internal module builds (examples/module/) - ✓ Working
2. External module builds from /tmp - ✓ Working  
3. Template-based external module - ✓ Working
4. DMOD_MODE=DMOD_MODULE - ✓ Working

## Impact

This change is backward compatible and does not affect:
- Existing module builds within the dmod tree
- DMOD_SYSTEM mode builds
- Makefile-based builds
