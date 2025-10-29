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
 * @file dmod_prf.h
 * @version 0.1
 */

#ifndef DMOD_PRF_H
#define DMOD_PRF_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DMOD_PRIVATE
#   error "This is private DMOD header. Don't include this outside DMOD library"
#endif

#include <stdarg.h>
#include <stddef.h>

/**
 * @brief Minimal vsnprintf implementation
 * 
 * @param Buffer Output buffer (can be NULL to calculate required size)
 * @param Size Size of the buffer
 * @param Format Format string
 * @param Args Variable argument list
 * 
 * @return Number of characters that would have been written (excluding null terminator)
 * 
 * Supported format specifiers:
 * - %c: character
 * - %s: string
 * - %d, %i: signed integer
 * - %u: unsigned integer
 * - %x: hexadecimal (lowercase)
 * - %X: hexadecimal (uppercase)
 * - %p: pointer
 * - %%: literal %
 */
extern int Dmod_VSnPrintf_Impl( char* Buffer, size_t Size, const char* Format, va_list Args );

/**
 * @brief Minimal snprintf implementation
 * 
 * @param Buffer Output buffer (can be NULL to calculate required size)
 * @param Size Size of the buffer
 * @param Format Format string
 * 
 * @return Number of characters that would have been written (excluding null terminator)
 */
extern int Dmod_SnPrintf_Impl( char* Buffer, size_t Size, const char* Format, ... );

#ifdef __cplusplus
}
#endif

#endif // DMOD_PRF_H
