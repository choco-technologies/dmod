# External Module Example

This directory contains an example of how to build a DMOD module outside the dmod repository tree.

## Overview

Modules can be built outside the dmod directory tree by including the necessary dmod CMake files and subdirectories. This allows you to:
- Keep your module code separate from the dmod framework
- Build modules independently
- Maintain your own version control for modules

## Building the Module

To build this example module:

1. Navigate to the module directory
2. Create a build directory
3. Configure with CMake, specifying the path to DMOD_DIR
4. Build

```bash
cd /path/to/your/module
mkdir build && cd build
cmake .. -DDMOD_DIR=/path/to/dmod
make
```

## CMakeLists.txt Template

The key elements for building a module outside the dmod tree are:

1. **Set DMOD_DIR**: Point to the dmod repository location
2. **Set DMOD_MODE**: Set to "DMOD_MODULE" 
3. **Include paths.cmake**: Include the dmod paths configuration
4. **Add dmod subdirectories**: Include scripts, lib, inc, and src
5. **Create dmod interface library**: Recreate the dmod target
6. **Use dmod_add_library or dmod_add_executable**: Create your module

See the `CMakeLists.txt.template` file for a complete template.

## Example Module Structure

```
your_module/
├── CMakeLists.txt
├── your_module.c
└── your_module.h (optional)
```

## Important Notes

- The DMOD_DIR must point to a valid dmod repository
- You need to include all necessary dmod subdirectories (scripts, lib, inc, src)
- The dmod libraries will be built as part of your module build
- The generated .dmf file will be in your build/dmf directory
