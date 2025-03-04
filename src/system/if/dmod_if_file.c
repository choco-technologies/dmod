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
 * @brief File interface for DMOD SAL
 * @date 20 gru 2024 10:46:38
 * 
 * @file dmod_if_file.c
 * 
 * @defgroup 
 * 
 * @version 0.1
 * 
 * 
 */

#include "dmod_sal.h"
#if DMOD_USE_STDIO
#   include <stdio.h>
#endif

//==============================================================================
//                              FUNCTIONS DECLARATIONS
//==============================================================================
/**
 * @brief Open file
 * 
 * @param Path Path to file
 * @param Mode Mode to open file
 * 
 * @return Pointer to file
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void*, _FileOpen, ( const char* Path, const char* Mode ))
{
    #if DMOD_USE_STDIO
    return fopen(Path, Mode);
    #else 
    DMOD_LOG_ERROR("Dmod_FileOpen interface not implemented");
    return NULL;
    #endif
}

/**
 * @brief Read file
 * 
 * @param File Pointer to file
 * @param Buffer Buffer to read data into
 * @param Size Size of data to read
 * @param Count Number of elements to read
 * 
 * @return Number of elements read
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, size_t, _FileRead, ( void* Buffer, size_t Size, size_t Count, void* File ))
{
    #if DMOD_USE_STDIO
    return fread(Buffer, Size, Count, File);
    #else
    DMOD_LOG_ERROR("Dmod_FileRead interface not implemented");
    return 0;
    #endif
}

/**
 * @brief Write file
 * 
 * @param File Pointer to file
 * @param Buffer Buffer to write data from
 * @param Size Size of data to write
 * @param Count Number of elements to write
 * 
 * @return Number of elements written
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, size_t, _FileWrite, ( const void* Buffer, size_t Size, size_t Count, void* File ))
{
    #if DMOD_USE_STDIO
    return fwrite(Buffer, Size, Count, File);
    #else
    DMOD_LOG_ERROR("Dmod_FileWrite interface not implemented");
    return 0;
    #endif
}

/**
 * @brief Seek file
 * 
 * @param File Pointer to file
 * @param Offset Offset to seek
 * @param Origin Origin of seek
 * 
 * @return 0 on success, -1 on error
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _FileSeek, ( void* File, long Offset, int Origin ))
{
    #if DMOD_USE_STDIO
    return fseek(File, Offset, Origin);
    #else 
    DMOD_LOG_ERROR("Dmod_FileSeek interface not implemented");
    return -1;
    #endif
}

/**
 * @brief Tell file
 * 
 * @param File Pointer to file
 * 
 * @return Current position in file
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, size_t, _FileTell, ( void* File ))
{
    #if DMOD_USE_STDIO
    return ftell(File);
    #else 
    DMOD_LOG_ERROR("Dmod_FileTell interface not implemented");
    return 0;
    #endif
}

/**
 * @brief Get file size
 * 
 * @param File Pointer to file
 * 
 * @return Size of file
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, size_t, _FileSize, ( void* File ))
{
    if( File == NULL )
    {
        return 0;
    }

    // Seek to end of file
    Dmod_FileSeek( File, 0, DMOD_SEEK_END );

    // Get current position
    size_t size = Dmod_FileTell( File );

    // Seek back to start
    Dmod_FileSeek( File, 0, DMOD_SEEK_SET );

    return size;
}

/**
 * @brief Close file
 * 
 * @param File Pointer to file
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void, _FileClose, ( void* File ))
{
    #if DMOD_USE_STDIO
    fclose(File);
    #else 
    DMOD_LOG_ERROR("Dmod_FileClose interface not implemented");
    #endif
}

/**
 * @brief Get repository directory
 * 
 * @return Path to repository directory
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, const char*, _GetRepoDir, ( void ))
{
    return DMOD_REPO_DIR;
}

/**
 * @brief Check if file is available
 * 
 * @param Path Path to file
 * 
 * @return True if file is available, false otherwise
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, bool, _FileAvailable, ( const char* Path ))
{
    void* file = Dmod_FileOpen( Path, "rb" );
    if( file == NULL )
    {
        return false;
    }
    Dmod_FileClose( file );
    return true;
}