/**
 * MIT License
 * 
 * Copyright (c) 2025 patryk.kubiak90@gmail.com
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
 * @brief String interface for DMOD SAL
 * @date 29 10 2025
 * 
 * The string interface is used to handle strings in the system.
 *
 * @file dmod_if_str.c
 * @version 0.1
 */
#include "dmod.h"


//==============================================================================
//                              FUNCTIONS DECLARATIONS
//==============================================================================

/**
 * @brief Duplicate a string, attributing the allocation to a named owner
 *
 * The owner has to be passed in rather than worked out here: this file is part
 * of the DMOD system, so the Dmod_Malloc() it sees is the system's own, and
 * every duplicate would otherwise be tagged to the system instead of to the
 * module that asked for it. Since allocation tracking bulk-frees a module's
 * memory on unload by exactly that tag, a mis-tagged string outlives - or dies
 * with - the wrong owner. See the comment on Dmod_StrDup in dmod_sal.h.
 *
 * @param Str.......... String to duplicate
 * @param ModuleName ... Allocation owner, as Dmod_MallocEx() understands it
 *
 * @return Pointer to the duplicated string, or NULL
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, char*, _StrDupEx, ( const char* Str, const char* ModuleName ))
{
    if (Str == NULL) 
    {
        return NULL;
    }

    size_t len = 0;
    while (Str[len] != '\0') 
    {
        len++;
    }

    char* copy = (char*)Dmod_MallocEx(len + 1, ModuleName);
    if (copy == NULL) 
    {
        return NULL;
    }

    for (size_t i = 0; i <= len; i++) 
    {
        copy[i] = Str[i];
    }

    return copy;
}

/**
 * @brief Duplicate a string, attributing the allocation the way this entry
 *        point always has - to the system
 *
 * Kept deliberately unchanged for the system's own use and for modules compiled
 * against a header that predates Dmod_StrDupEx(). Re-tagging these allocations
 * to the running context looks like the obvious fix and is not one: it
 * *shortens* their lifetime, because tracking bulk-frees a context's memory
 * when it unloads. Callers written against this entry point were built on the
 * old behaviour, where a duplicate outlived whichever process happened to make
 * it - measured on real hardware, re-tagging them turns strings those callers
 * still hold (a console's device path, for one) into freed memory the moment an
 * unrelated short-lived process exits.
 *
 * Correct attribution is therefore something a caller opts into by rebuilding
 * against the Dmod_StrDup macro in dmod_sal.h, which resolves to
 * Dmod_StrDupEx() with its own name - not something retrofitted underneath
 * callers that never asked for it.
 *
 * @param Str String to duplicate
 *
 * @return Pointer to the duplicated string, or NULL
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, char*, _StrDup, ( const char* Str ))
{
    if (Str == NULL) 
    {
        return NULL;
    }

    size_t len = 0;
    while (Str[len] != '\0') 
    {
        len++;
    }

    char* copy = (char*)Dmod_Malloc(len + 1);
    if (copy == NULL) 
    {
        return NULL;
    }

    for (size_t i = 0; i <= len; i++) 
    {
        copy[i] = Str[i];
    }

    return copy;
}