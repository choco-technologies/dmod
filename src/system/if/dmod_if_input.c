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
 * @brief User Input interface for DMOD SAL
 * @date 2024-11-13
 * 
 * The user input interface is used to read data from the user in the system.
 *
 * @file dmod_if_input.c
 * @version 0.1
 */

#include "dmod_sal.h"
#if DMOD_USE_STDIO
#   include <stdio.h>
#   include <stdarg.h>
#endif
#if DMOD_IMPLEMENT_SCANF
#   define DMOD_PRIVATE
#   include "private/dmod_scf.h"
#   include <stdarg.h>
#endif
#if DMOD_USE_TERMIOS
#   include <termios.h>
#   include <unistd.h>
#endif

//==============================================================================
//                              FUNCTIONS DECLARATIONS
//==============================================================================

/**
 * @brief Getc function - reads a single character from standard input
 * 
 * @return Character read from standard input, or EOF on error
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _Getc, ( void ))
{
    int Result = EOF;
    void* resolvedFile = Dmod_LockStdio( DMOD_STDIN );
    if( resolvedFile == NULL )
    {
        unsigned char Byte;
        if( Dmod_ReadKernel( &Byte, 1 ) == 1 )
        {
            Result = (int)Byte;
        }
    }
    else
    {
        #if DMOD_USE_STDIO
        Result = fgetc( resolvedFile );
        #else 
        char c = 0;
        Dmod_FileRead(&c, 1, 1, resolvedFile);
        Result = c;
        #endif
    }
    Dmod_UnlockStdio( DMOD_STDIN );
    return Result;
}

/**
 * @brief Gets function - reads a string from standard input
 * 
 * @param Buffer Pointer to buffer where the string will be stored
 * @param Size Maximum number of characters to read (including null terminator)
 * 
 * @return Pointer to the buffer on success, NULL on error or EOF
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, char*, _Gets, ( char* Buffer, int Size ))
{
    if( Buffer == NULL || Size <= 0 )
    {
        return NULL;
    }

    char* Result = NULL;
    void* resolvedFile = Dmod_LockStdio( DMOD_STDIN );
    if( resolvedFile == NULL )
    {
        int Count = 0;
        unsigned char Byte;
        while( Count < (Size - 1) && Dmod_ReadKernel( &Byte, 1 ) == 1 )
        {
            Buffer[Count++] = (char)Byte;
            if( Byte == '\n' )
            {
                break;
            }
        }
        if( Count > 0 )
        {
            Buffer[Count] = '\0';
            Result = Buffer;
        }
    }
    else
    {
        #if DMOD_USE_STDIO
        Result = fgets( Buffer, Size, resolvedFile );
        #else 
        Result = Dmod_FileRead(Buffer, 1, Size, resolvedFile);
        #endif
    }
    Dmod_UnlockStdio( DMOD_STDIN );
    return Result;
}

/**
 * @brief Vsscanf function - reads formatted input from a string buffer with va_list
 * 
 * @param Buffer Input buffer to scan from
 * @param Format Format string specifying how to read the input
 * @param Args Variable argument list to store the read values
 * 
 * @return Number of input items successfully matched and assigned, 
 *         or EOF on error or end-of-input
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _Vsscanf, ( const char* Buffer, const char* Format, va_list Args ))
{
    #if DMOD_USE_STDIO
    if( Buffer == NULL || Format == NULL )
    {
        return EOF;
    }
    return vsscanf( Buffer, Format, Args );
    #elif DMOD_IMPLEMENT_SCANF
    return Dmod_Vsscanf_Impl( Buffer, Format, Args );
    #else
    (void)Buffer;
    (void)Format;
    (void)Args;
    return EOF;
    #endif
}

/**
 * @brief Sscanf function - reads formatted input from a string buffer
 * 
 * @param Buffer Input buffer to scan from
 * @param Format Format string specifying how to read the input
 * @param ... Variable arguments to store the read values
 * 
 * @return Number of input items successfully matched and assigned, 
 *         or EOF on error or end-of-input
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _Sscanf, ( const char* Buffer, const char* Format, ... ))
{
    int Ret = 0;
    va_list Args;
    va_start( Args, Format );
    Ret = Dmod_Vsscanf( Buffer, Format, Args );
    va_end( Args );
    return Ret;
}

/**
 * @brief Vscanf function - reads formatted input from standard input with va_list
 * 
 * @param Format Format string specifying how to read the input
 * @param Args Variable argument list to store the read values
 * 
 * @return Number of input items successfully matched and assigned, 
 *         or EOF on error or end-of-file
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _Vscanf, ( const char* Format, va_list Args ))
{
    int Ret = EOF;
    void* resolvedFile = Dmod_LockStdio( DMOD_STDIN );
    if( resolvedFile != NULL )
    {
        #if DMOD_USE_STDIO
        Ret = vfscanf( resolvedFile, Format, Args );
        #endif
    }
    else
    {
        #if DMOD_IMPLEMENT_SCANF
        // When custom scanf implementation is available but stdio is not,
        // read a line of input and use Vsscanf to parse it
        char buffer[256];
        char* result = Dmod_Gets( buffer, sizeof(buffer) );
        if( result != NULL )
        {
            Ret = Dmod_Vsscanf( buffer, Format, Args );
        }
        #endif
    }
    Dmod_UnlockStdio( DMOD_STDIN );
    return Ret;
}

/**
 * @brief Scanf function - reads formatted input from standard input
 * 
 * @param Format Format string specifying how to read the input
 * @param ... Variable arguments to store the read values
 * 
 * @return Number of input items successfully matched and assigned, 
 *         or EOF on error or end-of-file
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _Scanf, ( const char* Format, ... ))
{
    int Ret = 0;
    va_list Args;
    va_start( Args, Format );
    Ret = Dmod_Vscanf( Format, Args );
    va_end( Args );
    return Ret;
}

//==============================================================================
//                              STDIN FLAGS FUNCTIONS
//==============================================================================

/**
 * @brief Global variable to store stdin flags when termios is not available
 */
static uint32_t g_StdinFlags = DMOD_STDIN_FLAG_ECHO | DMOD_STDIN_FLAG_CANONICAL;

/**
 * @brief Get the current stdin flags
 * 
 * @return Current stdin flags (combination of DMOD_STDIN_FLAG_*)
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, uint32_t, _Stdin_GetFlags, ( void ))
{
    #if DMOD_USE_TERMIOS
    struct termios term;
    uint32_t flags = 0;
    
    if (tcgetattr(STDIN_FILENO, &term) != 0)
    {
        return g_StdinFlags;
    }
    
    if (term.c_lflag & ECHO)
    {
        flags |= DMOD_STDIN_FLAG_ECHO;
    }
    if (term.c_lflag & ICANON)
    {
        flags |= DMOD_STDIN_FLAG_CANONICAL;
    }
    
    return flags;
    #else
    return g_StdinFlags;
    #endif
}

/**
 * @brief Set the stdin flags
 * 
 * @param Flags New stdin flags (combination of DMOD_STDIN_FLAG_*)
 * 
 * @return 0 on success, -1 on error
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _Stdin_SetFlags, ( uint32_t Flags ))
{
    #if DMOD_USE_TERMIOS
    struct termios term;
    
    if (tcgetattr(STDIN_FILENO, &term) != 0)
    {
        return -1;
    }
    
    /* Clear the flags we're going to set */
    term.c_lflag &= ~(ECHO | ICANON);
    
    /* Set requested flags */
    if (Flags & DMOD_STDIN_FLAG_ECHO)
    {
        term.c_lflag |= ECHO;
    }
    if (Flags & DMOD_STDIN_FLAG_CANONICAL)
    {
        term.c_lflag |= ICANON;
    }
    
    if (tcsetattr(STDIN_FILENO, TCSANOW, &term) != 0)
    {
        return -1;
    }
    
    return 0;
    #else
    /* When termios is not available, just store the flags in the global variable */
    g_StdinFlags = Flags;
    return 0;
    #endif
}
