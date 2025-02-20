/**
 * MIT License
 * 
 * Copyright (c) 2023 [Your Name or Your Organization]
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
 * @brief Compresion interface for DMOD SAL
 * @date 20 12 2024 10:57:00
 * 
 * The compression interface is used to compress and decompress data in the system.
 *
 * @file dmod_if_comp.c
 * @version 0.1
 */

#include <string.h>
#include "dmod_sal.h"
#if DMOD_USE_FASTLZ
#   include "fastlz.h"
#endif

//==============================================================================
//                              FUNCTIONS DECLARATIONS
//==============================================================================
/**
 * @brief Compress data
 * 
 * @param Name Compression algorithm name
 * @param Level Compression level
 * @param Dest Destination buffer
 * @param DestSize Destination buffer size
 * @param Src Source buffer
 * @param SrcSize Source buffer size
 * 
 * @return Compressed data size
 */
size_t DMOD_WEAK_SYMBOL Dmod_Compress( const char* Name, int Level, void* Dest, size_t DestSize, const void* Src, size_t SrcSize )
{
    size_t Ret = 0;
    #if DMOD_USE_FASTLZ
    if( strcmp( Name, "fastlz" ) == 0 )
    {
        Ret = fastlz_compress_level(Level, Src, SrcSize, Dest );
    }
    #endif
    return Ret;
}

/**
 * @brief Decompress data
 * 
 * @param Name Compression algorithm name
 * @param Dest Destination buffer
 * @param DestSize Destination buffer size
 * @param Src Source buffer
 * @param SrcSize Source buffer size
 * 
 * @return Decompressed data size
 */
size_t DMOD_WEAK_SYMBOL Dmod_Decompress( const char* Name, void* Dest, size_t DestSize, const void* Src, size_t SrcSize )
{
    size_t Ret = 0;
    #if DMOD_USE_FASTLZ
    if(  strcmp( Name, "fastlz" ) == 0 )
    {
        Ret = fastlz_decompress( Src, SrcSize, Dest, DestSize );
    }
    #endif
    return Ret;
}

/**
 * @brief Check if compression algorithm is supported
 * 
 * @param Name Compression algorithm name
 * 
 * @return true if compression algorithm is supported, false otherwise
 */
bool DMOD_WEAK_SYMBOL Dmod_IsCompressionSupported( const char* Name )
{
    bool Ret = false;
    #if DMOD_USE_FASTLZ
    if( strcmp( Name, "fastlz" ) == 0)
    {
        Ret = true;
    }
    #endif
    return Ret;
}