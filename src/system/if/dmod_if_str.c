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
 * @brief Allocate memory
 * 
 * @param Size Size of memory to allocate
 * 
 * @return Pointer to allocated memory
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