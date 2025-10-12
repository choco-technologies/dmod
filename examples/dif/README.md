# DIF (Dmod Interface) System

## Overview

The DIF (Dmod Interface) system allows you to define interfaces that can be implemented by multiple modules simultaneously. Unlike MAL (Module Abstraction Layer), which supports only one implementation at a time, DIF enables multiple implementations to coexist and be dynamically discovered and used at runtime.

## Use Case

Imagine you have a file system interface (DIFS) that you want to implement in different ways:
- **FatFS** - for FAT32 file systems
- **FlashFS** - for flash-based file systems
- **Any other implementation** - you can add as many as needed

With DIF, all these implementations can be loaded simultaneously, and your application (e.g., VFS - Virtual File System) can discover and use them dynamically.

## Architecture

### 1. Interface Definition Module (DIFS)

First, create a module that defines the interface:

**difs.h:**
```c
#include "dmod.h"
#include "difs_defs.h"

// Define DIF signature strings as compile-time constants
#define dmod_difs_fopen_sig   DMOD_MAKE_DIF_SIGNATURE( difs, 1.0, _fopen )
#define dmod_difs_fclose_sig  DMOD_MAKE_DIF_SIGNATURE( difs, 1.0, _fclose )

// Define the DIF with function signatures and typedefs
dmod_difs_dif( 1.0, int, _fopen, (void** fp, const char* path, int mode, int attr) );
dmod_difs_dif( 1.0, int, _fclose, (void* fp) );
```

The `dmod_difs_dif` macro creates:
- A typedef for the function pointer type: `dmod_difs_fopen_t`
- Access to the signature string via the `_sig` macros

**difs.c:**
```c
#define DMOD_ENABLE_REGISTRATION    ON
#ifndef DMOD_difs
#   define DMOD_difs
#endif

#include "difs.h"

int dmod_init(const Dmod_Config_t *Config) {
    Dmod_Printf("DIFS interface module initialized\n");
    return 0;
}
```

### 2. Implementation Modules (FatFS, FlashFS)

Create modules that implement the interface:

**fatfs.c:**
```c
#define DMOD_ENABLE_REGISTRATION    ON
#ifndef DMOD_fatfs
#   define DMOD_fatfs
#endif

#include "dmod.h"
#include "difs.h"

// Implement _fopen for FatFS
dmod_difs_dif_api_declaration( 1.0, FatFS, int, _fopen, (void** fp, const char* path, int mode, int attr) )
{
    Dmod_Printf("FatFS: Opening file '%s'\n", path);
    *fp = Dmod_Malloc(32);  // Allocate file handle
    return 0; // Success
}

// Implement other DIFS functions...
dmod_difs_dif_api_declaration( 1.0, FatFS, int, _fclose, (void* fp) )
{
    Dmod_Printf("FatFS: Closing file\n");
    Dmod_Free(fp);
    return 0;
}
```

**flashfs.c** - Similar structure with FlashFS-specific implementation.

### 3. Usage Module (VFS)

Create a module that discovers and uses the implementations:

**vfs.c:**
```c
#include "dmod.h"
#include "difs.h"

int dmod_init(const Dmod_Config_t *Config)
{
    // Iterate through all modules implementing DIFS
    Dmod_Context_t* fs = Dmod_GetNextDifModule( dmod_difs_fopen_sig, NULL );
    
    while(fs != NULL)
    {
        // Get function pointers from this module
        dmod_difs_fopen_t fopen_func = (dmod_difs_fopen_t)Dmod_GetDifFunction( fs, dmod_difs_fopen_sig );
        dmod_difs_fclose_t fclose_func = (dmod_difs_fclose_t)Dmod_GetDifFunction( fs, dmod_difs_fclose_sig );
        
        // Use the functions
        void* file_handle = NULL;
        fopen_func( &file_handle, "test.txt", 1, 0 );
        fclose_func( file_handle );
        
        // Get next implementation
        fs = Dmod_GetNextDifModule( dmod_difs_fopen_sig, fs );
    }
    
    return 0;
}
```

## API Reference

### Defining Interfaces

```c
dmod_<module>_dif( VERSION, RET, NAME, PARAMS )
```

Creates a DIF interface definition. This macro:
- Defines a function pointer typedef: `dmod_<module><NAME>_t`
- Makes the signature string available via macros

### Implementing Interfaces

```c
dmod_<module>_dif_api_declaration( VERSION, IMPL_MODULE, RET, NAME, PARAMS )
```

Implements a DIF function in a module. This macro:
- Declares the function with the correct name
- Registers it as an INPUT API with the DIF signature

### Using Interfaces

```c
Dmod_Context_t* Dmod_GetNextDifModule( const char* DifSignature, Dmod_Context_t* Previous )
```

Iterates through all loaded modules that implement a specific DIF function.
- **DifSignature**: The DIF signature string (e.g., `dmod_difs_fopen_sig`)
- **Previous**: Previous module context (NULL to start from the beginning)
- **Returns**: Next module implementing the DIF, or NULL if no more modules

```c
void* Dmod_GetDifFunction( Dmod_Context_t* Context, const char* DifSignature )
```

Gets the function pointer for a DIF implementation from a specific module.
- **Context**: Module context
- **DifSignature**: The DIF signature string
- **Returns**: Function pointer, or NULL if not found

## Building

### CMake

For the interface module:
```cmake
dmod_add_library(difs 1.0 difs.c)
target_include_directories(difs_if INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
```

For implementation modules:
```cmake
dmod_add_library(fatfs 1.0 fatfs.c)
target_link_libraries(fatfs difs_if)
```

### Testing

Run the DIF test application:
```bash
./build/examples/system/dif_test/dif_test
```

This will load DIFS, FatFS, FlashFS, and VFS modules, demonstrating dynamic discovery and usage of multiple interface implementations.

## Example Output

```
=== VFS Demo - Using DIF Interfaces ===

Found file system #1
FatFS: Opening file 'test.txt' with mode 1, attr 0
FatFS: Writing 16 bytes
  Wrote 16 bytes
FatFS: Reading 16 bytes
  Read 16 bytes
FatFS: Closing file

Found file system #2
FlashFS: Opening file 'test.txt' with mode 1, attr 0
FlashFS: Writing 16 bytes
  Wrote 16 bytes
FlashFS: Reading 16 bytes
  Read 16 bytes
FlashFS: Closing file

Total file systems found: 2

=== VFS Demo Complete ===
```

## Comparison with MAL

| Feature | MAL | DIF |
|---------|-----|-----|
| Multiple implementations | No (1:1) | Yes (1:N) |
| Dynamic discovery | No | Yes |
| Use case | Swappable implementations | Plugin-like architecture |
| Runtime overhead | Lower | Slightly higher (discovery) |

## Notes

- DIF signatures must match exactly between interface definition and implementations
- Use the `_sig` macros (e.g., `dmod_difs_fopen_sig`) as compile-time constants for signatures
- All DIF implementations must be loaded and enabled before they can be discovered
- The order of discovered modules is determined by the system's module loading order

