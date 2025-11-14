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
 * @brief Minimal scanf implementation for systems without stdio
 * @date 14 11 2025
 * 
 * This file provides a minimal scanf/sscanf/vsscanf implementation
 * that doesn't depend on stdio. It can be enabled with DMOD_IMPLEMENT_SCANF.
 *
 * @file dmod_scf.h
 * @version 0.1
 */

#ifndef DMOD_SCF_H
#define DMOD_SCF_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DMOD_PRIVATE
#   error "This is private DMOD header. Don't include this outside DMOD library"
#endif

#include <stdarg.h>
#include <stddef.h>

/**
 * @brief Minimal vsscanf implementation
 * 
 * @param Buffer Input buffer to scan from
 * @param Format Format string
 * @param Args Variable argument list
 * 
 * @return Number of input items successfully matched and assigned, 
 *         or EOF on error or end-of-input
 * 
 * Supported format specifiers:
 * - %c: character
 * - %s: string
 * - %d, %i: signed integer
 * - %u: unsigned integer
 * - %x, %X: hexadecimal
 * - %o: octal
 * - %p: pointer
 * - %%: literal %
 */
extern int Dmod_Vsscanf_Impl( const char* Buffer, const char* Format, va_list Args );

/**
 * @brief Minimal sscanf implementation
 * 
 * @param Buffer Input buffer to scan from
 * @param Format Format string
 * 
 * @return Number of input items successfully matched and assigned, 
 *         or EOF on error or end-of-input
 */
extern int Dmod_Sscanf_Impl( const char* Buffer, const char* Format, ... );

#ifdef __cplusplus
}
#endif

#endif // DMOD_SCF_H
