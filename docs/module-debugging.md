# DMOD Module Debugging Guide

This guide explains how to debug dynamic modules (DMF files) loaded by the `dmod_loader` using GDB.

## Overview

When a module is loaded by `dmod_loader`, it is placed at a dynamically allocated memory address. To debug the module with GDB, you need to:

1. Know the base address where the module is loaded
2. Load the module's debug symbols (from the ELF file) at the correct offset
3. Attach GDB to the running process

DMOD provides tools to simplify this process:
- `--debug` flag in `dmod_loader` - prints the base address and waits for debugger
- `dmod-debug.sh` script - automates the GDB setup

## Prerequisites

- GDB installed on your system
- Module built with debug symbols (`-g` flag)
- Access to both the DMF file and the original ELF file

## Method 1: Using the `--debug` Flag (Recommended)

The easiest way to debug a module is using the `--debug` flag:

```bash
./dmod_loader ./module.dmf --debug
```

This will:
1. Load the module
2. Print debug information including:
   - Module name
   - Base address
   - Module size
3. Wait for you to press ENTER before continuing

### Example Output

```
================================================================================
                         DMOD DEBUG MODE                                        
================================================================================

Module loaded successfully. Debug information:
  Module name:    example_app
  Base address:   0x55555576a2a0
  Module size:    1336 bytes

To debug this module with GDB:

  1. In another terminal, attach GDB to this process:
     gdb -p 12345

  2. In GDB, load symbols from the module's ELF file:
     add-symbol-file <path/to/module_elf> 0x55555576a2a0

  3. Set breakpoints and continue:
     break main
     continue

================================================================================
Press ENTER to continue execution...
================================================================================
```

### Attaching GDB

1. Open a new terminal
2. Attach GDB to the dmod_loader process:
   ```bash
   gdb -p <PID>
   ```
3. Load symbols from the module's ELF file:
   ```bash
   (gdb) add-symbol-file /path/to/build/examples/module/application/example_app 0x55555576a2a0
   ```
4. Set breakpoints:
   ```bash
   (gdb) break main
   (gdb) break dmod_init
   ```
5. Continue execution:
   ```bash
   (gdb) continue
   ```
6. Go back to the first terminal and press ENTER to continue

## Method 2: Using the `dmod-debug.sh` Script

The `dmod-debug.sh` script automates the GDB setup process.

### Basic Usage

```bash
./scripts/dmod-debug.sh <dmod_loader> <module.dmf> <module_elf> <base_address>
```

### Parameters

| Parameter | Description |
|-----------|-------------|
| `dmod_loader` | Path to the dmod_loader executable |
| `module.dmf` | Path to the DMF module file to load |
| `module_elf` | Path to the ELF file with debug symbols |
| `base_address` | Memory address where the module is loaded (hex format) |

### Options

| Option | Description |
|--------|-------------|
| `-g, --gdbserver` | Use gdbserver instead of direct gdb attachment |
| `-p, --port PORT` | Port for gdbserver (default: 1234) |
| `-v, --verbose` | Enable verbose output |
| `-h, --help` | Show help message |

### Examples

```bash
# Direct GDB debugging
./scripts/dmod-debug.sh ./build/dmod_loader ./build/dmf/example_app.dmf \
    ./build/examples/module/application/example_app 0x55555576a2a0

# Using gdbserver on port 2345
./scripts/dmod-debug.sh -g -p 2345 ./build/dmod_loader ./build/dmf/example_app.dmf \
    ./build/examples/module/application/example_app 0x55555576a2a0
```

## Method 3: Manual GDB Debugging

If you prefer to set up GDB manually:

### Step 1: Find the Base Address

Run dmod_loader with the `--debug` flag to get the base address:

```bash
./dmod_loader ./module.dmf --debug
```

Note the "Base address" value from the output.

### Step 2: Start GDB

```bash
gdb ./dmod_loader
```

### Step 3: Set Up Breakpoints and Run

In GDB:
```bash
(gdb) break Dmod_Run
(gdb) run ./module.dmf
```

### Step 4: Load Module Symbols

When the breakpoint is hit:
```bash
(gdb) add-symbol-file /path/to/module_elf <base_address>
(gdb) break main
(gdb) continue
```

## Building Modules with Debug Symbols

To debug a module, you need to build it with debug symbols. 

### CMake

```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DDMOD_MODE=DMOD_MODULE -B build -S .
cmake --build build
```

### Make

```bash
CFLAGS="-g -O0" make
```

## Finding the ELF File

When you build a module, two files are generated:
- `module_name.dmf` - The dynamic module file (in `build/dmf/`)
- `module_name` - The ELF executable (in `build/examples/module/.../`)

The ELF file contains the debug symbols. Its location depends on where the module source is located.

For example:
- DMF file: `build/dmf/example_app.dmf`
- ELF file: `build/examples/module/application/example_app`

## Troubleshooting

### "No debugging symbols found"

Make sure you're using the ELF file (not the DMF file) and that it was built with debug symbols (`-g` flag).

### "Cannot insert breakpoint"

The base address might be incorrect. Use the `--debug` flag to get the correct address.

### "Symbol not found"

The function might be optimized out or inlined. Try building with `-O0` to disable optimizations.

### GDB Cannot Attach

You might need to run GDB with elevated privileges or adjust ptrace settings:
```bash
sudo sysctl -w kernel.yama.ptrace_scope=0
```

## See Also

- [DMOD Loader Documentation](../examples/system/dmod_loader/)
- [Module Development Guide](../templates/module/README.md)
- [GDB Documentation](https://sourceware.org/gdb/documentation/)
