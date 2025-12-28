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
#include "private/dmod_prf.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

//==============================================================================
//                              HELPER FUNCTIONS
//==============================================================================

// Maximum field width to prevent integer overflow and unreasonable buffer usage
#define DMOD_PRINTF_MAX_WIDTH 1024

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

static void Dmod_Print_String_Width( char** Buffer, size_t* Pos, size_t Size, const char* Str, int Width, bool LeftAlign, int* Count )
{
    if( Str == NULL ) Str = "(null)";
    
    int StrLen = Dmod_StrLen( Str );
    int PadLen = Width - StrLen;
    
    // If string is longer than or equal to width, no padding needed
    if( PadLen <= 0 )
    {
        while( *Str )
        {
            Dmod_Print_Char( Buffer, Pos, Size, *Str++, Count );
        }
        return;
    }
    
    // Left-aligned: print string first, then padding
    if( LeftAlign )
    {
        while( *Str )
        {
            Dmod_Print_Char( Buffer, Pos, Size, *Str++, Count );
        }
        for( int i = 0; i < PadLen; i++ )
        {
            Dmod_Print_Char( Buffer, Pos, Size, ' ', Count );
        }
    }
    // Right-aligned: print padding first, then string
    else
    {
        for( int i = 0; i < PadLen; i++ )
        {
            Dmod_Print_Char( Buffer, Pos, Size, ' ', Count );
        }
        while( *Str )
        {
            Dmod_Print_Char( Buffer, Pos, Size, *Str++, Count );
        }
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
        // Handle most negative int32_t value (-2147483648, 0x80000000) specially to avoid overflow
        // Since -(-2147483648) cannot be represented in int32_t, we use the unsigned equivalent
        if( Value == (int32_t)0x80000000 )
        {
            UValue = 0x80000000u;
        }
        else
        {
            UValue = (uint32_t)(-Value);
        }
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

static void Dmod_Print_LongLong( char** Buffer, size_t* Pos, size_t Size, int64_t Value, int* Count )
{
    char Temp[21]; // Enough for -9223372036854775808
    int i = 0;
    bool IsNegative = false;
    uint64_t UValue;
    
    if( Value < 0 )
    {
        IsNegative = true;
        // Handle most negative int64_t value specially to avoid overflow
        if( Value == (int64_t)0x8000000000000000LL )
        {
            UValue = 0x8000000000000000ULL;
        }
        else
        {
            UValue = (uint64_t)(-Value);
        }
    }
    else
    {
        UValue = (uint64_t)Value;
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

static void Dmod_Print_ULongLong( char** Buffer, size_t* Pos, size_t Size, uint64_t Value, int* Count )
{
    char Temp[21]; // Enough for 18446744073709551615
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

static void Dmod_Print_Hex64( char** Buffer, size_t* Pos, size_t Size, uint64_t Value, bool Uppercase, int* Count )
{
    char Temp[17]; // Enough for 16 hex digits
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
            
            // Parse flags
            bool LeftAlign = false;
            if( *Format == '-' )
            {
                LeftAlign = true;
                Format++;
            }
            
            // Parse width
            int Width = 0;
            while( *Format >= '0' && *Format <= '9' )
            {
                int NewWidth = Width * 10 + (*Format - '0');
                // Prevent overflow by capping at maximum width
                if( NewWidth > DMOD_PRINTF_MAX_WIDTH )
                {
                    Width = DMOD_PRINTF_MAX_WIDTH;
                    // Skip remaining digits
                    while( *Format >= '0' && *Format <= '9' )
                    {
                        Format++;
                    }
                    break;
                }
                Width = NewWidth;
                Format++;
            }
            
            // Parse length modifier
            bool IsLongLong = false;
            if( *Format == 'l' && *(Format + 1) == 'l' )
            {
                IsLongLong = true;
                Format += 2;
            }
            
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
                    if( Width > 0 )
                    {
                        Dmod_Print_String_Width( BufPtr, &Pos, Size, Str, Width, LeftAlign, &Count );
                    }
                    else
                    {
                        Dmod_Print_String( BufPtr, &Pos, Size, Str, &Count );
                    }
                    break;
                }
                
                case 'd':
                case 'i': {
                    if( IsLongLong )
                    {
                        int64_t Value = va_arg( Args, int64_t );
                        Dmod_Print_LongLong( BufPtr, &Pos, Size, Value, &Count );
                    }
                    else
                    {
                        int32_t Value = va_arg( Args, int32_t );
                        Dmod_Print_Int( BufPtr, &Pos, Size, Value, &Count );
                    }
                    break;
                }
                
                case 'u': {
                    if( IsLongLong )
                    {
                        uint64_t Value = va_arg( Args, uint64_t );
                        Dmod_Print_ULongLong( BufPtr, &Pos, Size, Value, &Count );
                    }
                    else
                    {
                        uint32_t Value = va_arg( Args, uint32_t );
                        Dmod_Print_UInt( BufPtr, &Pos, Size, Value, &Count );
                    }
                    break;
                }
                
                case 'x': {
                    if( IsLongLong )
                    {
                        uint64_t Value = va_arg( Args, uint64_t );
                        Dmod_Print_Hex64( BufPtr, &Pos, Size, Value, false, &Count );
                    }
                    else
                    {
                        uint32_t Value = va_arg( Args, uint32_t );
                        Dmod_Print_Hex( BufPtr, &Pos, Size, Value, false, &Count );
                    }
                    break;
                }
                
                case 'X': {
                    if( IsLongLong )
                    {
                        uint64_t Value = va_arg( Args, uint64_t );
                        Dmod_Print_Hex64( BufPtr, &Pos, Size, Value, true, &Count );
                    }
                    else
                    {
                        uint32_t Value = va_arg( Args, uint32_t );
                        Dmod_Print_Hex( BufPtr, &Pos, Size, Value, true, &Count );
                    }
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
                    if( LeftAlign ) Dmod_Print_Char( BufPtr, &Pos, Size, '-', &Count );
                    // Print width digits if any
                    if( Width > 0 )
                    {
                        char WidthStr[12];
                        int i = 0;
                        int TempWidth = Width;
                        do
                        {
                            WidthStr[i++] = '0' + (TempWidth % 10);
                            TempWidth /= 10;
                        } while( TempWidth > 0 );
                        while( i > 0 )
                        {
                            Dmod_Print_Char( BufPtr, &Pos, Size, WidthStr[--i], &Count );
                        }
                    }
                    // Print length modifier if present
                    if( IsLongLong )
                    {
                        Dmod_Print_Char( BufPtr, &Pos, Size, 'l', &Count );
                        Dmod_Print_Char( BufPtr, &Pos, Size, 'l', &Count );
                    }
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
