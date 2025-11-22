/**
 * MIT License
 * 
 * Copyright (c) 2023 [Your Name or Your Organization]
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * @brief Raw memory interface for DMOD SAL
 * @date 2024-12-20
 * This file contains the system abstraction layer (SAL) for DMOD
 * 
 * @file dmod_if_rawmem.c
 * 
 */

#include <string.h>
#include "dmod_sal.h"

//==============================================================================
//                              MOCK MEMORY CONFIGURATION
//==============================================================================

#ifdef DMOD_MEMORY_MOCK_ENABLED
#ifndef DMOD_MEMORY_MOCK_COUNT
#   define DMOD_MEMORY_MOCK_COUNT 1
#endif

#ifndef DMOD_MAX_MOCK_MEMORY_REGIONS
#   define DMOD_MAX_MOCK_MEMORY_REGIONS 5
#endif

typedef struct {
    void* file;
    size_t size;
    uintptr_t address;
    bool initialized;
} Dmod_MockMemoryRegion_t;

static Dmod_MockMemoryRegion_t g_MockMemoryRegions[DMOD_MAX_MOCK_MEMORY_REGIONS] = {0};
static bool g_MockMemoryInitialized = false;

static void Dmod_InitMockMemory(void)
{
    if (g_MockMemoryInitialized) 
    {
        return;
    }
    g_MockMemoryInitialized = true;

#define INIT_MOCK_REGION(index, addr_macro, file_macro) \
    do { \
        if (index < DMOD_MEMORY_MOCK_COUNT) { \
            g_MockMemoryRegions[index].address = addr_macro; \
            g_MockMemoryRegions[index].file = Dmod_FileOpen(file_macro, "r+b"); \
            if (g_MockMemoryRegions[index].file != NULL) { \
                g_MockMemoryRegions[index].size = Dmod_FileSize(g_MockMemoryRegions[index].file); \
                g_MockMemoryRegions[index].initialized = true; \
                DMOD_LOG_INFO("Mock memory region %d: address=0x%lx, size=%zu, file=%s\n", \
                    index, (unsigned long)g_MockMemoryRegions[index].address, \
                    g_MockMemoryRegions[index].size, file_macro); \
            } else { \
                DMOD_LOG_ERROR("Failed to open mock memory file: %s\n", file_macro); \
            } \
        } \
    } while(0)

#ifdef DMOD_MEMORY_MOCK_ADDRESS_0
    INIT_MOCK_REGION(0, DMOD_MEMORY_MOCK_ADDRESS_0, DMOD_MEMORY_MOCK_FILE_0);
#endif
#ifdef DMOD_MEMORY_MOCK_ADDRESS_1
    INIT_MOCK_REGION(1, DMOD_MEMORY_MOCK_ADDRESS_1, DMOD_MEMORY_MOCK_FILE_1);
#endif
#ifdef DMOD_MEMORY_MOCK_ADDRESS_2
    INIT_MOCK_REGION(2, DMOD_MEMORY_MOCK_ADDRESS_2, DMOD_MEMORY_MOCK_FILE_2);
#endif
#ifdef DMOD_MEMORY_MOCK_ADDRESS_3
    INIT_MOCK_REGION(3, DMOD_MEMORY_MOCK_ADDRESS_3, DMOD_MEMORY_MOCK_FILE_3);
#endif
#ifdef DMOD_MEMORY_MOCK_ADDRESS_4
    INIT_MOCK_REGION(4, DMOD_MEMORY_MOCK_ADDRESS_4, DMOD_MEMORY_MOCK_FILE_4);
#endif

#undef INIT_MOCK_REGION
}

static Dmod_MockMemoryRegion_t* Dmod_FindMockMemoryRegion(uintptr_t Address)
{
    for (int i = 0; i < DMOD_MEMORY_MOCK_COUNT && i < DMOD_MAX_MOCK_MEMORY_REGIONS; i++)
    {
        if (g_MockMemoryRegions[i].initialized && 
            g_MockMemoryRegions[i].file != NULL &&
            Address >= g_MockMemoryRegions[i].address && 
            Address < (g_MockMemoryRegions[i].address + g_MockMemoryRegions[i].size))
        {
            return &g_MockMemoryRegions[i];
        }
    }
    return NULL;
}

#endif

//==============================================================================
//                              FUNCTIONS DECLARATIONS
//==============================================================================

/**
 * @brief Read memory from a specified address
 * 
 * @param Address Memory address to read from
 * @param Buffer Buffer to store the read data
 * @param Size Number of bytes to read
 * 
 * @return Number of bytes successfully read
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, size_t, _ReadMemory, ( uintptr_t Address, void* Buffer, size_t Size ))
{
    if (Buffer == NULL || Size == 0) 
    {
        return 0;
    }

#ifdef DMOD_MEMORY_MOCK_ENABLED
    Dmod_InitMockMemory();
    
    Dmod_MockMemoryRegion_t* region = Dmod_FindMockMemoryRegion(Address);
    if (region != NULL)
    {
        uintptr_t offset = Address - region->address;
        size_t available = region->size - offset;
        size_t toRead = (Size < available) ? Size : available;
        
        // Seek to the offset in the file
        if (Dmod_FileSeek(region->file, offset, DMOD_SEEK_SET) != 0)
        {
            DMOD_LOG_ERROR("Failed to seek in mock memory file\n");
            return 0;
        }
        
        // Read from file
        size_t bytesRead = Dmod_FileRead(Buffer, 1, toRead, region->file);
        return bytesRead;
    }
#endif

    // Direct memory access
    memcpy(Buffer, (const void*)Address, Size);
    return Size;
}

/**
 * @brief Write memory to a specified address
 * 
 * @param Address Memory address to write to
 * @param Buffer Buffer containing data to write
 * @param Size Number of bytes to write
 * 
 * @return Number of bytes successfully written
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, size_t, _WriteMemory, ( uintptr_t Address, const void* Buffer, size_t Size ))
{
    if (Buffer == NULL || Size == 0) 
    {
        return 0;
    }

#ifdef DMOD_MEMORY_MOCK_ENABLED
    Dmod_InitMockMemory();
    
    Dmod_MockMemoryRegion_t* region = Dmod_FindMockMemoryRegion(Address);
    if (region != NULL)
    {
        uintptr_t offset = Address - region->address;
        size_t available = region->size - offset;
        size_t toWrite = (Size < available) ? Size : available;
        
        // Seek to the offset in the file
        if (Dmod_FileSeek(region->file, offset, DMOD_SEEK_SET) != 0)
        {
            DMOD_LOG_ERROR("Failed to seek in mock memory file\n");
            return 0;
        }
        
        // Write to file
        size_t bytesWritten = Dmod_FileWrite(Buffer, 1, toWrite, region->file);
        return bytesWritten;
    }
#endif

    // Direct memory access
    memcpy((void*)Address, Buffer, Size);
    return Size;
}

/**
 * @brief Check if address is in RAM
 * 
 * @param Address Address to check
 * 
 * @return true if address is in RAM, false otherwise
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, bool, _IsRam, ( const void* Address ))
{
#if defined(__ARM_ARCH)
    uintptr_t addr = (uintptr_t)Address;
    // Common ARM architecture RAM regions (device-specific values should be configured)
    #if defined(DMOD_RAM_START) && defined(DMOD_RAM_END)
        return (addr >= DMOD_RAM_START && addr < DMOD_RAM_END);
    #else
        // Default: assume standard SRAM regions for common ARM Cortex-M
        // This is a fallback and should be overridden with platform-specific values
        return (addr >= 0x20000000 && addr < 0x30000000) || // SRAM region
               (addr >= 0x10000000 && addr < 0x20000000);    // Code SRAM region
    #endif
#else
    // For non-embedded systems (PC/Linux), we cannot reliably determine RAM regions
    // Return false to indicate this functionality is not available
    (void)Address;
    return false;
#endif
}

/**
 * @brief Check if address is in ROM
 * 
 * @param Address Address to check
 * 
 * @return true if address is in ROM, false otherwise
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, bool, _IsRom, ( const void* Address ))
{
#if defined(__ARM_ARCH)
    uintptr_t addr = (uintptr_t)Address;
    // Common ARM architecture ROM/Flash regions
    #if defined(DMOD_ROM_START) && defined(DMOD_ROM_END)
        return (addr >= DMOD_ROM_START && addr < DMOD_ROM_END);
    #else
        // Default: assume standard Flash regions for common ARM Cortex-M
        return (addr >= 0x00000000 && addr < 0x10000000);    // Flash region (0x00000000-0x0FFFFFFF)
    #endif
#else
    // For non-embedded systems, check if address is in code/text segment
    // This is a weak heuristic and may not work in all cases
    (void)Address;
    return false;
#endif
}

/**
 * @brief Check if address is in DMA region
 * 
 * @param Address Address to check
 * 
 * @return true if address is in DMA region, false otherwise
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, bool, _IsDma, ( const void* Address ))
{
#if defined(__ARM_ARCH)
    uintptr_t addr = (uintptr_t)Address;
    // DMA accessible regions are device-specific
    #if defined(DMOD_DMA_START) && defined(DMOD_DMA_END)
        return (addr >= DMOD_DMA_START && addr < DMOD_DMA_END);
    #else
        // Default: assume DMA can access SRAM and peripheral regions
        // This is device-specific and should be configured per platform
        return (addr >= 0x20000000 && addr < 0x30000000) || // SRAM
               (addr >= 0x40000000 && addr < 0x60000000);    // Peripherals
    #endif
#else
    (void)Address;
    return false;
#endif
}

/**
 * @brief Check if address is in External memory
 * 
 * @param Address Address to check
 * 
 * @return true if address is in External memory, false otherwise
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, bool, _IsExt, ( const void* Address ))
{
#if defined(__ARM_ARCH)
    uintptr_t addr = (uintptr_t)Address;
    // External memory regions are device-specific
    #if defined(DMOD_EXT_START) && defined(DMOD_EXT_END)
        return (addr >= DMOD_EXT_START && addr < DMOD_EXT_END);
    #else
        // Default: assume standard external memory regions for ARM Cortex-M
        return (addr >= 0x60000000 && addr < 0xA0000000) || // External RAM/Device
               (addr >= 0xC0000000 && addr < 0xE0000000);    // External Device
    #endif
#else
    (void)Address;
    return false;
#endif
}

/**
 * @brief Check if address is valid (in any known memory region)
 * 
 * @param Address Address to check
 * 
 * @return true if address is valid (RAM || ROM || DMA || EXT), false otherwise
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, bool, _IsAddressValid, ( const void* Address ))
{
    if (Address == NULL)
    {
        return false;
    }
    
    return Dmod_IsRam(Address) || 
           Dmod_IsRom(Address) || 
           Dmod_IsDma(Address) || 
           Dmod_IsExt(Address);
}
