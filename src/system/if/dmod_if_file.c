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

#define DMOD_PRIVATE
#include "dmod_sal.h"
#if DMOD_USE_STDIO
#   include <stdio.h>
#elif DMOD_IMPLEMENT_PRINTF
#   include "private/dmod_prf.h"
#endif
#if DMOD_USE_DIRENT
#   include <dirent.h>
#   include <sys/stat.h>
#   include <sys/types.h>
#   include <unistd.h>
#endif

#ifndef DMOD_VFPRINTF_STACK_BUFFER_SIZE
// Above this size, Dmod_VFPrintf falls back to a heap allocation instead of a stack buffer
#   define DMOD_VFPRINTF_STACK_BUFFER_SIZE 100
#endif

//==============================================================================
//                              FUNCTIONS DECLARATIONS
//==============================================================================

/**
 * @brief Get the file handle backing the DMOD_STDLOG stream
 *
 * By default this points at the same stream as DMOD_STDOUT. A platform-specific
 * implementation can override this weak function to redirect logging elsewhere
 * (a dedicated log file, UART, ...) without affecting DMOD_STDOUT.
 *
 * @return File handle used for the DMOD_STDLOG stream
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void*, _GetStdLogFile, ( void ))
{
    #if DMOD_USE_STDIO
    return stdout;
    #else
    return NULL;
    #endif
}

/**
 * @brief VFPrintf function - prints to a file using a va_list
 *
 * @param File File handle
 * @param Format Format string
 * @param Args Variable argument list
 *
 * @return Number of characters printed
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _VFPrintf, ( void* File, const char* Format, va_list Args ))
{
    #if DMOD_USE_STDIO
    void* resolvedFile = Dmod_ResolveStreamFile( Dmod_GetCurrentPid(), File );
    if( resolvedFile == NULL )
    {
        resolvedFile = stdout;
    }
    return vfprintf( resolvedFile, Format, Args );
    #elif DMOD_IMPLEMENT_PRINTF
    va_list ArgsCopy;
    va_copy( ArgsCopy, Args );
    int Len = Dmod_VSnPrintf_Impl( NULL, 0, Format, ArgsCopy );
    va_end( ArgsCopy );

    if( Len <= 0 )
    {
        return Len;
    }

    size_t Size = (size_t)Len + 1;

    if( Size <= DMOD_VFPRINTF_STACK_BUFFER_SIZE )
    {
        char Buffer[Size];
        Dmod_VSnPrintf_Impl( Buffer, Size, Format, Args );
        return (int)Dmod_FileWrite( Buffer, 1, (size_t)Len, File );
    }
    else
    {
        char* Buffer = (char*)Dmod_Malloc( Size );
        if( Buffer == NULL )
        {
            return -1;
        }
        Dmod_VSnPrintf_Impl( Buffer, Size, Format, Args );
        int Written = (int)Dmod_FileWrite( Buffer, 1, (size_t)Len, File );
        Dmod_Free( Buffer );
        return Written;
    }
    #else
    return 0;
    #endif
}

/**
 * @brief FPrintf function - prints to a file
 *
 * @param File File handle
 * @param Format Format string
 *
 * @return Number of characters printed
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _FPrintf, ( void* File, const char* Format, ... ))
{
    int Ret = 0;
    va_list Args;
    va_start( Args, Format );
    Ret = Dmod_VFPrintf( File, Format, Args );
    va_end( Args );
    return Ret;
}

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
    DMOD_LOG_ERROR("Dmod_FileOpen interface not implemented\n");
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
    size_t read = 0;
    if(File == NULL)
    {
        return read;
    }
    void* resolvedFile = Dmod_LockStdio(File);
    if(resolvedFile == NULL)
    {
        read = Dmod_ReadKernel(Buffer, Size * Count);
    }
    else
    {
        #if DMOD_USE_STDIO
        read = fread(Buffer, Size, Count, resolvedFile);
        #endif
        Dmod_UnlockStdio(File);
    }
    return read;
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
    size_t written = 0;
    if(File == NULL)
    {
        return written;
    }
    void* resolvedFile = Dmod_LockStdio(File);
    if(resolvedFile == NULL)
    {
        written = Dmod_WriteKernel(Buffer, Size * Count);
    }
    else 
    {
        #if DMOD_USE_STDIO
        written = fwrite(Buffer, Size, Count, resolvedFile);
        #endif
        Dmod_UnlockStdio(File);
    }
    return written;
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
    void* resolvedFile = Dmod_ResolveStreamFile( Dmod_GetCurrentPid(), File );
    if( resolvedFile == NULL )
    {
        // No real file bound for the current process/stream slot: seeking has no
        // meaning on the raw kernel I/O fallback used by Dmod_FileRead/Dmod_FileWrite.
        return -1;
    }
    return fseek(resolvedFile, Offset, Origin);
    #else
    DMOD_LOG_ERROR("Dmod_FileSeek interface not implemented\n");
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
    void* resolvedFile = Dmod_ResolveStreamFile( Dmod_GetCurrentPid(), File );
    if( resolvedFile == NULL )
    {
        // No real file bound for the current process/stream slot: there is no
        // position to report on the raw kernel I/O fallback.
        return 0;
    }
    return ftell(resolvedFile);
    #else
    DMOD_LOG_ERROR("Dmod_FileTell interface not implemented\n");
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
    if( File == DMOD_STDIN || File == DMOD_STDOUT || File == DMOD_STDERR || File == DMOD_STDLOG )
    {
        // Standard stream handles are not owned by the caller and must not be closed
        return;
    }

    #if DMOD_USE_STDIO
    fclose(File);
    #else
    DMOD_LOG_ERROR("Dmod_FileClose interface not implemented\n");
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

/**
 * @brief Open directory
 * 
 * @param Path Path to directory
 * 
 * @return Pointer to directory handle, NULL on error
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void*, _OpenDir, ( const char* Path ))
{
    #if DMOD_USE_DIRENT
    return opendir(Path);
    #else
    DMOD_LOG_ERROR("Dmod_OpenDir interface not implemented\n");
    return NULL;
    #endif
}

/**
 * @brief Read directory entry
 * 
 * @param Dir Pointer to directory handle
 * 
 * @return Name of the directory entry, NULL when no more entries
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, const char*, _ReadDir, ( void* Dir ))
{
    #if DMOD_USE_DIRENT
    struct dirent* entry = readdir((DIR*)Dir);
    return entry ? entry->d_name : NULL;
    #else
    DMOD_LOG_ERROR("Dmod_ReadDir interface not implemented\n");
    return NULL;
    #endif
}

/**
 * @brief Read directory entry with extended information
 * 
 * @param Dir Pointer to directory handle
 * 
 * @return Pointer to Dmod_DirEntry_t structure with entry information, NULL when no more entries
 * 
 * @note The returned pointer points to static storage that may be overwritten by subsequent calls.
 *       This behavior is consistent with the POSIX readdir() function and the original Dmod_ReadDir().
 *       Not thread-safe: use separate directory handles per thread.
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, const Dmod_DirEntry_t*, _ReadDirEx, ( void* Dir ))
{
    #if DMOD_USE_DIRENT
    static Dmod_DirEntry_t dirEntry;
    struct dirent* entry = readdir((DIR*)Dir);
    
    if (entry == NULL)
    {
        return NULL;
    }
    
    dirEntry.name = entry->d_name;
    dirEntry.type = Dmod_DirEntryType_Unknown;
    
    // Determine entry type based on d_type if available
    #if defined(_DIRENT_HAVE_D_TYPE)
    // Use constants from dirent.h: DT_REG=8, DT_DIR=4, DT_LNK=10
    // Direct comparison is used for compatibility with systems where these
    // constants might not be properly available in preprocessor context
    switch (entry->d_type)
    {
        case 8: // DT_REG - Regular file
            dirEntry.type = Dmod_DirEntryType_File;
            break;
        case 4: // DT_DIR - Directory
            dirEntry.type = Dmod_DirEntryType_Dir;
            break;
        case 10: // DT_LNK - Symbolic link
            dirEntry.type = Dmod_DirEntryType_Link;
            break;
        case 0: // DT_UNKNOWN
            dirEntry.type = Dmod_DirEntryType_Unknown;
            break;
        default: // Other types (socket, FIFO, etc.)
            dirEntry.type = Dmod_DirEntryType_Other;
            break;
    }
    #endif
    
    return &dirEntry;
    #else
    DMOD_LOG_ERROR("Dmod_ReadDirEx interface not implemented\n");
    return NULL;
    #endif
}

/**
 * @brief Close directory
 * 
 * @param Dir Pointer to directory handle
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void, _CloseDir, ( void* Dir ))
{
    #if DMOD_USE_DIRENT
    closedir((DIR*)Dir);
    #else
    DMOD_LOG_ERROR("Dmod_CloseDir interface not implemented\n");
    #endif
}

/**
 * @brief Create directory
 * 
 * @param Path Path to directory to create
 * @param Mode Directory permissions mode
 * 
 * @return 0 on success, -1 on error
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _MakeDir, ( const char* Path, int Mode ))
{
    #if DMOD_USE_DIRENT
    return mkdir(Path, (mode_t)Mode);
    #else
    DMOD_LOG_ERROR("Dmod_MakeDir interface not implemented\n");
    return -1;
    #endif
}

/**
 * @brief Check file accessibility
 * 
 * @param Path Path to file to check
 * @param Mode Access mode to test (R_OK, W_OK, X_OK, F_OK)
 * 
 * @return 0 on success (file accessible), -1 on error
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _Access, ( const char* Path, int Mode ))
{
    #if DMOD_USE_DIRENT
    return access(Path, Mode);
    #else
    // Fallback: check if file can be opened
    void* file = Dmod_FileOpen(Path, "rb");
    if (file) {
        Dmod_FileClose(file);
        return 0;
    }
    return -1;
    #endif
}

/**
 * @brief Read a single line from a file
 * 
 * Reads characters from the file into the buffer until either (Size - 1) 
 * characters have been read, a newline character is read and transferred
 * to Buffer, or end-of-file is reached. The string is then terminated
 * with a null character.
 * 
 * @param Buffer Pointer to buffer where the line will be stored
 * @param Size Maximum number of characters to read (including null terminator)
 * @param File Pointer to file handle
 * 
 * @return Pointer to the buffer on success, NULL on error or end-of-file
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, char*, _FileReadLine, ( char* Buffer, int Size, void* File ))
{
    #if DMOD_USE_STDIO
    if( Buffer == NULL || Size <= 0 || File == NULL )
    {
        return NULL;
    }
    return fgets( Buffer, Size, (FILE*)File );
    #else
    DMOD_LOG_ERROR("Dmod_FileReadLine interface not implemented\n");
    return NULL;
    #endif
}

/**
 * @brief Change current working directory
 * 
 * @param Path Path to the new working directory
 * 
 * @return 0 on success, -1 on error
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _ChDir, ( const char* Path ))
{
    #if DMOD_USE_DIRENT
    return chdir(Path);
    #else
    DMOD_LOG_ERROR("Dmod_ChDir interface not implemented\n");
    (void)Path;
    return -1;
    #endif
}

/**
 * @brief Get current working directory
 * 
 * @param Buffer Buffer to store the current working directory path
 * @param Size Size of the buffer
 * 
 * @return Pointer to Buffer on success, NULL on error
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, char*, _GetCwd, ( char* Buffer, size_t Size ))
{
    #if DMOD_USE_DIRENT
    return getcwd(Buffer, Size);
    #else
    return Dmod_GetEnv("PWD");
    #endif
}

/**
 * @brief Rename a file or directory
 * 
 * @param OldPath Current path of the file or directory
 * @param NewPath New path for the file or directory
 * 
 * @return 0 on success, -1 on error
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _Rename, ( const char* OldPath, const char* NewPath ))
{
    #if DMOD_USE_STDIO
    return rename(OldPath, NewPath);
    #else
    DMOD_LOG_ERROR("Dmod_Rename interface not implemented\n");
    (void)OldPath;
    (void)NewPath;
    return -1;
    #endif
}

/**
 * @brief Remove a directory
 * 
 * @param Path Path to the directory to remove
 * 
 * @return 0 on success, -1 on error
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _RemoveDir, ( const char* Path ))
{
    #if DMOD_USE_DIRENT
    return rmdir(Path);
    #else
    DMOD_LOG_ERROR("Dmod_RemoveDir interface not implemented\n");
    (void)Path;
    return -1;
    #endif
}

/**
 * @brief Remove a file
 * 
 * @param Path Path to the file to remove
 * 
 * @return 0 on success, -1 on error
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _FileRemove, ( const char* Path ))
{
    #if DMOD_USE_DIRENT
    return unlink(Path);
    #else
    DMOD_LOG_ERROR("Dmod_FileRemove interface not implemented\n");
    (void)Path;
    return -1;
    #endif
}