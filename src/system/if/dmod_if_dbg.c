/**
 * MIT License
 * 
 * Copyright (c) 2024 patryk.kubiak90@gmail.com
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
 * @brief Debug interface for DMOD SAL
 * @date 20 12 2024 10:57:00
 * 
 * The debug interface is used to print and assert messages in the system.
 *
 * @file dmod_if_dbg.c
 * @version 0.1
 */

#define DMOD_PRIVATE
#include "dmod_sal.h"
#include "private/dmod_vars.h"

#if DMOD_USE_STDIO
#   include <stdarg.h>
#   include <stdio.h>
#endif
#if DMOD_USE_ASSERT
#   include <assert.h>
#endif
#if DMOD_IMPLEMENT_PRINTF
#   include "private/dmod_prf.h"
#   include <stdarg.h>
#endif

#if DMOD_USE_STDLIB
#   include <string.h>
#   include <ctype.h>
#endif

//==============================================================================
//                              FUNCTIONS DECLARATIONS
//==============================================================================

/* Forward declaration for caching env-var-based module log levels */
extern void Dmod_SetModuleLogLevel( const char* ModuleName, Dmod_LogLevel_t Level );

/**
 * @brief Check if the log level is enabled
 * 
 * @param LogLevel Log level to check
 * 
 * @return True if the log level is enabled, false otherwise
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, bool, _CheckLogLevel, ( Dmod_LogLevel_t LogLevel ))
{
    return Dmod_LogLevel >= LogLevel;
}

/**
 * @brief Parse a log level string into a Dmod_LogLevel_t value
 *
 * Recognised strings (case-insensitive): verbose, info, warning, error, none.
 *
 * @param Value  Null-terminated string to parse (may be NULL)
 *
 * @return Parsed log level, or Dmod_LogLevel_Count if the string is unknown / NULL
 */
static Dmod_LogLevel_t ParseLogLevelString( const char* Value )
{
#if DMOD_USE_STDLIB
    if( Value == NULL )
    {
        return Dmod_LogLevel_Count;
    }
    /* Build a lowercase copy of Value for comparison (up to 8 chars) */
    char lower[8];
    size_t i;
    for( i = 0; i < sizeof(lower) - 1 && Value[i] != '\0'; i++ )
    {
        lower[i] = (char)tolower((unsigned char)Value[i]);
    }
    lower[i] = '\0';

    if( strcmp(lower, "verbose") == 0 ) return Dmod_LogLevel_Verbose;
    if( strcmp(lower, "info")    == 0 ) return Dmod_LogLevel_Info;
    if( strcmp(lower, "warning") == 0 ) return Dmod_LogLevel_Warn;
    if( strcmp(lower, "error")   == 0 ) return Dmod_LogLevel_Error;
    if( strcmp(lower, "none")    == 0 ) return Dmod_LogLevel_None;
#else
    (void)Value;
#endif
    return Dmod_LogLevel_Count; /* unknown */
}

/**
 * @brief Check if the given log level is enabled for the specified module
 *
 * The function resolves the effective log level in the following order:
 *  1. Per-module level set via Dmod_SetModuleLogLevel()
 *  2. Environment variable <UPPERCASE_MODULE_NAME>_LOG_LEVEL (cached on first use)
 *  3. Global Dmod_LogLevel (fallback)
 *
 * @param ModuleName  Name of the calling module (may be NULL)
 * @param LogLevel    Log level to check
 *
 * @return True if the log level is enabled, false otherwise
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, bool, _CheckModuleLogLevel, ( const char* ModuleName, Dmod_LogLevel_t LogLevel ))
{
    if( ModuleName == NULL )
    {
        return Dmod_CheckLogLevel(LogLevel);
    }

#if DMOD_USE_STDLIB
    /* Search the per-module list first */
    Dmod_ModuleLogLevel_t* node = Dmod_ModuleLogLevels;
    while( node != NULL )
    {
        if( strncmp(node->Name, ModuleName, DMOD_MAX_MODULE_NAME_LENGTH - 1) == 0 )
        {
            return node->Level >= LogLevel;
        }
        node = node->Next;
    }

    /* Not found – check env var: <UPPERCASE_MODULE_NAME>_LOG_LEVEL */
    {
        char envName[DMOD_MAX_MODULE_NAME_LENGTH + 11]; /* "_LOG_LEVEL\0" = 11 chars */
        size_t i = 0;
        while( ModuleName[i] != '\0' && i < (DMOD_MAX_MODULE_NAME_LENGTH - 1) )
        {
            envName[i] = (char)toupper((unsigned char)ModuleName[i]);
            i++;
        }
        if( i + 11 <= sizeof(envName) )
        {
            memcpy(envName + i, "_LOG_LEVEL", 11);  /* includes null terminator */

            const char* envValue = Dmod_GetEnv(envName);
            if( envValue != NULL )
            {
                Dmod_LogLevel_t level = ParseLogLevelString(envValue);
                if( level < Dmod_LogLevel_Count )
                {
                    /* Cache the result so future calls skip the env lookup */
                    Dmod_SetModuleLogLevel(ModuleName, level);
                    return level >= LogLevel;
                }
            }
        }
    }
#endif /* DMOD_USE_STDLIB */

    /* Fall back to the global log level */
    return Dmod_CheckLogLevel(LogLevel);
}

/**
 * @brief Printf function
 * 
 * @param Format Format string
 * 
 * @return Number of characters printed
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _Printf, ( const char* Format, ... ))
{
    #if DMOD_USE_STDIO
    int Ret = 0;
    va_list Args;
    va_start( Args, Format );
    Ret = vprintf( Format, Args );
    va_end( Args );
    return Ret;
    #else
    return 0;
    #endif
}

/**
 * @brief FPrintf function - prints to a file
 * 
 * @param File File handle
 * @param Format Format string
 * 
 * @return Number of characters printed
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _FPrintf, ( void* File, const char* Format, ... ))
{
    #if DMOD_USE_STDIO
    int Ret = 0;
    va_list Args;
    va_start( Args, Format );
    Ret = vfprintf( (FILE*)File, Format, Args );
    va_end( Args );
    return Ret;
    #else
    return 0;
    #endif
}

/**
 * @brief VSnPrintf function
 * 
 * @param Buffer Output buffer (can be NULL to calculate required size)
 * @param Size Size of the buffer
 * @param Format Format string
 * @param Args Variable argument list
 * 
 * @return Number of characters that would have been written (excluding null terminator)
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _VSnPrintf, ( char* Buffer, size_t Size, const char* Format, va_list Args ))
{
    #if DMOD_USE_STDIO
    if( Buffer == NULL )
    {
        // Calculate required buffer size without writing
        va_list ArgsCopy;
        va_copy( ArgsCopy, Args );
        int Ret = vsnprintf( NULL, 0, Format, ArgsCopy );
        va_end( ArgsCopy );
        return Ret;
    }
    else
    {
        return vsnprintf( Buffer, Size, Format, Args );
    }
    #elif DMOD_IMPLEMENT_PRINTF
    return Dmod_VSnPrintf_Impl( Buffer, Size, Format, Args );
    #else
    return 0;
    #endif
}

/**
 * @brief SnPrintf function
 * 
 * @param Buffer Output buffer (can be NULL to calculate required size)
 * @param Size Size of the buffer
 * @param Format Format string
 * 
 * @return Number of characters that would have been written (excluding null terminator)
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _SnPrintf, ( char* Buffer, size_t Size, const char* Format, ... ))
{
    #if DMOD_USE_STDIO || DMOD_IMPLEMENT_PRINTF
    int Ret = 0;
    va_list Args;
    va_start( Args, Format );
    Ret = Dmod_VSnPrintf( Buffer, Size, Format, Args );
    va_end( Args );
    return Ret;
    #else
    return 0;
    #endif
}

/**
 * @brief Assert function
 * 
 * @param Condition Condition to assert
 * @param Message Message to print
 * @param File File name
 * @param Line Line number
 * @param Function Function name
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void, _Assert, ( int Condition, const char* Message, const char* File, int Line, const char* Function ))
{
    #if DMOD_USE_ASSERT
    if( !Condition )
    {
        __assert_fail( Message, File, Line, Function );
    }
    #else
    if( !Condition )
    {
        DMOD_LOG_ERROR( "Assertion failed: %s\n", Message );
        DMOD_LOG_ERROR( "File: %s\n", File );
        DMOD_LOG_ERROR( "Line: %d\n", Line );
        DMOD_LOG_ERROR( "Function: %s\n", Function );

        while(1);
    }
    #endif
}

/**
 * @brief Get progress bar string for a given percentage
 * 
 * Returns one of seven UTF-8 block bar strings corresponding to 0–100%.
 * Each of the 6 segments represents ~17% (Percent * 6 / 100, clamped to 0–6).
 * 
 * @param Percent  Completion percentage (0–100; values outside this range are clamped)
 * 
 * @return Pointer to a statically-allocated bar string, e.g. "[██░░░░]"
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, const char*, _GetStepBar, ( int Percent ))
{
    static const char* const bars[7] = {
        "[\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91]",
        "[\xe2\x96\x88\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91]",
        "[\xe2\x96\x88\xe2\x96\x88\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91]",
        "[\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91]",
        "[\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x91\xe2\x96\x91]",
        "[\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x91]",
        "[\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88]"
    };
    int idx = Percent * 6 / 100;
    if( idx < 0 ) idx = 0;
    if( idx > 6 ) idx = 6;
    return bars[idx];
}
