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
 * @brief Compresion interface for DMOD SAL
 * @date 24 02 2025 22:36:00
 * 
 * The compression interface is used to compress and decompress data in the system.
 *
 * @file dmod_if_env.c
 * @version 0.1
 */
#include <string.h>
#include "dmod.h"
#if DMOD_USE_STDLIB
#   include <stdlib.h>
#endif

//==============================================================================
//                              FUNCTIONS DECLARATIONS
//==============================================================================

/**
 * @brief Get environment variable
 * 
 * @param Name Name of the environment variable
 * 
 * @return Value of the environment variable
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, const char*, _GetEnv, ( const char* Name ))
{
    const char* value = NULL;
#if DMOD_USE_STDLIB && DMOD_USE_GETENV
    value = getenv(Name);
#endif

    if(value == NULL)
    {
        if( strcmp(Name, "DMOD_REPO_DIR") == 0 )
        {
            value = DMOD_REPO_DIR;
        }
        else if( strcmp(Name, "DMOD_REPO_PATHS") == 0 )
        {
            value = DMOD_REPO_PATHS;
        }
    }

    return value;
}

/**
 * @brief Set environment variable
 * 
 * @param Name Name of the environment variable
 * @param Value Value to set for the environment variable
 * @param Overwrite If non-zero, overwrite existing variable, otherwise do not change it
 * 
 * @return 0 on success, -1 on error
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _SetEnv, ( const char* Name, const char* Value, int Overwrite ))
{
#if DMOD_USE_STDLIB && DMOD_USE_GETENV
    return setenv(Name, Value, Overwrite);
#else
    (void)Name;
    (void)Value;
    (void)Overwrite;
    return -1;
#endif
}

