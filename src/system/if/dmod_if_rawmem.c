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

#ifdef DMOD_MEMORY_MOCK_ADDRESS
#   define DMOD_MEMORY_MOCK_ENABLED 1
static void* g_MockMemoryFile = NULL;
static size_t g_MockMemorySize = 0;
static uintptr_t g_MockMemoryAddress = DMOD_MEMORY_MOCK_ADDRESS;

static void Dmod_InitMockMemory(void)
{
    static bool initialized = false;
    if (initialized) 
    {
        return;
    }
    initialized = true;

    g_MockMemoryFile = Dmod_FileOpen(DMOD_MEMORY_MOCK_FILE, "r+b");
    if (g_MockMemoryFile == NULL) 
    {
        DMOD_LOG_ERROR("Failed to open mock memory file: %s\n", DMOD_MEMORY_MOCK_FILE);
        return;
    }

    g_MockMemorySize = Dmod_FileSize(g_MockMemoryFile);
    DMOD_LOG_INFO("Mock memory initialized at address 0x%lx with size %zu from file %s\n", 
                  (unsigned long)g_MockMemoryAddress, g_MockMemorySize, DMOD_MEMORY_MOCK_FILE);
}
#else
#   define DMOD_MEMORY_MOCK_ENABLED 0
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

#if DMOD_MEMORY_MOCK_ENABLED
    Dmod_InitMockMemory();
    
    if (g_MockMemoryFile != NULL && Address >= g_MockMemoryAddress && 
        Address < (g_MockMemoryAddress + g_MockMemorySize)) 
    {
        uintptr_t offset = Address - g_MockMemoryAddress;
        size_t available = g_MockMemorySize - offset;
        size_t toRead = (Size < available) ? Size : available;
        
        // Seek to the offset in the file
        if (Dmod_FileSeek(g_MockMemoryFile, offset, DMOD_SEEK_SET) != 0)
        {
            DMOD_LOG_ERROR("Failed to seek in mock memory file\n");
            return 0;
        }
        
        // Read from file
        size_t bytesRead = Dmod_FileRead(Buffer, 1, toRead, g_MockMemoryFile);
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

#if DMOD_MEMORY_MOCK_ENABLED
    Dmod_InitMockMemory();
    
    if (g_MockMemoryFile != NULL && Address >= g_MockMemoryAddress && 
        Address < (g_MockMemoryAddress + g_MockMemorySize)) 
    {
        uintptr_t offset = Address - g_MockMemoryAddress;
        size_t available = g_MockMemorySize - offset;
        size_t toWrite = (Size < available) ? Size : available;
        
        // Seek to the offset in the file
        if (Dmod_FileSeek(g_MockMemoryFile, offset, DMOD_SEEK_SET) != 0)
        {
            DMOD_LOG_ERROR("Failed to seek in mock memory file\n");
            return 0;
        }
        
        // Write to file
        size_t bytesWritten = Dmod_FileWrite(Buffer, 1, toWrite, g_MockMemoryFile);
        return bytesWritten;
    }
#endif

    // Direct memory access
    memcpy((void*)Address, Buffer, Size);
    return Size;
}
