# External Module Example

This directory contains an example showing how to build a DMOD module outside the dmod repository tree.

## Overview

Modules can be built outside the dmod directory by including `paths.cmake` and using the same approach as in the internal examples. The CMakeLists.txt structure is very similar to the examples in `examples/module/`.

## Building an External Module

To build a module outside the dmod tree:

1. Set `DMOD_DIR` to point to the dmod repository
2. Set `DMOD_MODE` to "DMOD_MODULE"
3. Set `DMOD_BUILD_DIR` to your project's build directory
4. Include `${DMOD_DIR}/paths.cmake`
5. Define your project with `project()`
6. Call `dmod_setup_external_module()` to setup dmod libraries
7. Configure your module (set DMOD_MODULE_NAME, DMOD_MODULE_VERSION, etc.)
8. Use `dmod_add_library()` or `dmod_add_executable()` to create your module

## Example

See `CMakeLists.txt.example` and `external_example.c.example` for a complete working example.

## Building

```bash
cd your_module_directory
mkdir build && cd build
cmake ..
make
```

The generated `.dmf` file will be in `build/dmf/`.

## Important Notes

- `DMOD_BUILD_DIR` should be set to `${CMAKE_CURRENT_BINARY_DIR}` before including paths.cmake
- The approach is the same as used in `examples/module/` - just set some variables and call the dmod functions
- All dmod functions (dmod_add_library, dmod_add_executable) work the same as in internal modules
