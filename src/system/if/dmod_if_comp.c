/**
 * MIT License
 * 
 * Copyright (c) 2025 patryk.kubiak90@gmail.com
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
#if DMOD_USE_DMCOMP
#   include "dmcomp.h"
#endif

//==============================================================================
//                              GLOBAL VARIABLES
//==============================================================================
static const char* SupportedCompressions[] = {
    #if DMOD_USE_FASTLZ
    "fastlz",       // The first one is the default of todmfc: it has to stay fastlz (the loaders that were built before deflate do not read the other)
    #endif
    #if DMOD_USE_DMCOMP
    "deflate",      // Raw DEFLATE (RFC 1951) by dmcomp: ~20 % smaller than fastlz, 1.7 kB of the decoder
    #endif
};
static const size_t SupportedCompressionsCount = sizeof(SupportedCompressions) / sizeof(SupportedCompressions[0]);

//==============================================================================
//                              FUNCTIONS DECLARATIONS
//==============================================================================

/**
 * @brief Get maximum compressed buffer size
 * 
 * @param Name Compression algorithm name
 * @param Level Compression level
 * @param SrcSize Source buffer size
 * 
 * @return Compressed buffer size
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, size_t, _Compression_GetMaxSize, ( const char* Name, int Level, size_t SrcSize ))
{
    size_t max_output_size = 0;
    if( SrcSize == 0 )
    {
        DMOD_LOG_ERROR("Cannot compress data with size 0\n");
        return 0;
    }
    #if DMOD_USE_FASTLZ
    else if( strcmp( Name, "fastlz" ) == 0 )
    {
        max_output_size = SrcSize + (SrcSize / 16) + 64 + 3;
    }
    #endif
    #if DMOD_USE_DMCOMP && DMOD_USE_DMCOMP_ENCODER
    else if( strcmp( Name, "deflate" ) == 0 )
    {
        max_output_size = dmcomp_deflate_bound( SrcSize );
    }
    #endif
    else 
    {
        DMOD_LOG_ERROR("Compression algorithm '%s' is not supported (or this build cannot compress with it)\n", Name);
    }

    return max_output_size;
}

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
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, size_t, _Compression_Pack, ( const char* Name, int Level, void* Dest, size_t DestSize, const void* Src, size_t SrcSize ))
{
    size_t Ret = 0;
    if( SrcSize == 0 )
    {
        DMOD_LOG_ERROR("Cannot compress data with size 0\n");
        return 0;
    }
    #if DMOD_USE_FASTLZ
    else if( strcmp( Name, "fastlz" ) == 0 )
    {
        Ret = fastlz_compress_level(Level, Src, SrcSize, Dest );
    }
    #endif
    #if DMOD_USE_DMCOMP && DMOD_USE_DMCOMP_ENCODER
    else if( strcmp( Name, "deflate" ) == 0 )
    {
        // Level 0 (stored) .. 9 (the smallest); dmcomp takes a level out of the range as its default
        int result = dmcomp_deflate( Src, SrcSize, Dest, DestSize, &Ret, Level );
        if( result != DMCOMP_OK )
        {
            DMOD_LOG_ERROR("Cannot compress with deflate: %s\n", dmcomp_strerror( result ));
            Ret = 0;
        }
    }
    #endif
    else 
    {
        DMOD_LOG_ERROR("Compression algorithm '%s' is not supported (or this build cannot compress with it)\n", Name);
    }

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
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, size_t, _Compression_Unpack, ( const char* Name, void* Dest, size_t DestSize, const void* Src, size_t SrcSize ))
{
    size_t Ret = 0;
    if( SrcSize == 0 )
    {
        DMOD_LOG_ERROR("Cannot decompress data with size 0\n");
        return 0;
    }
    #if DMOD_USE_FASTLZ
    if(  strcmp( Name, "fastlz" ) == 0 )
    {
        Ret = fastlz_decompress( Src, SrcSize, Dest, DestSize );
    }
    #endif
    #if DMOD_USE_DMCOMP
    #if DMOD_USE_FASTLZ
    else
    #endif
    if( strcmp( Name, "deflate" ) == 0 )
    {
        // The tables of the decoder (~3 kB) come from the heap, not from the stack: a module is loaded from whatever
        // thread asks for it, and some of them have 4 kB of the stack
        void* workspace = Dmod_Malloc( dmcomp_inflate_workspace_size() );
        if( workspace == NULL )
        {
            DMOD_LOG_ERROR("Cannot decompress with deflate: out of memory\n");
        }
        else
        {
            int result = dmcomp_inflate_ws( Src, SrcSize, Dest, DestSize, &Ret, workspace );
            if( result != DMCOMP_OK )
            {
                DMOD_LOG_ERROR("Cannot decompress with deflate: %s\n", dmcomp_strerror( result ));
                Ret = 0;
            }
            Dmod_Free( workspace );
        }
    }
    #endif
    #if DMOD_USE_FASTLZ || DMOD_USE_DMCOMP
    else 
    #endif
    {
        DMOD_LOG_ERROR("Compression algorithm '%s' is not supported\n", Name);
    }
    return Ret;
}

/**
 * @brief Check if compression algorithm is supported
 * 
 * @param Name Compression algorithm name
 * 
 * @return true if compression algorithm is supported, false otherwise
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, bool, _Compression_IsSupported, ( const char* Name ))
{
    bool result = false;

    for( size_t i = 0; i < SupportedCompressionsCount; i++ )
    {
        if( strcmp( SupportedCompressions[i], Name ) == 0 )
        {
            result = true;
            break;
        }
    }

    return result;
}

/**
 * @brief Get next supported compression algorithm
 * 
 * @param CompressionName Current compression algorithm name
 * 
 * @return Next supported compression algorithm name
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, const char*, _Compression_GetNextSupported, ( const char* CompressionName ))
{
    const char* next = NULL;

    if(CompressionName == NULL)
    {
        return SupportedCompressions[0];
    }

    for( size_t i = 0; i < SupportedCompressionsCount; i++ )
    {
        if( strcmp( SupportedCompressions[i], CompressionName ) == 0 )
        {
            if( (i + 1) < SupportedCompressionsCount )
            {
                next = SupportedCompressions[i + 1];
            }
            break;
        }
    }
    
    return next;
}