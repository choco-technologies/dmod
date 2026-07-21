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
 * @brief Kernel I/O interface for DMOD SAL
 *
 * Raw, unbuffered access to the kernel's standard output/input, bypassing stdio
 * and the Dmod_FileWrite/Dmod_FileRead/DMOD_STDIN/DMOD_STDOUT abstraction entirely.
 *
 * @file dmod_if_kernel.c
 * @version 0.1
 */

#include "dmod_sal.h"
#if DMOD_USE_DIRENT
#   include <unistd.h>
#endif
#if DMOD_USE_TERMIOS
#   include <termios.h>
#endif

//==============================================================================
//                              FUNCTIONS DECLARATIONS
//==============================================================================

/**
 * @brief Write a buffer directly to the kernel's raw stdout, bypassing any buffering
 *
 * @param Buffer Data to write
 * @param Size   Number of bytes to write
 *
 * @return Number of bytes actually written
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, size_t, _WriteKernel, ( const void* Buffer, size_t Size ))
{
    #if DMOD_USE_DIRENT
    ssize_t Written = write( 1, Buffer, Size );
    return Written < 0 ? 0 : (size_t)Written;
    #else
    DMOD_LOG_ERROR("Dmod_WriteKernel interface not implemented\n");
    return 0;
    #endif
}

/**
 * @brief Read a buffer directly from the kernel's raw stdin, bypassing any buffering
 *
 * @param Buffer Buffer to read into
 * @param Size   Number of bytes to read
 *
 * @return Number of bytes actually read
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, size_t, _ReadKernel, ( void* Buffer, size_t Size ))
{
    #if DMOD_USE_DIRENT
    ssize_t Read = read( 0, Buffer, Size );
    return Read < 0 ? 0 : (size_t)Read;
    #else
    DMOD_LOG_ERROR("Dmod_ReadKernel interface not implemented\n");
    return 0;
    #endif
}

/**
 * @brief Set the echo/canonical flags applied to the raw kernel console path
 *        (Dmod_ReadKernel()/Dmod_WriteKernel())
 *
 * @param Flags New flags (combination of DMOD_STDIN_FLAG_*)
 *
 * @return 0 on success, -1 on error
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _SetKernelInputFlags, ( uint32_t Flags ))
{
    #if DMOD_USE_TERMIOS
    struct termios term;

    if (tcgetattr(STDIN_FILENO, &term) != 0)
    {
        return -1;
    }

    term.c_lflag &= ~(ECHO | ICANON);
    if (Flags & DMOD_STDIN_FLAG_ECHO)      term.c_lflag |= ECHO;
    if (Flags & DMOD_STDIN_FLAG_CANONICAL) term.c_lflag |= ICANON;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &term) != 0)
    {
        return -1;
    }

    return 0;
    #else
    (void)Flags;
    DMOD_LOG_ERROR("Dmod_SetKernelInputFlags interface not implemented\n");
    return -1;
    #endif
}

/**
 * @brief Get the current echo/canonical flags applied to the raw kernel console path
 *        (Dmod_ReadKernel()/Dmod_WriteKernel())
 *
 * @return uint32_t Current flags (combination of DMOD_STDIN_FLAG_*)
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, uint32_t, _GetKernelInputFlags, ( void ))
{
    #if DMOD_USE_TERMIOS
    struct termios term;

    if (tcgetattr(STDIN_FILENO, &term) != 0)
    {
        return 0;
    }

    uint32_t flags = 0;
    if (term.c_lflag & ECHO)   flags |= DMOD_STDIN_FLAG_ECHO;
    if (term.c_lflag & ICANON) flags |= DMOD_STDIN_FLAG_CANONICAL;

    return flags;
    #else
    DMOD_LOG_ERROR("Dmod_GetKernelInputFlags interface not implemented\n");
    return 0;
    #endif
}
