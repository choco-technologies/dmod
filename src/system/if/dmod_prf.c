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
#include <limits.h>

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

static void Dmod_Print_Int( char** Buffer, size_t* Pos, size_t Size, int32_t Value, int Width, bool LeftAlign, int* Count )
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

    // Build the forward (sign + digits) string and let Dmod_Print_String_Width
    // apply field-width padding, same as for %s.
    char Out[13];
    int OutLen = 0;
    if( IsNegative )
    {
        Out[OutLen++] = '-';
    }
    while( i > 0 )
    {
        Out[OutLen++] = Temp[--i];
    }
    Out[OutLen] = '\0';

    if( Width > 0 )
    {
        Dmod_Print_String_Width( Buffer, Pos, Size, Out, Width, LeftAlign, Count );
    }
    else
    {
        Dmod_Print_String( Buffer, Pos, Size, Out, Count );
    }
}

static void Dmod_Print_UInt( char** Buffer, size_t* Pos, size_t Size, uint32_t Value, int Width, bool LeftAlign, int* Count )
{
    char Temp[11]; // Enough for 4294967295
    int i = 0;

    // Convert to string (reversed)
    do
    {
        Temp[i++] = '0' + (Value % 10);
        Value /= 10;
    } while( Value > 0 );

    char Out[11];
    int OutLen = 0;
    while( i > 0 )
    {
        Out[OutLen++] = Temp[--i];
    }
    Out[OutLen] = '\0';

    if( Width > 0 )
    {
        Dmod_Print_String_Width( Buffer, Pos, Size, Out, Width, LeftAlign, Count );
    }
    else
    {
        Dmod_Print_String( Buffer, Pos, Size, Out, Count );
    }
}

static void Dmod_Print_Hex( char** Buffer, size_t* Pos, size_t Size, uint32_t Value, bool Uppercase, int Width, bool LeftAlign, int* Count )
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

    char Out[9];
    int OutLen = 0;
    while( i > 0 )
    {
        Out[OutLen++] = Temp[--i];
    }
    Out[OutLen] = '\0';

    if( Width > 0 )
    {
        Dmod_Print_String_Width( Buffer, Pos, Size, Out, Width, LeftAlign, Count );
    }
    else
    {
        Dmod_Print_String( Buffer, Pos, Size, Out, Count );
    }
}

static void Dmod_Print_Pointer( char** Buffer, size_t* Pos, size_t Size, void* Ptr, int Width, bool LeftAlign, int* Count )
{
    uintptr_t Value = (uintptr_t)Ptr;
    char Temp[17]; // Enough for 16 hex digits
    int i = 0;
    const char* HexDigits = "0123456789abcdef";

    // Convert to hex string (reversed)
    do
    {
        Temp[i++] = HexDigits[Value & 0xF];
        Value >>= 4;
    } while( Value > 0 );

    char Out[19]; // "0x" + up to 16 hex digits + '\0'
    int OutLen = 0;
    Out[OutLen++] = '0';
    Out[OutLen++] = 'x';
    while( i > 0 )
    {
        Out[OutLen++] = Temp[--i];
    }
    Out[OutLen] = '\0';

    if( Width > 0 )
    {
        Dmod_Print_String_Width( Buffer, Pos, Size, Out, Width, LeftAlign, Count );
    }
    else
    {
        Dmod_Print_String( Buffer, Pos, Size, Out, Count );
    }
}

static void Dmod_Print_LongLong( char** Buffer, size_t* Pos, size_t Size, int64_t Value, int Width, bool LeftAlign, int* Count )
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

    char Out[21];
    int OutLen = 0;
    if( IsNegative )
    {
        Out[OutLen++] = '-';
    }
    while( i > 0 )
    {
        Out[OutLen++] = Temp[--i];
    }
    Out[OutLen] = '\0';

    if( Width > 0 )
    {
        Dmod_Print_String_Width( Buffer, Pos, Size, Out, Width, LeftAlign, Count );
    }
    else
    {
        Dmod_Print_String( Buffer, Pos, Size, Out, Count );
    }
}

static void Dmod_Print_ULongLong( char** Buffer, size_t* Pos, size_t Size, uint64_t Value, int Width, bool LeftAlign, int* Count )
{
    char Temp[21]; // Enough for 18446744073709551615
    int i = 0;

    // Convert to string (reversed)
    do
    {
        Temp[i++] = '0' + (Value % 10);
        Value /= 10;
    } while( Value > 0 );

    char Out[21];
    int OutLen = 0;
    while( i > 0 )
    {
        Out[OutLen++] = Temp[--i];
    }
    Out[OutLen] = '\0';

    if( Width > 0 )
    {
        Dmod_Print_String_Width( Buffer, Pos, Size, Out, Width, LeftAlign, Count );
    }
    else
    {
        Dmod_Print_String( Buffer, Pos, Size, Out, Count );
    }
}

static void Dmod_Print_Hex64( char** Buffer, size_t* Pos, size_t Size, uint64_t Value, bool Uppercase, int Width, bool LeftAlign, int* Count )
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

    char Out[17];
    int OutLen = 0;
    while( i > 0 )
    {
        Out[OutLen++] = Temp[--i];
    }
    Out[OutLen] = '\0';

    if( Width > 0 )
    {
        Dmod_Print_String_Width( Buffer, Pos, Size, Out, Width, LeftAlign, Count );
    }
    else
    {
        Dmod_Print_String( Buffer, Pos, Size, Out, Count );
    }
}

static void Dmod_Print_Octal( char** Buffer, size_t* Pos, size_t Size, uint32_t Value, int Width, bool LeftAlign, int* Count )
{
    char Temp[12]; // Enough for 11 octal digits (32-bit)
    int i = 0;

    // Convert to octal string (reversed)
    do
    {
        Temp[i++] = '0' + (Value & 0x7);
        Value >>= 3;
    } while( Value > 0 );

    char Out[12];
    int OutLen = 0;
    while( i > 0 )
    {
        Out[OutLen++] = Temp[--i];
    }
    Out[OutLen] = '\0';

    if( Width > 0 )
    {
        Dmod_Print_String_Width( Buffer, Pos, Size, Out, Width, LeftAlign, Count );
    }
    else
    {
        Dmod_Print_String( Buffer, Pos, Size, Out, Count );
    }
}

static void Dmod_Print_Octal64( char** Buffer, size_t* Pos, size_t Size, uint64_t Value, int Width, bool LeftAlign, int* Count )
{
    char Temp[23]; // Enough for 22 octal digits (64-bit)
    int i = 0;

    // Convert to octal string (reversed)
    do
    {
        Temp[i++] = '0' + (Value & 0x7);
        Value >>= 3;
    } while( Value > 0 );

    char Out[23];
    int OutLen = 0;
    while( i > 0 )
    {
        Out[OutLen++] = Temp[--i];
    }
    Out[OutLen] = '\0';

    if( Width > 0 )
    {
        Dmod_Print_String_Width( Buffer, Pos, Size, Out, Width, LeftAlign, Count );
    }
    else
    {
        Dmod_Print_String( Buffer, Pos, Size, Out, Count );
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
            typedef enum {
                LEN_NONE,    // default (int/unsigned int)
                LEN_HH,      // char
                LEN_H,       // short
                LEN_L,       // long
                LEN_LL,      // long long
                LEN_Z        // size_t
            } LengthModifier;
            
            LengthModifier LenMod = LEN_NONE;
            
            if( *Format == 'h' )
            {
                if( *(Format + 1) == 'h' )
                {
                    LenMod = LEN_HH;
                    Format += 2;
                }
                else
                {
                    LenMod = LEN_H;
                    Format++;
                }
            }
            else if( *Format == 'l' )
            {
                if( *(Format + 1) == 'l' )
                {
                    LenMod = LEN_LL;
                    Format += 2;
                }
                else
                {
                    LenMod = LEN_L;
                    Format++;
                }
            }
            else if( *Format == 'z' )
            {
                LenMod = LEN_Z;
                Format++;
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
                    if( LenMod == LEN_LL )
                    {
                        int64_t Value = va_arg( Args, int64_t );
                        Dmod_Print_LongLong( BufPtr, &Pos, Size, Value, Width, LeftAlign, &Count );
                    }
                    else if( LenMod == LEN_L || LenMod == LEN_Z )
                    {
                        // long and size_t/ssize_t are typically same size
                        long Value = va_arg( Args, long );
                        #if LONG_MAX == INT64_MAX
                        Dmod_Print_LongLong( BufPtr, &Pos, Size, (int64_t)Value, Width, LeftAlign, &Count );
                        #else
                        Dmod_Print_Int( BufPtr, &Pos, Size, (int32_t)Value, Width, LeftAlign, &Count );
                        #endif
                    }
                    else if( LenMod == LEN_HH )
                    {
                        // char is promoted to int in varargs
                        int Value = va_arg( Args, int );
                        Dmod_Print_Int( BufPtr, &Pos, Size, (int32_t)(signed char)Value, Width, LeftAlign, &Count );
                    }
                    else if( LenMod == LEN_H )
                    {
                        // short is promoted to int in varargs
                        int Value = va_arg( Args, int );
                        Dmod_Print_Int( BufPtr, &Pos, Size, (int32_t)(short)Value, Width, LeftAlign, &Count );
                    }
                    else
                    {
                        int32_t Value = va_arg( Args, int32_t );
                        Dmod_Print_Int( BufPtr, &Pos, Size, Value, Width, LeftAlign, &Count );
                    }
                    break;
                }

                case 'u': {
                    if( LenMod == LEN_LL )
                    {
                        uint64_t Value = va_arg( Args, uint64_t );
                        Dmod_Print_ULongLong( BufPtr, &Pos, Size, Value, Width, LeftAlign, &Count );
                    }
                    else if( LenMod == LEN_L || LenMod == LEN_Z )
                    {
                        // unsigned long and size_t are typically same size
                        unsigned long Value = va_arg( Args, unsigned long );
                        #if ULONG_MAX == UINT64_MAX
                        Dmod_Print_ULongLong( BufPtr, &Pos, Size, (uint64_t)Value, Width, LeftAlign, &Count );
                        #else
                        Dmod_Print_UInt( BufPtr, &Pos, Size, (uint32_t)Value, Width, LeftAlign, &Count );
                        #endif
                    }
                    else if( LenMod == LEN_HH )
                    {
                        // unsigned char is promoted to int in varargs
                        unsigned int Value = va_arg( Args, unsigned int );
                        Dmod_Print_UInt( BufPtr, &Pos, Size, (uint32_t)(unsigned char)Value, Width, LeftAlign, &Count );
                    }
                    else if( LenMod == LEN_H )
                    {
                        // unsigned short is promoted to int in varargs
                        unsigned int Value = va_arg( Args, unsigned int );
                        Dmod_Print_UInt( BufPtr, &Pos, Size, (uint32_t)(unsigned short)Value, Width, LeftAlign, &Count );
                    }
                    else
                    {
                        uint32_t Value = va_arg( Args, uint32_t );
                        Dmod_Print_UInt( BufPtr, &Pos, Size, Value, Width, LeftAlign, &Count );
                    }
                    break;
                }
                
                case 'x': {
                    if( LenMod == LEN_LL )
                    {
                        uint64_t Value = va_arg( Args, uint64_t );
                        Dmod_Print_Hex64( BufPtr, &Pos, Size, Value, false, Width, LeftAlign, &Count );
                    }
                    else if( LenMod == LEN_L || LenMod == LEN_Z )
                    {
                        unsigned long Value = va_arg( Args, unsigned long );
                        #if ULONG_MAX == UINT64_MAX
                        Dmod_Print_Hex64( BufPtr, &Pos, Size, (uint64_t)Value, false, Width, LeftAlign, &Count );
                        #else
                        Dmod_Print_Hex( BufPtr, &Pos, Size, (uint32_t)Value, false, Width, LeftAlign, &Count );
                        #endif
                    }
                    else if( LenMod == LEN_HH )
                    {
                        unsigned int Value = va_arg( Args, unsigned int );
                        Dmod_Print_Hex( BufPtr, &Pos, Size, (uint32_t)(unsigned char)Value, false, Width, LeftAlign, &Count );
                    }
                    else if( LenMod == LEN_H )
                    {
                        unsigned int Value = va_arg( Args, unsigned int );
                        Dmod_Print_Hex( BufPtr, &Pos, Size, (uint32_t)(unsigned short)Value, false, Width, LeftAlign, &Count );
                    }
                    else
                    {
                        uint32_t Value = va_arg( Args, uint32_t );
                        Dmod_Print_Hex( BufPtr, &Pos, Size, Value, false, Width, LeftAlign, &Count );
                    }
                    break;
                }

                case 'X': {
                    if( LenMod == LEN_LL )
                    {
                        uint64_t Value = va_arg( Args, uint64_t );
                        Dmod_Print_Hex64( BufPtr, &Pos, Size, Value, true, Width, LeftAlign, &Count );
                    }
                    else if( LenMod == LEN_L || LenMod == LEN_Z )
                    {
                        unsigned long Value = va_arg( Args, unsigned long );
                        #if ULONG_MAX == UINT64_MAX
                        Dmod_Print_Hex64( BufPtr, &Pos, Size, (uint64_t)Value, true, Width, LeftAlign, &Count );
                        #else
                        Dmod_Print_Hex( BufPtr, &Pos, Size, (uint32_t)Value, true, Width, LeftAlign, &Count );
                        #endif
                    }
                    else if( LenMod == LEN_HH )
                    {
                        unsigned int Value = va_arg( Args, unsigned int );
                        Dmod_Print_Hex( BufPtr, &Pos, Size, (uint32_t)(unsigned char)Value, true, Width, LeftAlign, &Count );
                    }
                    else if( LenMod == LEN_H )
                    {
                        unsigned int Value = va_arg( Args, unsigned int );
                        Dmod_Print_Hex( BufPtr, &Pos, Size, (uint32_t)(unsigned short)Value, true, Width, LeftAlign, &Count );
                    }
                    else
                    {
                        uint32_t Value = va_arg( Args, uint32_t );
                        Dmod_Print_Hex( BufPtr, &Pos, Size, Value, true, Width, LeftAlign, &Count );
                    }
                    break;
                }

                case 'o': {
                    if( LenMod == LEN_LL )
                    {
                        uint64_t Value = va_arg( Args, uint64_t );
                        Dmod_Print_Octal64( BufPtr, &Pos, Size, Value, Width, LeftAlign, &Count );
                    }
                    else if( LenMod == LEN_L || LenMod == LEN_Z )
                    {
                        unsigned long Value = va_arg( Args, unsigned long );
                        #if ULONG_MAX == UINT64_MAX
                        Dmod_Print_Octal64( BufPtr, &Pos, Size, (uint64_t)Value, Width, LeftAlign, &Count );
                        #else
                        Dmod_Print_Octal( BufPtr, &Pos, Size, (uint32_t)Value, Width, LeftAlign, &Count );
                        #endif
                    }
                    else if( LenMod == LEN_HH )
                    {
                        unsigned int Value = va_arg( Args, unsigned int );
                        Dmod_Print_Octal( BufPtr, &Pos, Size, (uint32_t)(unsigned char)Value, Width, LeftAlign, &Count );
                    }
                    else if( LenMod == LEN_H )
                    {
                        unsigned int Value = va_arg( Args, unsigned int );
                        Dmod_Print_Octal( BufPtr, &Pos, Size, (uint32_t)(unsigned short)Value, Width, LeftAlign, &Count );
                    }
                    else
                    {
                        uint32_t Value = va_arg( Args, uint32_t );
                        Dmod_Print_Octal( BufPtr, &Pos, Size, Value, Width, LeftAlign, &Count );
                    }
                    break;
                }

                case 'p': {
                    void* Ptr = va_arg( Args, void* );
                    Dmod_Print_Pointer( BufPtr, &Pos, Size, Ptr, Width, LeftAlign, &Count );
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
                    if( LenMod == LEN_HH )
                    {
                        Dmod_Print_Char( BufPtr, &Pos, Size, 'h', &Count );
                        Dmod_Print_Char( BufPtr, &Pos, Size, 'h', &Count );
                    }
                    else if( LenMod == LEN_H )
                    {
                        Dmod_Print_Char( BufPtr, &Pos, Size, 'h', &Count );
                    }
                    else if( LenMod == LEN_L )
                    {
                        Dmod_Print_Char( BufPtr, &Pos, Size, 'l', &Count );
                    }
                    else if( LenMod == LEN_LL )
                    {
                        Dmod_Print_Char( BufPtr, &Pos, Size, 'l', &Count );
                        Dmod_Print_Char( BufPtr, &Pos, Size, 'l', &Count );
                    }
                    else if( LenMod == LEN_Z )
                    {
                        Dmod_Print_Char( BufPtr, &Pos, Size, 'z', &Count );
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
