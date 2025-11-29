# DMOD Module Debugging Guide

This guide explains how to debug dynamic modules (DMF files) loaded by the `dmod_loader` using GDB.

## ⚠️ Important: ptrace Permission

On most Linux systems, GDB cannot attach to another process due to security restrictions. **Before debugging**, you must either:

1. **Run GDB as root:**
   ```bash
   sudo gdb -p <PID>
   ```

2. **Or disable ptrace protection (temporarily):**
   ```bash
   echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope
   ```

If you see this error, use one of the above solutions:
```
Could not attach to process. [...] ptrace: Operation not permitted.
```

## Quick Start (Easiest Method)

The simplest way to debug a module:

```bash
# Start dmod_loader with --debug and path to ELF file
./dmod_loader ./module.dmf --debug ./module

# This generates two scripts in the current directory:
#   dmod_gdb_script.gdb  - GDB commands with correct addresses
#   dmod_debug.sh        - Bash script to run GDB with all parameters

# In another terminal, simply run:
./dmod_debug.sh

# Press ENTER in the dmod_loader terminal to continue
```

## Overview

When a module is loaded by `dmod_loader`, it is placed at a dynamically allocated memory address that **changes on every run**. To debug the module with GDB, you need to:

1. Start dmod_loader with `--debug` flag (pauses after loading)
2. Attach GDB to the running process (use `sudo gdb` if needed)
3. Load symbols at the **runtime text section address** shown
4. Set breakpoints and continue

**Important:** The module address changes every time the program runs due to memory allocation. You must use the address shown by `--debug` during that specific run.

DMOD provides tools to simplify this process:
- `--debug [elf_path]` flag in `dmod_loader` - pauses after load, optionally generates debug scripts
- Auto-generated `dmod_debug.sh` - one-command GDB attachment with correct addresses

## Prerequisites

- GDB installed on your system
- Module built with debug symbols (`-g` flag)
- Access to both the DMF file and the original ELF file
- **Root access or ptrace permissions** (see above)

## Method 1: Auto-Generated Scripts (Recommended)

### Step 1: Start dmod_loader with --debug and ELF path

```bash
./dmod_loader ./module.dmf --debug ./module_elf
```

This will:
1. Load the module and pause
2. Generate `dmod_gdb_script.gdb` with correct addresses
3. Generate `dmod_debug.sh` with pre-filled PID and script path

### Step 2: Run the generated script (in another terminal)

```bash
./dmod_debug.sh
```

### Step 3: Set breakpoints in GDB

```bash
(gdb) break main
(gdb) c
```

### Step 4: Resume dmod_loader

Press **ENTER** in the dmod_loader terminal to continue execution.

## Method 2: Manual Debugging

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
     sudo gdb -p 12345

  2. In GDB, load symbols from the module's ELF file:
     add-symbol-file <path/to/module_elf> 0x443140
...
================================================================================
Press ENTER to continue execution...
================================================================================
```

### Step 2: Attach GDB (in another terminal)

```bash
sudo gdb -p 12345  # Use the PID shown above
```

**Note:** You need `sudo` or ptrace permissions. See the ptrace section at the top of this document.

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
