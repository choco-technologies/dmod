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
 * @brief Minimal printf implementation for systems without stdio
 * @date 28 10 2025
 * 
 * This file provides a minimal printf/snprintf/vsnprintf implementation
 * that doesn't depend on stdio. It can be enabled with DMOD_IMPLEMENT_PRINTF.
 *
 * @file dmod_prf.c
 * @version 0.1
 */

#define DMOD_PRIVATE
#include "dmod_prf.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

//==============================================================================
//                              HELPER FUNCTIONS
//==============================================================================

static int Dmod_StrLen( const char* Str )
{
    int Len = 0;
    if( Str == NULL ) return 0;
    while( Str[Len] ) Len++;
    return Len;
}

static void Dmod_Print_Char( char** Buffer, size_t* Pos, size_t Size, char Ch, int* Count )
{
    (*Count)++;
    if( Buffer && *Buffer && *Pos < Size - 1 )
    {
        (*Buffer)[*Pos] = Ch;
        (*Pos)++;
    }
}

static void Dmod_Print_String( char** Buffer, size_t* Pos, size_t Size, const char* Str, int* Count )
{
    if( Str == NULL ) Str = "(null)";
    while( *Str )
    {
        Dmod_Print_Char( Buffer, Pos, Size, *Str++, Count );
    }
}

static void Dmod_Print_Int( char** Buffer, size_t* Pos, size_t Size, int32_t Value, int* Count )
{
    char Temp[12]; // Enough for -2147483648
    int i = 0;
    bool IsNegative = false;
    uint32_t UValue;
    
    if( Value < 0 )
    {
        IsNegative = true;
        // Handle INT32_MIN specially to avoid overflow
        UValue = (Value == INT32_MIN) ? ((uint32_t)INT32_MAX + 1) : (uint32_t)(-Value);
    }
    else
    {
        UValue = (uint32_t)Value;
    }
    
    // Convert to string (reversed)
    do
    {
        Temp[i++] = '0' + (UValue % 10);
        UValue /= 10;
    } while( UValue > 0 );
    
    // Add sign
    if( IsNegative )
    {
        Dmod_Print_Char( Buffer, Pos, Size, '-', Count );
    }
    
    // Print in correct order
    while( i > 0 )
    {
        Dmod_Print_Char( Buffer, Pos, Size, Temp[--i], Count );
    }
}

static void Dmod_Print_UInt( char** Buffer, size_t* Pos, size_t Size, uint32_t Value, int* Count )
{
    char Temp[11]; // Enough for 4294967295
    int i = 0;
    
    // Convert to string (reversed)
    do
    {
        Temp[i++] = '0' + (Value % 10);
        Value /= 10;
    } while( Value > 0 );
    
    // Print in correct order
    while( i > 0 )
    {
        Dmod_Print_Char( Buffer, Pos, Size, Temp[--i], Count );
    }
}

static void Dmod_Print_Hex( char** Buffer, size_t* Pos, size_t Size, uint32_t Value, bool Uppercase, int* Count )
{
    char Temp[9]; // Enough for 8 hex digits
    int i = 0;
    const char* HexDigits = Uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    
    // Convert to hex string (reversed)
    do
    {
        Temp[i++] = HexDigits[Value & 0xF];
        Value >>= 4;
    } while( Value > 0 );
    
    // Print in correct order
    while( i > 0 )
    {
        Dmod_Print_Char( Buffer, Pos, Size, Temp[--i], Count );
    }
}

static void Dmod_Print_Pointer( char** Buffer, size_t* Pos, size_t Size, void* Ptr, int* Count )
{
    uintptr_t Value = (uintptr_t)Ptr;
    char Temp[17]; // Enough for 16 hex digits
    int i = 0;
    const char* HexDigits = "0123456789abcdef";
    
    // Print "0x" prefix
    Dmod_Print_Char( Buffer, Pos, Size, '0', Count );
    Dmod_Print_Char( Buffer, Pos, Size, 'x', Count );
    
    // Convert to hex string (reversed)
    do
    {
        Temp[i++] = HexDigits[Value & 0xF];
        Value >>= 4;
    } while( Value > 0 );
    
    // Print in correct order
    while( i > 0 )
    {
        Dmod_Print_Char( Buffer, Pos, Size, Temp[--i], Count );
    }
}

//==============================================================================
//                              PUBLIC FUNCTIONS
//==============================================================================

int Dmod_VSnPrintf_Impl( char* Buffer, size_t Size, const char* Format, va_list Args )
{
    size_t Pos = 0;
    int Count = 0;
    char** BufPtr = Buffer ? &Buffer : NULL;
    
    if( Format == NULL ) return 0;
    if( Size == 0 ) BufPtr = NULL;
    
    while( *Format )
    {
        if( *Format == '%' )
        {
            Format++;
            
            // Handle format specifiers
            switch( *Format )
            {
                case '%':
                    Dmod_Print_Char( BufPtr, &Pos, Size, '%', &Count );
                    break;
                    
                case 'c':
                    Dmod_Print_Char( BufPtr, &Pos, Size, (char)va_arg( Args, int ), &Count );
                    break;
                    
                case 's': {
                    const char* Str = va_arg( Args, const char* );
                    Dmod_Print_String( BufPtr, &Pos, Size, Str, &Count );
                    break;
                }
                
                case 'd':
                case 'i': {
                    int32_t Value = va_arg( Args, int32_t );
                    Dmod_Print_Int( BufPtr, &Pos, Size, Value, &Count );
                    break;
                }
                
                case 'u': {
                    uint32_t Value = va_arg( Args, uint32_t );
                    Dmod_Print_UInt( BufPtr, &Pos, Size, Value, &Count );
                    break;
                }
                
                case 'x': {
                    uint32_t Value = va_arg( Args, uint32_t );
                    Dmod_Print_Hex( BufPtr, &Pos, Size, Value, false, &Count );
                    break;
                }
                
                case 'X': {
                    uint32_t Value = va_arg( Args, uint32_t );
                    Dmod_Print_Hex( BufPtr, &Pos, Size, Value, true, &Count );
                    break;
                }
                
                case 'p': {
                    void* Ptr = va_arg( Args, void* );
                    Dmod_Print_Pointer( BufPtr, &Pos, Size, Ptr, &Count );
                    break;
                }
                
                default:
                    // Unknown format specifier, just print it
                    Dmod_Print_Char( BufPtr, &Pos, Size, '%', &Count );
                    Dmod_Print_Char( BufPtr, &Pos, Size, *Format, &Count );
                    break;
            }
            Format++;
        }
        else
        {
            Dmod_Print_Char( BufPtr, &Pos, Size, *Format++, &Count );
        }
    }
    
    // Null-terminate if buffer is provided
    if( Buffer && Size > 0 )
    {
        Buffer[Pos] = '\0';
    }
    
    return Count;
}

int Dmod_SnPrintf_Impl( char* Buffer, size_t Size, const char* Format, ... )
{
    va_list Args;
    va_start( Args, Format );
    int Result = Dmod_VSnPrintf_Impl( Buffer, Size, Format, Args );
    va_end( Args );
    return Result;
}
