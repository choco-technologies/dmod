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
int DMOD_WEAK_SYMBOL Dmod_Printf( const char* Format, ... )
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
 * @brief Assert function
 * 
 * @param Condition Condition to assert
 * @param Message Message to print
 * @param File File name
 * @param Line Line number
 * @param Function Function name
 */
void  DMOD_WEAK_SYMBOL Dmod_Assert( int Condition, const char* Message, const char* File, int Line, const char* Function )
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
