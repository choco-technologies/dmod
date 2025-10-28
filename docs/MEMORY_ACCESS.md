# Memory Access Functions

## Overview

DMOD provides two new functions for reading and writing memory at arbitrary addresses:
- `Dmod_ReadMemory` - Read data from a memory address into a buffer
- `Dmod_WriteMemory` - Write data from a buffer to a memory address

These functions are part of the DMOD SAL (System Abstraction Layer) and can be overridden by the system.

## API

### Dmod_ReadMemory

```c
size_t Dmod_ReadMemory(uintptr_t Address, void* Buffer, size_t Size);
```

**Parameters:**
- `Address` - Memory address to read from
- `Buffer` - Buffer to store the read data
- `Size` - Number of bytes to read

**Returns:** Number of bytes successfully read

### Dmod_WriteMemory

```c
size_t Dmod_WriteMemory(uintptr_t Address, const void* Buffer, size_t Size);
```

**Parameters:**
- `Address` - Memory address to write to
- `Buffer` - Buffer containing data to write
- `Size` - Number of bytes to write

**Returns:** Number of bytes successfully written

## Usage

### Basic Example

```c
#include "dmod_sal.h"

uint8_t data[16];
// Read 16 bytes from address 0x1000
size_t bytesRead = Dmod_ReadMemory(0x1000, data, sizeof(data));

uint8_t writeData[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11};
// Write 8 bytes to address 0x2000
size_t bytesWritten = Dmod_WriteMemory(0x2000, writeData, sizeof(writeData));
```

## Mock Memory

For testing and simulation purposes, DMOD supports mock memory. This allows you to map a file to a specific memory address range.

### Configuration

Enable mock memory by passing the `DMOD_MEMORY` parameter to CMake:

```bash
cmake .. -DDMOD_MODE=DMOD_SYSTEM -DDMOD_MEMORY=<address>:<filepath>
```

**Format:** `address:filepath`
- `address` - Hexadecimal address without '0x' prefix (e.g., `ffff0000` not `0xffff0000`)
- `filepath` - Path to the file to use as mock memory

### Example

```bash
# Map my-file.bin to address 0xffff0000
cmake .. -DDMOD_MODE=DMOD_SYSTEM -DDMOD_MEMORY=ffff0000:./my-file.bin
make
```

When mock memory is enabled:
- Reads from the configured address range will read from the file
- Writes to the configured address range will write to the in-memory copy
- Reads/writes outside the mock memory range use direct memory access

### Example Usage with Mock Memory

```c
#include "dmod_sal.h"

// If built with -DDMOD_MEMORY=ffff0000:./data.bin
// The file contents will be available at 0xffff0000

uint8_t buffer[32];
size_t bytesRead = Dmod_ReadMemory(0xffff0000, buffer, sizeof(buffer));
// Reads first 32 bytes from data.bin

uint8_t newData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
Dmod_WriteMemory(0xffff0000 + 100, newData, sizeof(newData));
// Writes to offset 100 in the mock memory (in RAM, not in file)
```

## Implementation Details

- Functions are defined as weak symbols, allowing system-level override
- Default implementation uses direct memory access via `memcpy`
- Mock memory is loaded into RAM on first access
- Mock memory requires `DMOD_USE_STDIO` to be enabled
- Functions return 0 on error (NULL buffer, zero size, etc.)

## Testing

Test cases are provided in:
- `tests/system/if/tests_dmod_rawmem.cpp` - Basic functionality tests
- `tests/system/if/tests_dmod_mock_memory.cpp` - Mock memory tests

Run tests:
```bash
cd build
./tests/system/if/tests_dmod_rawmem
./tests/system/if/tests_dmod_mock_memory
```
