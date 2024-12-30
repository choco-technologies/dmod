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
 * @brief Memory interface for DMOD SAL
 * @date 20 12 2024 10:35:47
 * This file contains the system abstraction layer (SAL) for DMOD
 * 
 * @file dmod_if_mem.c
 * 
 */

#include "dmod_sal.h"
#if DMOD_USE_STDLIB
#   include <stdlib.h>
#   include <unistd.h>
#endif
#if DMOD_USE_MMAN
#   include <sys/mman.h>
#endif

//==============================================================================
//                              FUNCTIONS DECLARATIONS
//==============================================================================

/**
 * @brief Allocate memory
 * 
 * @param Size Size of memory to allocate
 * 
 * @return Pointer to allocated memory
 */
void* DMOD_WEAK_SYMBOL Dmod_Malloc(size_t Size)        
{
    #if DMOD_USE_STDLIB
    return malloc(Size);
    #else
    DMOD_LOG_ERROR("Dmod_Malloc interface not implemented");
    return NULL;
    #endif
}

/**
 * @brief Allocate aligned memory
 * 
 * @param Size Size of memory to allocate
 * @param Alignment Alignment of memory
 * 
 * @return Pointer to allocated memory
 * 
 * @note Optional - set to NULL if not supported
 */
void* DMOD_WEAK_SYMBOL Dmod_AlignedMalloc(size_t Size, size_t Alignment)
{
    #if DMOD_USE_STDLIB
    size_t pagesize = Alignment;
    #if DMOD_USE_MMAN
    pagesize = sysconf(_SC_PAGESIZE);
    #endif
    void* mem = aligned_alloc(pagesize, Size);
    #if DMOD_USE_MMAN
    if (mprotect(mem, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) 
    {
        DMOD_LOG_ERROR("Cannot set memory protection. Pagesize: %d\n", pagesize);
        free(mem);
        return NULL;
    }
    #endif
    return mem;
    #else
    DMOD_LOG_ERROR("Dmod_AlignedMalloc interface not implemented");
    return NULL;
    #endif
}

/**
 * @brief Free memory
 * 
 * @param ptr Pointer to memory to free
 */
void DMOD_WEAK_SYMBOL Dmod_Free(void *ptr)
{
    #if DMOD_USE_STDLIB
    free(ptr);
    #else 
    DMOD_LOG_ERROR("Dmod_Free interface not implemented");
    #endif
}
