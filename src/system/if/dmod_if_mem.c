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

#include <string.h>
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
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void*, _Malloc, ( size_t Size ))
{
    return Dmod_MallocEx(Size, NULL);
}

/**
 * @brief Allocate memory
 * 
 * @param Size Size of memory to allocate
 * @param ModuleName Name of the module requesting memory
 * 
 * @return Pointer to allocated memory
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void*, _MallocEx, ( size_t Size, const char* ModuleName ))
{
    DMOD_LOG_VERBOSE("Allocating %zu bytes for module: %s\n", Size, ModuleName ? ModuleName : "NULL");
#if DMOD_USE_ALIGNED_MALLOC_MOCK
    return Dmod_AlignedMalloc(Size, sizeof(void*));
#elif DMOD_USE_STDLIB
    return malloc(Size);
    #else
    DMOD_LOG_ERROR("Dmod_Malloc interface not implemented");
    return NULL;
#endif
}

/**
 * @brief Reallocate memory
 * 
 * @param Ptr Pointer to memory to reallocate
 * @param Size Size of memory to allocate
 * 
 * @return Pointer to reallocated memory
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void*, _Realloc, ( void* Ptr, size_t Size ))
{
    return Dmod_ReallocEx(Ptr, Size, NULL);
}

/**
 * @brief Reallocate memory
 * 
 * @param Ptr Pointer to memory to reallocate
 * @param Size Size of memory to allocate
 * @param ModuleName Name of the module requesting memory
 * 
 * @return Pointer to reallocated memory
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void*, _ReallocEx, ( void* Ptr, size_t Size, const char* ModuleName ))
{
#if DMOD_USE_STDLIB && DMOD_USE_REALLOC
    return realloc(Ptr, Size);
#else
    void* newPtr = Dmod_Malloc(Size);
    if (newPtr != NULL) 
    {
        memcpy(newPtr, Ptr, Size);
        Dmod_Free(Ptr);
    }
    return newPtr;
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
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void*, _AlignedMalloc, ( size_t Size, size_t Alignment ))
{
    return Dmod_AlignedMallocEx(Size, Alignment, NULL);
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
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void*, _AlignedMallocEx, ( size_t Size, size_t Alignment, const char* ModuleName ))
{
    void* mem = NULL;
    size_t pagesize = Alignment;
    (void)pagesize;
#if DMOD_USE_MMAN
    pagesize = sysconf(_SC_PAGESIZE);
#endif

#if DMOD_USE_ALIGNED_ALLOC && DMOD_USE_STDLIB
    mem = aligned_alloc(pagesize, Size);
#elif DMOD_USE_ALIGNED_MALLOC_MOCK && DMOD_USE_STDLIB
#if !DMOD_USE_STDLIB
#   error DMOD_USE_ALIGNED_MALLOC_MOCK cannot be used without DMOD_USE_STDLIB
    void* original = NULL;
#else 
    void* original = malloc(Size + Alignment - 1 + sizeof(void*));
#endif
    if (original == NULL) 
    {
        DMOD_LOG_ERROR("malloc has returned NULL\n");
        return NULL;
    }
    uintptr_t aligned = (uintptr_t)original + Alignment - 1 + sizeof(void*);
    aligned &= ~(Alignment - 1);
    ((void**)aligned)[-1] = original;
    mem = (void*)aligned;
#else 
    DMOD_LOG_ERROR("Dmod_AlignedMalloc interface not implemented");
    mem = NULL;
#endif 

#if DMOD_USE_MMAN
    if (mem != NULL) 
    {
        // Round up Size to the nearest page boundary
        size_t prot_size = ((Size + pagesize - 1) / pagesize) * pagesize;
        if (mprotect(mem, prot_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) 
        {
            DMOD_LOG_ERROR("Cannot set memory protection. Size: %zu, Pagesize: %zu\n", prot_size, pagesize);
            free(mem);
            return NULL;
        }
    }
#endif

    return mem;
}


/**
 * @brief Free memory
 * 
 * @param ptr Pointer to memory to free
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void, _Free, ( void* ptr ))
{
    Dmod_FreeEx(ptr, false);
}

/**
 * @brief Free memory
 * 
 * @param ptr Pointer to memory to free
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void, _FreeEx, ( void* ptr, const bool concatenate ))
{
#if DMOD_USE_ALIGNED_MALLOC_MOCK
    #if !DMOD_USE_STDLIB
    #   error DMOD_USE_ALIGNED_MALLOC_MOCK cannot be used without DMOD_USE_STDLIB
    #else 
    free(((void**)ptr)[-1]);
    #endif
#elif DMOD_USE_STDLIB
    free(ptr);
#else 
    DMOD_LOG_ERROR("Dmod_Free interface not implemented");
#endif
}

/**
 * @brief Free all memory allocated by a module
 * 
 * @param ModuleName Name of the module
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void, _FreeModule, ( const char* ModuleName ))
{
    (void)ModuleName;
    DMOD_LOG_WARN("Dmod_FreeModule interface not implemented");
}
