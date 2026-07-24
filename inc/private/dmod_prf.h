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
 * - %o: octal
 * - %p: pointer
 * - %f, %F: floating point (double; float is promoted to double through '...'),
 *           prints "nan"/"inf" for non-finite values
 * - %%: literal %
 *
 * Supported format modifiers:
 * - Width: Minimum field width (e.g., %30s for 30 characters)
 * - Left-align: '-' flag for left-justification (e.g., %-30s)
 * - Zero-pad: '0' flag for zero-padding numeric conversions (e.g., %02x,
 *             %05d) instead of the width default of spaces. Ignored for
 *             %s and for %p, and overridden by '-' when both are given
 *             (left-aligned fields are always space-padded), matching
 *             standard printf. A leading sign on %d/%lld/%f stays before
 *             the zero-padding (e.g. %04d of -5 is "-005", not "00-5").
 * - Precision: digits after the decimal point for %f/%F (e.g., %.3f), default 6,
 *              capped at 17 (a double's significant-digit limit)
 * - Length modifier 'hh': char (8-bit) for d, i, u, x, X, o (e.g., %hhd, %hhu)
 * - Length modifier 'h': short (16-bit) for d, i, u, x, X, o (e.g., %hd, %hu)
 * - Length modifier 'l': long for d, i, u, x, X, o (e.g., %ld, %lu, %lx)
 * - Length modifier 'll': long long (64-bit) for d, i, u, x, X, o (e.g., %lld, %llu, %llx, %llX)
 * - Length modifier 'z': size_t for d, i, u, x, X, o (e.g., %zu, %zd, %zx)
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
