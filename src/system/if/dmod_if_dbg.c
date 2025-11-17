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

#include "dmod_sal.h"
#if DMOD_USE_STDIO
#   include <stdarg.h>
#   include <stdio.h>
#endif
#if DMOD_USE_ASSERT
#   include <assert.h>
#endif
#if DMOD_IMPLEMENT_PRINTF
#   define DMOD_PRIVATE
#   include "private/dmod_prf.h"
#   include <stdarg.h>
#endif

//==============================================================================
//                              FUNCTIONS DECLARATIONS
//==============================================================================

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
