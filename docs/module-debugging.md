# DMOD Module Debugging Guide

This guide explains how to debug dynamic modules (DMF files) loaded by the `dmod_loader` using GDB.

## Overview

When a module is loaded by `dmod_loader`, it is placed at a dynamically allocated memory address that **changes on every run**. To debug the module with GDB, you need to:

1. Start dmod_loader with `--debug` flag (pauses after loading)
2. Attach GDB to the running process
3. Load symbols at the **runtime text section address** shown
4. Set breakpoints and continue

**Important:** The module address changes every time the program runs due to memory allocation. You must use the address shown by `--debug` during that specific run.

DMOD provides tools to simplify this process:
- `--debug` flag in `dmod_loader` - pauses after load, shows the text section address
- `dmod-debug.sh` script - helper script for the debugging workflow

## Prerequisites

- GDB installed on your system
- Module built with debug symbols (`-g` flag)
- Access to both the DMF file and the original ELF file

## Method 1: Using the `--debug` Flag (Recommended)

### Step 1: Start dmod_loader with --debug

```bash
./dmod_loader ./module.dmf --debug
```

The program will load the module and pause, showing output like:

```
================================================================================
                         DMOD DEBUG MODE                                        
================================================================================

Module loaded successfully. Debug information:
  Module name:    example_app
  Base address:   0x443000
  Text section:   0x443140 (offset: 0x140)
  Module size:    1336 bytes

To debug this module with GDB:

  1. In another terminal, attach GDB to this process:
     gdb -p 12345

  2. In GDB, load symbols from the module's ELF file:
     add-symbol-file <path/to/module_elf> 0x443140
...
================================================================================
Press ENTER to continue execution...
================================================================================
```

### Step 2: Attach GDB (in another terminal)

```bash
gdb -p 12345  # Use the PID shown above
```

### Step 3: Load Symbols at the Text Section Address

In GDB, use the **text section address** shown (e.g., `0x443140`):

```bash
(gdb) add-symbol-file /path/to/example_app 0x443140
```

GDB will ask for confirmation - type `y`.

### Step 4: Set Breakpoints

```bash
(gdb) break main
(gdb) break dmod_init
```

### Step 5: Continue Execution in GDB

```bash
(gdb) c
```

### Step 6: Resume dmod_loader

Go back to the first terminal and press **ENTER** to continue execution.

GDB will stop at your breakpoints!

## Method 2: Using the `dmod-debug.sh` Script

The script automates launching dmod_loader with `--debug`:

```bash
./scripts/dmod-debug.sh ./dmod_loader ./module.dmf ./module_elf
```

This will:
1. Start dmod_loader with `--debug`
2. Display instructions for attaching GDB
3. Wait for you to debug

Then follow Steps 2-6 from Method 1 above.

## Method 3: Manual GDB Debugging

For advanced users who want to debug from the start:

### Step 1: Start GDB with dmod_loader

```bash
gdb ./dmod_loader
```

### Step 2: Set a Breakpoint on WaitForDebugger

```bash
(gdb) break WaitForDebugger
(gdb) run ./module.dmf --debug
```

### Step 3: When Breakpoint Hits, Get the Text Address

```bash
(gdb) print/x (uintptr_t)context->Data + context->Footer->Text.SectionStart
```

### Step 4: Load Symbols and Set Breakpoints

```bash
(gdb) add-symbol-file ./module_elf <address_from_above>
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

### Breakpoints don't work / code doesn't stop

**Most common cause:** Using wrong address. The address changes every run!

Always use the **text section address** shown by `--debug` during the current run. Do not reuse addresses from previous runs.

### "No debugging symbols found"

Make sure you're using the ELF file (not the DMF file) and that it was built with debug symbols (`-g` flag).

### "Cannot insert breakpoint"

The text section address might be incorrect. Verify you're using the address from the current `--debug` output.

### "Symbol not found"

The function might be optimized out or inlined. Try building with `-O0` to disable optimizations.

### GDB Cannot Attach

You might need to run GDB with elevated privileges or adjust ptrace settings:
```bash
sudo sysctl -w kernel.yama.ptrace_scope=0
```

## Quick Reference

```bash
# Terminal 1: Start dmod_loader
./dmod_loader ./module.dmf --debug
# Note the PID and Text section address

# Terminal 2: Attach and debug
gdb -p <PID>
(gdb) add-symbol-file ./module_elf <TEXT_SECTION_ADDRESS>
(gdb) break main
(gdb) c

# Terminal 1: Press ENTER to continue
```

## See Also

- [DMOD Loader Documentation](../examples/system/dmod_loader/)
- [Module Development Guide](../templates/module/README.md)
- [GDB Documentation](https://sourceware.org/gdb/documentation/)
