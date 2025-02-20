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
 * @brief API for handling DMFC
 * @date 20 02 2025
 * 
 * The DMFC API is used to handle the DMFC (Dynamic Module File Compressed) in the system.
 */
#define DMOD_PRIVATE
#include "dmod.h"
#include <string.h>

//==============================================================================
//                              FUNCTION PROTOTYPES
//==============================================================================
/**
 * @addtogroup DMFC
 * @{
 */

 /**
  * @brief Checks if the given data is DMFC data
  * 
  * This function checks if the given data is DMFC data.
  * 
  * @param Data Data to check
  * @param Size Size of the data
  * 
  * @return true if the data is DMFC data, false otherwise
  */
bool Dmod_IsDMFC( const void* Data, size_t Size )
{
    if( Data == NULL || Size < sizeof(Dmod_DmfcHeader_t) )
    {
        return false;
    }

    Dmod_DmfcHeader_t* header = (Dmod_DmfcHeader_t*)Data;
    return header->Signature == DMOD_DMFC_SIGNATURE;
}

/**
 * @brief Checks if the given file is DMFC file
 * 
 * This function checks if the given file is DMFC file.
 * 
 * @param Path Path to the file
 * 
 * @return true if the file is DMFC file, false otherwise
 */
bool Dmod_IsDMFCFile( const char* Path )
{
    if( Path == NULL )
    {
        return false;
    }

    void* file = Dmod_FileOpen( Path, "rb" );
    if( file == NULL )
    {
        return false;
    }

    size_t fileSize = Dmod_FileSize( file );
    if( fileSize == 0 )
    {
        Dmod_FileClose( file );
        return false;
    }

    Dmod_DmfcHeader_t header;

    size_t read = Dmod_FileRead( &header, 1, sizeof(header), file );
    Dmod_FileClose( file );
    if( read != sizeof(header) )
    {
        return false;
    }

    bool result = Dmod_IsDMFC( &header, sizeof(header) );
    return result;
}

/**
 * @brief Convert DMF to DMFC
 * 
 * This function converts the DMF data to DMFC (Dynamic Module File Compressed) data.
 * 
 * @param CompressionName Compression algorithm name
 * @param Level Compression level
 * @param DmfData DMF data
 * @param DmfSize DMF size
 * @param outDmfcData Output DMFC data
 * @param outDmfcSize Output DMFC size
 * 
 * @return true if the conversion was successful, false otherwise
 */
 bool Dmod_ToDMFC( const char* CompressionName, int Level, const void* DmfData, size_t DmfSize, void** outDmfcData, size_t* outDmfcSize )
{
    if( DmfData == NULL || DmfSize == 0 || outDmfcData == NULL || outDmfcSize == NULL || CompressionName == NULL )
    {
        DMOD_LOG_ERROR("Cannot convert to DMFC - invalid parameters\n");
        return false;
    }

    if( strlen(CompressionName) > DMOD_MAX_COMPRESSION_NAME_LENGTH )
    {
        DMOD_LOG_ERROR("Cannot convert to DMFC - compression algorithm name is too long (max: %d)\n", DMOD_MAX_COMPRESSION_NAME_LENGTH);
        return false;
    }

    if( Dmod_Compression_IsSupported( CompressionName ) == false )
    {
        DMOD_LOG_ERROR("Cannot convert to DMFC - compression algorithm '%s' is not supported\n", CompressionName);
        return false;
    }
    
    size_t maxBufferSize = Dmod_Compression_GetMaxSize( CompressionName, Level, DmfSize );
    if( maxBufferSize == 0 )
    {
        DMOD_LOG_ERROR("Cannot convert to DMFC - failed to get max buffer size\n");
        return false;
    }
    size_t bufferSize = maxBufferSize + sizeof(Dmod_DmfcHeader_t);
    void* buffer = Dmod_Malloc( bufferSize );
    if( buffer == NULL )
    {
        DMOD_LOG_ERROR("Cannot convert to DMFC - failed to allocate buffer\n");
        return false;
    }
    Dmod_DmfcHeader_t* header = (Dmod_DmfcHeader_t*)buffer;
    
    header->Signature = DMOD_DMFC_SIGNATURE;
    header->HeaderSize = sizeof(Dmod_DmfcHeader_t);
    header->HeaderVersion = DMOD_DMFC_VERSION;
    strncpy( header->Compression, CompressionName, DMOD_MAX_COMPRESSION_NAME_LENGTH );
    header->OriginalSize = DmfSize;
    void* data = (void*)((uint8_t*)buffer + sizeof(Dmod_DmfcHeader_t));
    size_t compressedSize = Dmod_Compression_Pack( CompressionName, Level, data, maxBufferSize, DmfData, DmfSize );
    if( compressedSize == 0 )
    {
        DMOD_LOG_ERROR("Cannot convert to DMFC - failed to compress data\n");
        Dmod_Free( buffer );
        return false;
    }
    
    bufferSize = compressedSize + sizeof(Dmod_DmfcHeader_t);
    buffer = Dmod_Realloc( buffer, bufferSize );
    if( buffer == NULL )
    {
        DMOD_LOG_ERROR("Cannot convert to DMFC - failed to reallocate buffer\n");
        return false;
    }

    *outDmfcData = buffer;
    *outDmfcSize = bufferSize;

    return true;
}

/**
 * @brief Convert DMFC to DMF
 * 
 * This function converts the DMFC data to DMF (Dynamic Module File) data.
 * 
 * @param DmfcData DMFC data
 * @param DmfcSize DMFC size
 * @param outDmfData Output DMF data
 * @param outDmfSize Output DMF size
 * 
 * @return true if the conversion was successful, false otherwise
 */
bool Dmod_FromDMFC( const void* DmfcData, size_t DmfcSize, void** outDmfData, size_t* outDmfSize )
{
    if( DmfcData == NULL || DmfcSize == 0 || outDmfData == NULL || outDmfSize == NULL )
    {
        DMOD_LOG_ERROR("Cannot convert from DMFC - invalid parameters\n");
        return false;
    }

    if( DmfcSize < sizeof(Dmod_DmfcHeader_t) )
    {
        DMOD_LOG_ERROR("Cannot convert from DMFC - DMFC size is too small\n");
        return false;
    }

    Dmod_DmfcHeader_t* header = (Dmod_DmfcHeader_t*)DmfcData;
    if( header->Signature != DMOD_DMFC_SIGNATURE )
    {
        DMOD_LOG_ERROR("Cannot convert from DMFC - invalid DMFC signature\n");
        return false;
    }

    if( header->HeaderSize < sizeof(Dmod_DmfcHeader_t) )
    {
        DMOD_LOG_ERROR("Cannot convert from DMFC - invalid DMFC header size\n");
        return false;
    }

    if( !DMOD_DMFC_COMPATIBLE_VERSION(header->HeaderVersion) )
    {
        DMOD_LOG_ERROR("Cannot convert from DMFC - The given DMFC header version is not compatible: 0x%04X != \n", header->HeaderVersion, DMOD_DMFC_VERSION);
        return false;
    }

    if( header->OriginalSize == 0 )
    {
        DMOD_LOG_ERROR("Cannot convert from DMFC - invalid DMFC original size\n");
        return false;
    }

    if( header->OriginalSize > (DmfcSize - sizeof(Dmod_DmfcHeader_t)) )
    {
        DMOD_LOG_ERROR("Cannot convert from DMFC - invalid DMFC original size\n");
        return false;
    }

    if( strlen(header->Compression) > DMOD_MAX_COMPRESSION_NAME_LENGTH )
    {
        DMOD_LOG_ERROR("Cannot convert from DMFC - compression algorithm name is too long (max: %d)\n", DMOD_MAX_COMPRESSION_NAME_LENGTH);
        return false;
    }

    if( Dmod_Compression_IsSupported( header->Compression ) == false )
    {
        DMOD_LOG_ERROR("Cannot convert from DMFC - compression algorithm '%s' is not supported\n", header->Compression);
        return false;
    }

    void* buffer = Dmod_Malloc( header->OriginalSize );
    if( buffer == NULL )
    {
        DMOD_LOG_ERROR("Cannot convert from DMFC - failed to allocate buffer\n");
        return false;
    }
    void* srcData = (void*)((uint8_t*)DmfcData + sizeof(Dmod_DmfcHeader_t));
    size_t srcSize = DmfcSize - sizeof(Dmod_DmfcHeader_t);

    size_t decompressedSize = Dmod_Compression_Unpack( header->Compression, buffer, header->OriginalSize, srcData, srcSize );
    if( decompressedSize == 0 )
    {
        DMOD_LOG_ERROR("Cannot convert from DMFC - failed to decompress data\n");
        Dmod_Free( buffer );
        return false;
    }
    if( decompressedSize != header->OriginalSize )
    {
        DMOD_LOG_WARN("Decompressed size is different than the original size: %d != %d\n", decompressedSize, header->OriginalSize);
        buffer = Dmod_Realloc( buffer, decompressedSize );
        if( buffer == NULL )
        {
            DMOD_LOG_ERROR("Cannot convert from DMFC - failed to reallocate buffer\n");
            return false;
        }
    }
    *outDmfData = buffer;
    *outDmfSize = header->OriginalSize;

    return true;
}
