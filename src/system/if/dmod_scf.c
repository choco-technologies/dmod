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
 * @file dmod_scf.c
 * @version 0.1
 */

#define DMOD_PRIVATE
#include "private/dmod_scf.h"
#include <stdint.h>
#include <stdbool.h>

//==============================================================================
//                              HELPER FUNCTIONS
//==============================================================================

/**
 * @brief Check if a character is a whitespace
 */
static inline bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

/**
 * @brief Check if a character is a digit
 */
static inline bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

/**
 * @brief Check if a character is a hex digit
 */
static inline bool is_xdigit(char c)
{
    return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

/**
 * @brief Convert hex character to value
 */
static inline int hex_to_int(char c)
{
    if (is_digit(c))
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    return 0;
}

/**
 * @brief Skip whitespace in input buffer
 */
static const char* skip_whitespace(const char* str)
{
    while (is_space(*str))
        str++;
    return str;
}

//==============================================================================
//                              IMPLEMENTATION
//==============================================================================

/**
 * @brief Minimal vsscanf implementation
 */
int Dmod_Vsscanf_Impl(const char* Buffer, const char* Format, va_list Args)
{
    int count = 0;
    const char* str = Buffer;
    
    if (Buffer == NULL || Format == NULL)
        return -1;
    
    while (*Format && *str)
    {
        // Handle whitespace in format
        if (is_space(*Format))
        {
            str = skip_whitespace(str);
            Format++;
            continue;
        }
        
        // Handle format specifier
        if (*Format == '%')
        {
            Format++;
            
            // Handle %%
            if (*Format == '%')
            {
                if (*str != '%')
                    return count;
                str++;
                Format++;
                continue;
            }
            
            // Skip whitespace before reading value
            str = skip_whitespace(str);
            
            // Handle format specifiers
            switch (*Format)
            {
                case 'c': // Character
                {
                    char* arg = va_arg(Args, char*);
                    if (arg == NULL || *str == '\0')
                        return count;
                    *arg = *str++;
                    count++;
                    break;
                }
                
                case 's': // String
                {
                    char* arg = va_arg(Args, char*);
                    if (arg == NULL || *str == '\0')
                        return count;
                    while (*str && !is_space(*str))
                        *arg++ = *str++;
                    *arg = '\0';
                    count++;
                    break;
                }
                
                case 'd': // Signed decimal
                case 'i': // Signed integer
                {
                    int* arg = va_arg(Args, int*);
                    if (arg == NULL || *str == '\0')
                        return count;
                    
                    int sign = 1;
                    if (*str == '-')
                    {
                        sign = -1;
                        str++;
                    }
                    else if (*str == '+')
                    {
                        str++;
                    }
                    
                    int value = 0;
                    bool found = false;
                    while (is_digit(*str))
                    {
                        value = value * 10 + (*str - '0');
                        str++;
                        found = true;
                    }
                    
                    if (!found)
                        return count;
                    
                    *arg = sign * value;
                    count++;
                    break;
                }
                
                case 'u': // Unsigned decimal
                {
                    unsigned int* arg = va_arg(Args, unsigned int*);
                    if (arg == NULL || *str == '\0')
                        return count;
                    
                    unsigned int value = 0;
                    bool found = false;
                    while (is_digit(*str))
                    {
                        value = value * 10 + (*str - '0');
                        str++;
                        found = true;
                    }
                    
                    if (!found)
                        return count;
                    
                    *arg = value;
                    count++;
                    break;
                }
                
                case 'x': // Hexadecimal
                case 'X':
                {
                    unsigned int* arg = va_arg(Args, unsigned int*);
                    if (arg == NULL || *str == '\0')
                        return count;
                    
                    // Skip optional 0x prefix
                    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
                        str += 2;
                    
                    unsigned int value = 0;
                    bool found = false;
                    while (is_xdigit(*str))
                    {
                        value = value * 16 + hex_to_int(*str);
                        str++;
                        found = true;
                    }
                    
                    if (!found)
                        return count;
                    
                    *arg = value;
                    count++;
                    break;
                }
                
                case 'p': // Pointer
                {
                    void** arg = va_arg(Args, void**);
                    if (arg == NULL || *str == '\0')
                        return count;
                    
                    // Skip optional 0x prefix
                    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
                        str += 2;
                    
                    uintptr_t value = 0;
                    bool found = false;
                    while (is_xdigit(*str))
                    {
                        value = value * 16 + hex_to_int(*str);
                        str++;
                        found = true;
                    }
                    
                    if (!found)
                        return count;
                    
                    *arg = (void*)value;
                    count++;
                    break;
                }
                
                default:
                    return count;
            }
            
            Format++;
        }
        else
        {
            // Literal character must match
            if (*Format != *str)
                return count;
            Format++;
            str++;
        }
    }
    
    return count;
}

/**
 * @brief Minimal sscanf implementation
 */
int Dmod_Sscanf_Impl(const char* Buffer, const char* Format, ...)
{
    va_list args;
    va_start(args, Format);
    int ret = Dmod_Vsscanf_Impl(Buffer, Format, args);
    va_end(args);
    return ret;
}
