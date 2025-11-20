# Memory Analysis for DMOD Modules

DMOD provides built-in memory analysis tools to help you understand the RAM and ROM usage of your modules during the build process.

## Overview

When building modules in MODULE mode, DMOD automatically analyzes the memory usage of each source file and displays a summary table at the end of the build. This helps you:

- Understand the memory footprint of your module
- Identify which source files consume the most memory
- Optimize your code for embedded systems with limited resources

## Automatic Memory Analysis

After building any module (library or application), you'll see a memory usage table automatically displayed:

```
================================================================================
  Memory Usage Analysis for: example_app
================================================================================

  File                           |        RAM |        ROM
  -------------------------------+------------+-----------
  main.c                         |        72B |       869B
  example.c                      |         8B |       107B
  example_app_header.c           |       248B |       280B
  -------------------------------+------------+-----------
  TOTAL                          |       328B |        1kB
================================================================================
```

### Understanding the Table

- **File**: The source file name (without path)
- **RAM**: Runtime memory usage (data + bss sections)
  - `data`: Initialized global/static variables
  - `bss`: Uninitialized global/static variables
- **ROM**: Read-only memory usage (text + data sections)
  - `text`: Program code
  - `data`: Initialized global/static variables (stored in ROM, copied to RAM at startup)

### Size Units

Memory sizes are automatically formatted for readability:
- **B**: Bytes (< 1 KB)
- **kB**: Kilobytes (< 1 MB)
- **MB**: Megabytes (≥ 1 MB)

## Detailed Symbol Analysis

For a more detailed breakdown of memory usage by individual symbols (functions, variables, etc.), use the dedicated memory analysis target:

```bash
cmake --build build/ --target <module_name>_memory_detailed
```

### Example Usage

For a module named `example_app`:

```bash
cmake --build build/ --target example_app_memory_detailed
```

This will display a detailed table showing every symbol in each source file:

```
================================================================================
  Detailed Symbol Analysis for: example_app
================================================================================

  File: main.c
  --------------------------------------------------------------------------------
  Symbol                         | Type |      Size
  -------------------------------+------+-----------
  Dmod_FreeEx                    | DATA |         8B
  Dmod_MallocEx                  | DATA |         8B
  Dmod_Printf                    | DATA |         8B
  _print                         | DATA |         8B
  dmodex_foo                     | DATA |         8B
  dmodex_bar_registration        | DATA |        16B
  dmodex_example_registration    | DATA |        16B
  _example                       | TEXT |        22B
  dmodex_bar                     | W    |        22B
  global_print                   | TEXT |        22B
  main                           | TEXT |       312B

  File: example.c
  --------------------------------------------------------------------------------
  Symbol                         | Type |      Size
  -------------------------------+------+-----------
  Dmod_Printf                    | DATA |         8B
  HelloWorld                     | TEXT |        19B
  ...
================================================================================
```

### Symbol Types

- **TEXT**: Code (functions)
- **DATA**: Initialized global/static data
- **BSS**: Uninitialized global/static data
- **RDAT**: Read-only data (constants)
- **W**: Weak symbols (can be overridden)

## Use Cases

### 1. Memory Optimization

Identify the largest functions and data structures in your module:

```bash
# Build your module
cmake --build build/ --target my_module

# Check the summary - identify files with high RAM/ROM usage
# ...

# Get detailed breakdown
cmake --build build/ --target my_module_memory_detailed

# Look for large functions or data structures that could be optimized
```

### 2. Comparing Changes

Before and after code changes, compare the memory usage:

```bash
# Before changes
cmake --build build/ --target my_module
# Note the TOTAL RAM and ROM values

# Make your changes...

# After changes
cmake --build build/ --target my_module
# Compare the new TOTAL values
```

### 3. Embedded System Planning

Use the ROM total to estimate flash memory requirements and the RAM total to estimate runtime memory requirements:

```bash
cmake --build build/ --target my_module
# Check the TOTAL row:
# - ROM total = Flash memory needed
# - RAM total = Runtime RAM needed (excluding stack)
```

## Technical Details

The memory analysis uses standard GNU binutils tools:

- **size**: Extracts section sizes from object files
- **nm**: Lists symbols with their sizes and types

The analysis script is located at `scripts/memory_analysis.cmake` and is automatically invoked during the build process for all modules created with `dmod_add_library()` or `dmod_add_executable()`.

## Limitations

1. **Object File Based**: Analysis is performed on individual object files before linking, so:
   - Linker optimizations (dead code elimination, etc.) are not reflected
   - The final binary size may be smaller than the sum shown
   
2. **Static Analysis Only**: The analysis shows compile-time memory usage:
   - Dynamic allocations (malloc/free) are not included
   - Stack usage is not included (only global/static data)

3. **Requires GNU Binutils**: The analysis requires the `size` and `nm` commands from GNU binutils, which are typically available on Linux systems.

## Troubleshooting

### No Analysis Output

If you don't see the memory analysis table after building:

1. Ensure you're building in MODULE mode:
   ```bash
   cmake -DDMOD_MODE=DMOD_MODULE -B build -S .
   ```

2. Check that GNU binutils is installed:
   ```bash
   size --version
   nm --version
   ```

### Incorrect Memory Values

If the memory values seem incorrect:

1. Ensure object files are being generated (check `build/<path>/CMakeFiles/<module>.dir/`)
2. Try rebuilding from scratch:
   ```bash
   rm -rf build
   cmake -DDMOD_MODE=DMOD_MODULE -B build -S .
   cmake --build build/
   ```

## See Also

- [Module Development](../README.md#module-development)
- [Building](../README.md#building)
- [Memory Access Documentation](MEMORY_ACCESS.md)
