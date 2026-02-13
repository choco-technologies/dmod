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
 * @brief DMOD SAL (System Abstraction Layer) header file
 * @date 2024-12-20
 * @author Patryk Kubiak <patryk.kubiak90@gmail.com>
 * 
 * This file contains the system abstraction layer (SAL) for DMOD
 * 
 * @file dmod_sal.h
 * 
 * @defgroup DMOD_SAL DMOD SAL
 * @ingroup DMOD
 * 
 * @version 0.1
 * 
 * The DMOD SAL provides an abstraction layer for the system. This allows the
 * DMOD to be used on different systems without changing the core code.
 * 
 * The default implementation is provided in the DMOD system as weak symbols.
 * 
 */
#ifndef INC_DMOD_SAL_H_
#define INC_DMOD_SAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dmod_types.h"
#include <stdarg.h>

//==============================================================================
//                              FUNCTION PROTOTYPES
//==============================================================================

/**
 * @defgroup DMOD_SAL_MEM Memory Interface
 * @ingroup DMOD_SAL
 * 
 * This interface is used to allocate and free memory in the system. 
 * The default implementation is provided in the DMOD system as weak symbols 
 * and uses the standard malloc and free functions.
 * 
 * @addtogroup DMOD_SAL_MEM
 * @{
 */
#define Dmod_Malloc(Size)                        Dmod_MallocEx(Size, Dmod_GetCurrentModuleName())
#define Dmod_Realloc(Ptr, Size)                  Dmod_ReallocEx(Ptr, Size, Dmod_GetCurrentModuleName())
#define Dmod_AlignedMalloc(Size, Alignment)      Dmod_AlignedMallocEx(Size, Alignment, Dmod_GetCurrentModuleName())
#define Dmod_Free(Ptr)                           Dmod_FreeEx(Ptr, false)

DMOD_BUILTIN_API(Dmod, 1.0, void*,  _MallocEx,          ( size_t Size, const char* ModuleName ) );
DMOD_BUILTIN_API(Dmod, 1.0, void*,  _ReallocEx,         ( void* Ptr, size_t Size, const char* ModuleName ) );
DMOD_BUILTIN_API(Dmod, 1.0, void ,  _FreeEx ,           ( void* Ptr, bool Concatenate ) );
DMOD_BUILTIN_API(Dmod, 1.0, void*,  _AlignedMallocEx,   ( size_t Size, size_t Alignment, const char* ModuleName ) );
DMOD_BUILTIN_API(Dmod, 1.0, void,   _FreeModule,        ( const char* ModuleName )          );
DMOD_BUILTIN_API(Dmod, 1.0, size_t, _ReadMemory,        ( uintptr_t Address, void* Buffer, size_t Size ) );
DMOD_BUILTIN_API(Dmod, 1.0, size_t, _WriteMemory,       ( uintptr_t Address, const void* Buffer, size_t Size ) );
//! @}

/**
 * @defgroup DMOD_SAL_FILE File Interface
 * @ingroup DMOD_SAL
 * 
 * This interface is used to open, read and close files in the system. 
 * The default implementation is provided in the DMOD system as weak symbols 
 * and uses the standard fopen, fread and fclose functions.
 * 
 * @addtogroup DMOD_SAL_FILE
 * @{
 */
DMOD_BUILTIN_API(Dmod, 1.0, void*       , _FileOpen,    ( const char* Path, const char* Mode ) );
DMOD_BUILTIN_API(Dmod, 1.0, size_t      , _FileRead,    ( void* Buffer, size_t Size, size_t Count, void* File ) );
DMOD_BUILTIN_API(Dmod, 1.0, size_t      , _FileWrite,   ( const void* Buffer, size_t Size, size_t Count, void* File ) );
DMOD_BUILTIN_API(Dmod, 1.0, int         , _FileSeek,    ( void* File, long Offset, int Origin ) );
DMOD_BUILTIN_API(Dmod, 1.0, size_t      , _FileTell,    ( void* File ) );
DMOD_BUILTIN_API(Dmod, 1.0, size_t      , _FileSize,    ( void* File ) );
DMOD_BUILTIN_API(Dmod, 1.0, void        , _FileClose,   ( void* File ) );
DMOD_BUILTIN_API(Dmod, 1.0, const char* , _GetRepoDir,  ( void ) );
DMOD_BUILTIN_API(Dmod, 1.0, bool        , _FileAvailable, ( const char* Path ) );
DMOD_BUILTIN_API(Dmod, 1.0, void*                   , _OpenDir,     ( const char* Path ) );
DMOD_BUILTIN_API(Dmod, 1.0, const char*             , _ReadDir,     ( void* Dir ) );
DMOD_BUILTIN_API(Dmod, 1.0, const Dmod_DirEntry_t* , _ReadDirEx,   ( void* Dir ) );
DMOD_BUILTIN_API(Dmod, 1.0, void                    , _CloseDir,    ( void* Dir ) );
DMOD_BUILTIN_API(Dmod, 1.0, int         , _MakeDir,     ( const char* Path, int Mode ) );
DMOD_BUILTIN_API(Dmod, 1.0, int         , _Access,      ( const char* Path, int Mode ) );
DMOD_BUILTIN_API(Dmod, 1.0, char*       , _FileReadLine, ( char* Buffer, int Size, void* File ) );
DMOD_BUILTIN_API(Dmod, 1.0, int         , _ChDir,       ( const char* Path ) );
DMOD_BUILTIN_API(Dmod, 1.0, char*       , _GetCwd,      ( char* Buffer, size_t Size ) );
DMOD_BUILTIN_API(Dmod, 1.0, int         , _Rename,      ( const char* OldPath, const char* NewPath ) );
DMOD_BUILTIN_API(Dmod, 1.0, int         , _RemoveDir,   ( const char* Path ) );
DMOD_BUILTIN_API(Dmod, 1.0, int         , _FileRemove,  ( const char* Path ) );

#ifndef DMOD_SEEK_SET
#   define DMOD_SEEK_SET   0
#   define DMOD_SEEK_CUR   1
#   define DMOD_SEEK_END   2
#endif

#ifndef DMOD_R_OK
#   define DMOD_R_OK   4  /* Test for read permission */
#   define DMOD_W_OK   2  /* Test for write permission */
#   define DMOD_X_OK   1  /* Test for execute permission */
#   define DMOD_F_OK   0  /* Test for existence */
#endif

#ifndef DMOD_EOF
#   define DMOD_EOF    (-1)
#endif
#ifndef EOF
#   define EOF        DMOD_EOF
#endif

//! @}

/**
 * @defgroup DMOD_SAL_ENV Environment Interface
 * @ingroup DMOD_SAL
 * 
 * This interface is used to get and set environment variables in the system.
 * 
 * @addtogroup DMOD_SAL_ENV
 * @{
 */

#ifdef DMOD_MODULE_NAME
#   define Dmod_GetCurrentModuleName()      Dmod_GetCurrentModuleNameEx(DMOD_MODULE_NAME)
#else
#   define Dmod_GetCurrentModuleName()      Dmod_GetCurrentModuleNameEx(NULL)
#endif

DMOD_BUILTIN_API(Dmod, 1.0, const char*, _GetEnv, ( const char* Name ) );
DMOD_BUILTIN_API(Dmod, 1.0, int, _SetEnv, ( const char* Name, const char* Value, int Overwrite ) );
DMOD_BUILTIN_API(Dmod, 1.0, int, _Unsetenv, ( const char* Name ) );
DMOD_BUILTIN_API(Dmod, 1.0, const char*, _GetNextEnvName, ( const char* Last ) );
DMOD_BUILTIN_API(Dmod, 1.0, int, _EnvCtx_Push, ( void ) );
DMOD_BUILTIN_API(Dmod, 1.0, int, _EnvCtx_Pop, ( void ) );
DMOD_BUILTIN_API(Dmod, 1.0, const char*, _GetCurrentModuleNameEx, ( const char* Default ) );

//! @}

/**
 * @defgroup DMOD_SAL_DEBUG Debug Interface
 * @ingroup DMOD_SAL
 * 
 * This interface is used to print debug messages in the system.
 * The default implementation is provided in the DMOD system as weak symbols
 * and uses the standard printf and assert functions.
 * 
 * @addtogroup DMOD_SAL_DEBUG
 * @{
 */
DMOD_BUILTIN_API( Dmod, 1.0, bool ,_CheckLogLevel, ( Dmod_LogLevel_t LogLevel ) );
DMOD_BUILTIN_API( Dmod, 1.0, int  ,_Printf, ( const char* Format, ... ) );
DMOD_BUILTIN_API( Dmod, 1.0, int  ,_FPrintf, ( void* File, const char* Format, ... ) );
DMOD_BUILTIN_API( Dmod, 1.0, int  ,_VSnPrintf, ( char* Buffer, size_t Size, const char* Format, va_list Args ) );
DMOD_BUILTIN_API( Dmod, 1.0, int  ,_SnPrintf, ( char* Buffer, size_t Size, const char* Format, ... ) );
DMOD_BUILTIN_API( Dmod, 1.0, void ,_Assert, ( int Condition, const char* Message, const char* File, int Line, const char* Function ) );

#ifdef NDEBUG
#   define DMOD_ASSERT_MSG( Condition, Message )           ((void)0)
#else 
#   define DMOD_ASSERT_MSG( Condition, Message )           Dmod_Assert( Condition, Message, __FILE__, __LINE__, __func__ )
#endif
#ifdef DMOD_NO_LOGGING
#   define DMOD_LOG(...)                                   ((void)0)    
#else
#   ifdef DMOD_MODULE_NAME
#   define DMOD_LOG(LogLevel,...)                          \
                                if(Dmod_CheckLogLevel(LogLevel)) {\
                                    Dmod_Printf( DMOD_MODULE_NAME ": " __VA_ARGS__ );\
                                    Dmod_Printf( "\033[0m" );\
                                }
#   else 
#   define DMOD_LOG(LogLevel,...)                          \
                                if(Dmod_CheckLogLevel(LogLevel)) {\
                                    Dmod_Printf( __VA_ARGS__ );\
                                    Dmod_Printf( "\033[0m" );\
                                }
#   endif
#endif 

#ifdef DMOD_LOG_LEVEL
#   if DMOD_LOG_LEVEL < Dmod_LogLevel_Verbose
#       define DMOD_LOG_VERBOSE(...)
#   endif
#   if DMOD_LOG_LEVEL < Dmod_LogLevel_Info
#       define DMOD_LOG_INFO(...)
#   endif
#   if DMOD_LOG_LEVEL < Dmod_LogLevel_Warn
#       define DMOD_LOG_WARN(...)
#   endif
#   if DMOD_LOG_LEVEL < Dmod_LogLevel_Error
#       define DMOD_LOG_ERROR(...)
#   endif
#endif 

#define DMOD_ASSERT( Condition )        DMOD_ASSERT_MSG( Condition, #Condition )

#ifndef DMOD_LOG_VERBOSE
#   define DMOD_LOG_VERBOSE(...)    DMOD_LOG( Dmod_LogLevel_Verbose, "\033[35;1m[VERBOSE] " __VA_ARGS__ ); 
#endif

#ifndef DMOD_LOG_INFO
#   define DMOD_LOG_INFO(...)      DMOD_LOG( Dmod_LogLevel_Info, "\033[34;1m[INFO] " __VA_ARGS__ ); 
#endif

#ifndef DMOD_LOG_WARN
#   define DMOD_LOG_WARN(...)      DMOD_LOG( Dmod_LogLevel_Warn, "\033[33;1m[WARN] " __VA_ARGS__ ); 
#endif

#ifndef DMOD_LOG_ERROR
#   define DMOD_LOG_ERROR(...)     DMOD_LOG( Dmod_LogLevel_Error, "\033[31;1m[ERROR] " __VA_ARGS__ ); 
#endif

//! @}

/**
 * @defgroup DMOD_SAL_IRQ IRQ Interface
 * @ingroup DMOD_SAL
 * 
 * This interface is used to handle interrupts in the system.
 * 
 * @addtogroup DMOD_SAL_CONTEXT
 * @{
 */

DMOD_BUILTIN_API( Dmod, 1.0, void, _EnterCritical   , ( void ) );
DMOD_BUILTIN_API( Dmod, 1.0, void, _ExitCritical    , ( void ) );

//! @}

/**
 * @defgroup DMOD_SAL_EVENT Events Interface
 * @ingroup DMOD_SAL
 * 
 * 
 * 
 * @addtogroup DMOD_SAL_CONTEXT
 * @{
 */

DMOD_BUILTIN_API(Dmod, 1.0, void, _Event_ModuleLoaded, ( Dmod_Context_t* Context ) );
DMOD_BUILTIN_API(Dmod, 1.0, void, _Event_ModuleUnloaded, ( Dmod_Context_t* Context ) );
DMOD_BUILTIN_API(Dmod, 1.0, void, _Event_ModuleLoadingInProgress, ( const char* Name, uint16_t Progress ) );
DMOD_BUILTIN_API(Dmod, 1.0, void, _Event_ModuleEnabled, ( Dmod_Context_t* Context ) );
DMOD_BUILTIN_API(Dmod, 1.0, void, _Event_ModuleDisabled, ( Dmod_Context_t* Context ) );
DMOD_BUILTIN_API(Dmod, 1.0, void, _Event_ModuleRunning, ( Dmod_Context_t* Context ) );
DMOD_BUILTIN_API(Dmod, 1.0, void, _Event_ModuleStopped, ( Dmod_Context_t* Context ) );

//! @}

/**
 * @defgroup DMOD_SAL_RTOS RTOS Interface
 * @ingroup DMOD_SAL
 * 
 * This interface is used to handle real-time operating system (RTOS) in the system.
 * 
 * @addtogroup DMOD_SAL_RTOS
 * @{
 */

DMOD_BUILTIN_API(Dmod, 1.0, void*, _Mutex_New, ( bool Recursive ) );
DMOD_BUILTIN_API(Dmod, 1.0, int  , _Mutex_Lock, ( void* Mutex ) );
DMOD_BUILTIN_API(Dmod, 1.0, int  , _Mutex_Unlock, ( void* Mutex ) );
DMOD_BUILTIN_API(Dmod, 1.0, void , _Mutex_Delete, ( void* Mutex ) );

//! @}

/**
 * @defgroup DMOD_SAL_COMPRESSION Compression Interface
 * @ingroup DMOD_SAL
 * 
 * This interface is used to compress and decompress data in the system.
 * 
 * @addtogroup DMOD_SAL_COMPRESSION
 * @{
 */

DMOD_BUILTIN_API(Dmod, 1.0, size_t     , _Compression_GetMaxSize,       ( const char* Name, int Level, size_t SrcSize ) );
DMOD_BUILTIN_API(Dmod, 1.0, size_t     , _Compression_Pack,             ( const char* Name, int Level, void* Dest, size_t DestSize, const void* Src, size_t SrcSize ) );
DMOD_BUILTIN_API(Dmod, 1.0, size_t     , _Compression_Unpack,           ( const char* Name, void* Dest, size_t DestSize, const void* Src, size_t SrcSize ) );
DMOD_BUILTIN_API(Dmod, 1.0, bool       , _Compression_IsSupported,      ( const char* Name ) );
DMOD_BUILTIN_API(Dmod, 1.0, const char*, _Compression_GetNextSupported, ( const char* CompressionName ));

//! @}

/**
 * @defgroup DMOD_SAL_STRING String Interface
 * @ingroup DMOD_SAL
 * 
 * This interface is used to handle strings in the system.
 * 
 * @addtogroup DMOD_SAL_STRING
 * @{
 */

DMOD_BUILTIN_API(Dmod, 1.0, char*     , _StrDup,        ( const char* Str ) );

//! @}

/**
 * @defgroup DMOD_SAL_INPUT User Input Interface
 * @ingroup DMOD_SAL
 * 
 * This interface is used to read data from the user.
 * The default implementation is provided in the DMOD system as weak symbols
 * and uses the standard getc, fgets, and scanf functions.
 * 
 * @addtogroup DMOD_SAL_INPUT
 * @{
 */

DMOD_BUILTIN_API(Dmod, 1.0, int  , _Getc,    ( void ) );
DMOD_BUILTIN_API(Dmod, 1.0, char*, _Gets,    ( char* Buffer, int Size ) );
DMOD_BUILTIN_API(Dmod, 1.0, int  , _Vsscanf, ( const char* Buffer, const char* Format, va_list Args ) );
DMOD_BUILTIN_API(Dmod, 1.0, int  , _Sscanf,  ( const char* Buffer, const char* Format, ... ) );
DMOD_BUILTIN_API(Dmod, 1.0, int  , _Vscanf,  ( const char* Format, va_list Args ) );
DMOD_BUILTIN_API(Dmod, 1.0, int  , _Scanf,   ( const char* Format, ... ) );

/**
 * @brief Stdin flags for terminal I/O control
 */
#define DMOD_STDIN_FLAG_ECHO        (1 << 0)  /**< Enable echo of input characters */
#define DMOD_STDIN_FLAG_CANONICAL   (1 << 1)  /**< Enable canonical (line-buffered) mode */

DMOD_BUILTIN_API(Dmod, 1.0, uint32_t, _Stdin_GetFlags, ( void ) );
DMOD_BUILTIN_API(Dmod, 1.0, int     , _Stdin_SetFlags, ( uint32_t Flags ) );

//! @}

/**
 * @defgroup DMOD_SAL_PROC Process Interface
 * @ingroup DMOD_SAL
 * 
 * This interface is used to control process execution in the system.
 * The default implementation is provided in the DMOD system as weak symbols
 * and uses the standard exit function.
 * 
 * @addtogroup DMOD_SAL_PROC
 * @{
 */

DMOD_BUILTIN_API(Dmod, 1.0, void, _Exit, ( int Status ) );
DMOD_BUILTIN_API(Dmod, 1.0, int, _Spawn, ( Dmod_Context_t* Context, int argc, char *argv[] ) );
DMOD_BUILTIN_API(Dmod, 1.0, int, _RunDetached, ( Dmod_Context_t* Context, int argc, char *argv[] ) );

//! @}

#ifdef __cplusplus
}
#endif
#endif /* INC_DMOD_SAL_H_ */
