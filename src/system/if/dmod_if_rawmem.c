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

#if DMOD_USE_STDIO
#   include <stdio.h>
#endif

//==============================================================================
//                              MOCK MEMORY CONFIGURATION
//==============================================================================

#ifdef DMOD_MEMORY_MOCK_ADDRESS
#   define DMOD_MEMORY_MOCK_ENABLED 1
static uint8_t* g_MockMemoryBuffer = NULL;
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

#if DMOD_USE_STDIO
    FILE* file = fopen(DMOD_MEMORY_MOCK_FILE, "rb");
    if (file == NULL) 
    {
        DMOD_LOG_ERROR("Failed to open mock memory file: %s\n", DMOD_MEMORY_MOCK_FILE);
        return;
    }

    fseek(file, 0, SEEK_END);
    g_MockMemorySize = ftell(file);
    fseek(file, 0, SEEK_SET);

    g_MockMemoryBuffer = (uint8_t*)Dmod_Malloc(g_MockMemorySize);
    if (g_MockMemoryBuffer == NULL) 
    {
        DMOD_LOG_ERROR("Failed to allocate mock memory buffer\n");
        fclose(file);
        return;
    }

    size_t read = fread(g_MockMemoryBuffer, 1, g_MockMemorySize, file);
    if (read != g_MockMemorySize) 
    {
        DMOD_LOG_WARN("Mock memory file size mismatch: expected %zu, got %zu\n", g_MockMemorySize, read);
    }

    fclose(file);
    DMOD_LOG_INFO("Mock memory initialized at address 0x%lx with size %zu from file %s\n", 
                  (unsigned long)g_MockMemoryAddress, g_MockMemorySize, DMOD_MEMORY_MOCK_FILE);
#else
    DMOD_LOG_ERROR("DMOD_MEMORY_MOCK requires DMOD_USE_STDIO to be enabled\n");
#endif
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
    
    if (g_MockMemoryBuffer != NULL && Address >= g_MockMemoryAddress && 
        Address < (g_MockMemoryAddress + g_MockMemorySize)) 
    {
        uintptr_t offset = Address - g_MockMemoryAddress;
        size_t available = g_MockMemorySize - offset;
        size_t toRead = (Size < available) ? Size : available;
        
        memcpy(Buffer, g_MockMemoryBuffer + offset, toRead);
        return toRead;
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
    
    if (g_MockMemoryBuffer != NULL && Address >= g_MockMemoryAddress && 
        Address < (g_MockMemoryAddress + g_MockMemorySize)) 
    {
        uintptr_t offset = Address - g_MockMemoryAddress;
        size_t available = g_MockMemorySize - offset;
        size_t toWrite = (Size < available) ? Size : available;
        
        memcpy(g_MockMemoryBuffer + offset, Buffer, toWrite);
        return toWrite;
    }
#endif

    // Direct memory access
    memcpy((void*)Address, Buffer, Size);
    return Size;
}
