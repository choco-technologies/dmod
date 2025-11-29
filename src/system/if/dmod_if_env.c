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
#   define _POSIX_C_SOURCE=200112L
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

/**
 * @brief Push a new environment context onto the stack
 * 
 * Creates a new environment context and pushes it as the current context.
 * The previous context is preserved and can be restored with Dmod_EnvCtx_Pop.
 * 
 * @note This function has no POSIX equivalent. The default implementation is empty.
 * 
 * @return 0 on success, -1 on error
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _EnvCtx_Push, ( void ))
{
    /* No POSIX equivalent - default empty implementation */
    return 0;
}

/**
 * @brief Pop the current environment context from the stack
 * 
 * Restores the previous environment context that was saved with Dmod_EnvCtx_Push.
 * 
 * @note This function has no POSIX equivalent. The default implementation is empty.
 * 
 * @return 0 on success, -1 on error
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _EnvCtx_Pop, ( void ))
{
    /* No POSIX equivalent - default empty implementation */
    return 0;
}

/**
 * @brief Unset an environment variable
 * 
 * Removes the specified environment variable from the environment.
 * 
 * @param Name Name of the environment variable to unset
 * 
 * @return 0 on success, -1 on error
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _Unsetenv, ( const char* Name ))
{
#if DMOD_USE_STDLIB && DMOD_USE_GETENV
    return unsetenv(Name);
#else
    (void)Name;
    return -1;
#endif
}

/**
 * @brief Get the next environment variable name
 * 
 * Iterates over environment variable names. Call with Last=NULL to get the first
 * environment variable name. Call with the previously returned name to get the next one.
 * Returns NULL when there are no more environment variables.
 * 
 * @note This function uses thread-local storage for the returned name buffer.
 * @note The search is O(n) per call because environment variables may change between calls.
 * 
 * @param Last The last returned environment variable name, or NULL to start iteration
 * 
 * @return The next environment variable name, or NULL if there are no more
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, const char*, _GetNextEnvName, ( const char* Last ))
{
#if DMOD_USE_STDLIB && DMOD_USE_GETENV
    extern char **environ;
    
    /* Maximum length for environment variable names */
    #define DMOD_ENV_NAME_MAX_LEN 256
    
    /* Thread-local storage for the name buffer to ensure thread safety */
    static __thread char nameBuf[DMOD_ENV_NAME_MAX_LEN];
    
    if (environ == NULL)
    {
        return NULL;
    }
    
    if (Last == NULL)
    {
        /* Start from the beginning */
        if (environ[0] == NULL)
        {
            return NULL;
        }
        /* Return the name part (before '=') */
        const char* eq = strchr(environ[0], '=');
        if (eq != NULL)
        {
            size_t nameLen = (size_t)(eq - environ[0]);
            if (nameLen >= DMOD_ENV_NAME_MAX_LEN)
            {
                nameLen = DMOD_ENV_NAME_MAX_LEN - 1;
            }
            memcpy(nameBuf, environ[0], nameLen);
            nameBuf[nameLen] = '\0';
            return nameBuf;
        }
        return environ[0];
    }
    else
    {
        /* Find the current entry and return the next one.
         * Linear search is required because the environment may change between calls. */
        size_t lastLen = strlen(Last);
        for (int i = 0; environ[i] != NULL; i++)
        {
            /* Check if this entry matches Last */
            if (strncmp(environ[i], Last, lastLen) == 0 && environ[i][lastLen] == '=')
            {
                /* Found current, return next */
                if (environ[i + 1] == NULL)
                {
                    return NULL;
                }
                const char* eq = strchr(environ[i + 1], '=');
                if (eq != NULL)
                {
                    size_t nameLen = (size_t)(eq - environ[i + 1]);
                    if (nameLen >= DMOD_ENV_NAME_MAX_LEN)
                    {
                        nameLen = DMOD_ENV_NAME_MAX_LEN - 1;
                    }
                    memcpy(nameBuf, environ[i + 1], nameLen);
                    nameBuf[nameLen] = '\0';
                    return nameBuf;
                }
                return environ[i + 1];
            }
        }
        /* Not found, return NULL */
        return NULL;
    }
    
    #undef DMOD_ENV_NAME_MAX_LEN
#else
    (void)Last;
    return NULL;
#endif
}
