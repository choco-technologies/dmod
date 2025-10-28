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
 * @file dmod_printf_impl.c
 * @version 0.1
 */

#include "dmod_printf_impl.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

//==============================================================================
//                              HELPER FUNCTIONS
//==============================================================================

static int dmod_strlen(const char* str)
{
    int len = 0;
    if (str == NULL) return 0;
    while (str[len]) len++;
    return len;
}

static void dmod_print_char(char** buffer, size_t* pos, size_t size, char ch, int* count)
{
    (*count)++;
    if (buffer && *buffer && *pos < size - 1) {
        (*buffer)[*pos] = ch;
        (*pos)++;
    }
}

static void dmod_print_string(char** buffer, size_t* pos, size_t size, const char* str, int* count)
{
    if (str == NULL) str = "(null)";
    while (*str) {
        dmod_print_char(buffer, pos, size, *str++, count);
    }
}

static void dmod_print_int(char** buffer, size_t* pos, size_t size, int32_t value, int* count)
{
    char temp[12]; // Enough for -2147483648
    int i = 0;
    bool is_negative = false;
    
    if (value < 0) {
        is_negative = true;
        value = -value;
    }
    
    // Convert to string (reversed)
    do {
        temp[i++] = '0' + (value % 10);
        value /= 10;
    } while (value > 0);
    
    // Add sign
    if (is_negative) {
        dmod_print_char(buffer, pos, size, '-', count);
    }
    
    // Print in correct order
    while (i > 0) {
        dmod_print_char(buffer, pos, size, temp[--i], count);
    }
}

static void dmod_print_uint(char** buffer, size_t* pos, size_t size, uint32_t value, int* count)
{
    char temp[11]; // Enough for 4294967295
    int i = 0;
    
    // Convert to string (reversed)
    do {
        temp[i++] = '0' + (value % 10);
        value /= 10;
    } while (value > 0);
    
    // Print in correct order
    while (i > 0) {
        dmod_print_char(buffer, pos, size, temp[--i], count);
    }
}

static void dmod_print_hex(char** buffer, size_t* pos, size_t size, uint32_t value, bool uppercase, int* count)
{
    char temp[9]; // Enough for 8 hex digits
    int i = 0;
    const char* hex_digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    
    // Convert to hex string (reversed)
    do {
        temp[i++] = hex_digits[value & 0xF];
        value >>= 4;
    } while (value > 0);
    
    // Print in correct order
    while (i > 0) {
        dmod_print_char(buffer, pos, size, temp[--i], count);
    }
}

static void dmod_print_pointer(char** buffer, size_t* pos, size_t size, void* ptr, int* count)
{
    uintptr_t value = (uintptr_t)ptr;
    char temp[17]; // Enough for 16 hex digits
    int i = 0;
    const char* hex_digits = "0123456789abcdef";
    
    // Print "0x" prefix
    dmod_print_char(buffer, pos, size, '0', count);
    dmod_print_char(buffer, pos, size, 'x', count);
    
    // Convert to hex string (reversed)
    do {
        temp[i++] = hex_digits[value & 0xF];
        value >>= 4;
    } while (value > 0);
    
    // Print in correct order
    while (i > 0) {
        dmod_print_char(buffer, pos, size, temp[--i], count);
    }
}

//==============================================================================
//                              PUBLIC FUNCTIONS
//==============================================================================

int dmod_vsnprintf_impl(char* buffer, size_t size, const char* format, va_list args)
{
    size_t pos = 0;
    int count = 0;
    char** buf_ptr = buffer ? &buffer : NULL;
    
    if (format == NULL) return 0;
    if (size == 0) buf_ptr = NULL;
    
    while (*format) {
        if (*format == '%') {
            format++;
            
            // Handle format specifiers
            switch (*format) {
                case '%':
                    dmod_print_char(buf_ptr, &pos, size, '%', &count);
                    break;
                    
                case 'c':
                    dmod_print_char(buf_ptr, &pos, size, (char)va_arg(args, int), &count);
                    break;
                    
                case 's': {
                    const char* str = va_arg(args, const char*);
                    dmod_print_string(buf_ptr, &pos, size, str, &count);
                    break;
                }
                
                case 'd':
                case 'i': {
                    int32_t value = va_arg(args, int32_t);
                    dmod_print_int(buf_ptr, &pos, size, value, &count);
                    break;
                }
                
                case 'u': {
                    uint32_t value = va_arg(args, uint32_t);
                    dmod_print_uint(buf_ptr, &pos, size, value, &count);
                    break;
                }
                
                case 'x': {
                    uint32_t value = va_arg(args, uint32_t);
                    dmod_print_hex(buf_ptr, &pos, size, value, false, &count);
                    break;
                }
                
                case 'X': {
                    uint32_t value = va_arg(args, uint32_t);
                    dmod_print_hex(buf_ptr, &pos, size, value, true, &count);
                    break;
                }
                
                case 'p': {
                    void* ptr = va_arg(args, void*);
                    dmod_print_pointer(buf_ptr, &pos, size, ptr, &count);
                    break;
                }
                
                default:
                    // Unknown format specifier, just print it
                    dmod_print_char(buf_ptr, &pos, size, '%', &count);
                    dmod_print_char(buf_ptr, &pos, size, *format, &count);
                    break;
            }
            format++;
        } else {
            dmod_print_char(buf_ptr, &pos, size, *format++, &count);
        }
    }
    
    // Null-terminate if buffer is provided
    if (buffer && size > 0) {
        buffer[pos] = '\0';
    }
    
    return count;
}

int dmod_snprintf_impl(char* buffer, size_t size, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int result = dmod_vsnprintf_impl(buffer, size, format, args);
    va_end(args);
    return result;
}
