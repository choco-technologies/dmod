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
#if defined(DMOD_MODULE_NAME)
#   if defined(DMOD_LIBRARY_MODULE) && DMOD_LIBRARY_MODULE == 1
#       define DMOD_CURRENT_ALLOCATOR                        DMOD_MODULE_NAME
#   elif defined(DMOD_APPLICATION_MODULE) && DMOD_APPLICATION_MODULE == 1
#       define DMOD_CURRENT_ALLOCATOR                        Dmod_GetCurrentAllocatorName()
#   else
#       error DMOD_LIBRARY_MODULE neither DMOD_APPLICATION_MODULE is defined
#   endif
#elif !defined(DMOD_CURRENT_ALLOCATOR)
/*
 * On the system side there is no DMOD_MODULE_NAME to fall back on, and passing
 * NULL (as this used to do) tags every allocation as anonymous, which makes
 * per-owner tracking (dmheap and friends) useless for whatever links this in.
 * Every system-side library/binary must therefore define DMOD_CURRENT_ALLOCATOR
 * itself - normally as a compiler definition set to its own name - before this
 * header is included. dmod's create_library_makefile()/dmod_add_tool() CMake
 * helpers do this automatically, using the CMake target's own name.
 */
#   error DMOD_CURRENT_ALLOCATOR must be defined manually on the system side (typically as a compiler definition, e.g. -DDMOD_CURRENT_ALLOCATOR='"<library-name>"') to the name of the library defining it
#endif
#define Dmod_Malloc(Size)                        Dmod_MallocEx(Size, DMOD_CURRENT_ALLOCATOR)
#define Dmod_Realloc(Ptr, Size)                  Dmod_ReallocEx(Ptr, Size, DMOD_CURRENT_ALLOCATOR)
#define Dmod_AlignedMalloc(Size, Alignment)      Dmod_AlignedMallocEx(Size, Alignment, DMOD_CURRENT_ALLOCATOR)
#define Dmod_MallocTagged(Tags, Size)            Dmod_MallocTaggedEx(Tags, Size, DMOD_CURRENT_ALLOCATOR)
#define Dmod_Free(Ptr)                           Dmod_FreeEx(Ptr, false)
DMOD_BUILTIN_API(Dmod, 1.0, void*,  _MallocEx,          ( size_t Size, const char* ModuleName ) );
DMOD_BUILTIN_API(Dmod, 1.0, void*,  _ReallocEx,         ( void* Ptr, size_t Size, const char* ModuleName ) );
DMOD_BUILTIN_API(Dmod, 1.0, void ,  _FreeEx ,           ( void* Ptr, bool Concatenate ) );
DMOD_BUILTIN_API(Dmod, 1.0, void*,  _AlignedMallocEx,   ( size_t Size, size_t Alignment, const char* ModuleName ) );
/**
 * @brief Allocate memory of a kind - the system's heaps tagged with it - else the default memory.
 *
 * Tags is a list of the kinds wanted, separated by commas, the preferred one first: "fast"
 * (internal RAM worth giving to what is worked on all the time), "dma" (memory DMA reaches
 * without cache maintenance), ... - as the system tags its heaps (dmheap: a heap's name is
 * the list of its tags). A kind the system does not have, or one without room: the next one,
 * at last what Dmod_MallocEx() gives - so a module asks for what it gains from and still works
 * everywhere. Freed with Dmod_Free(). The default implementation is Dmod_MallocEx().
 *
 * @param Tags       The kinds of memory, preferred first (NULL or "": the default memory)
 * @param Size       Size of memory to allocate
 * @param ModuleName Allocation owner, as Dmod_MallocEx() understands it
 *
 * @return Pointer to the allocated memory, NULL when there is none
 */
DMOD_BUILTIN_API(Dmod, 1.0, void*,  _MallocTaggedEx,    ( const char* Tags, size_t Size, const char* ModuleName ) );
DMOD_BUILTIN_API(Dmod, 1.0, void,   _FreeModule,        ( const char* ModuleName )          );
DMOD_BUILTIN_API(Dmod, 1.0, bool,   _RetagEx,           ( void* Ptr, const char* ModuleName ) );
DMOD_BUILTIN_API(Dmod, 1.0, bool,   _RenameTag,         ( const char* OldTag, const char* NewTag ) );
DMOD_BUILTIN_API(Dmod, 1.0, size_t, _ReadMemory,        ( uintptr_t Address, void* Buffer, size_t Size ) );
DMOD_BUILTIN_API(Dmod, 1.0, size_t, _WriteMemory,       ( uintptr_t Address, const void* Buffer, size_t Size ) );

/**
 * @brief Make code just written to memory safe to execute
 *
 * Called by the module loader once a module is loaded and relocated: on a
 * core with caches, the code may still sit in the data cache, and the
 * instruction cache may hold what was at these addresses before (a module
 * unloaded from there). The system cleans the data cache for the range and
 * invalidates the instruction cache. The default weak implementation does
 * nothing - right for cores without caches and for hosts.
 *
 * @param Address Start of the code
 * @param Size    Its size in bytes
 */
DMOD_BUILTIN_API(Dmod, 1.0, void,   _SyncCode,          ( const void* Address, size_t Size ) );
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
DMOD_BUILTIN_API(Dmod, 2.0, int               , _FileSeek, ( void* File, Dmod_FileOffset_t Offset, int Origin ) );
DMOD_BUILTIN_API(Dmod, 2.0, Dmod_FileOffset_t , _FileTell, ( void* File ) );
DMOD_BUILTIN_API(Dmod, 2.0, Dmod_FileSize_t   , _FileSize, ( void* File ) );
DMOD_BUILTIN_API(Dmod, 2.0, int               , _FileStat, ( const char* Path, Dmod_FileStat_t* Stat ) );
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

/**
 * @brief Issue a driver-specific ioctl on a file (including DMOD_STDIN/OUT/ERR/LOG)
 *
 * DMOD_STDIN/OUT/ERR/LOG are resolved to the calling process's bound stream file
 * first, exactly like Dmod_FileRead/Dmod_FileWrite - if nothing is bound (raw
 * kernel I/O fallback), this returns a negative error code, since there is no
 * underlying driver to forward the ioctl to.
 *
 * @param File File handle, or one of DMOD_STDIN/DMOD_STDOUT/DMOD_STDERR/DMOD_STDLOG
 * @param Command Driver-specific ioctl command
 * @param Arg Command-specific argument
 * @return 0 on success, negative error code on failure (including "unsupported")
 */
DMOD_BUILTIN_API(Dmod, 1.0, int         , _Ioctl,       ( void* File, int Command, void* Arg ) );

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
#ifndef DMOD_FILE_OFFSET_ERROR
#   define DMOD_FILE_OFFSET_ERROR ((Dmod_FileOffset_t)-1)
#endif
/** Checked conversion for APIs that must allocate an address-space-sized buffer. */
static inline bool Dmod_FileSizeToSizeT(Dmod_FileSize_t FileSize, size_t* outSize)
{
    if( outSize == NULL || FileSize > (Dmod_FileSize_t)SIZE_MAX )
    {
        return false;
    }
    *outSize = (size_t)FileSize;
    return true;
}
#ifndef EOF
#   define EOF        DMOD_EOF
#endif

/**
 * @brief Standard stream handles
 *
 * These special values can be passed wherever a file handle (`void* File`) is expected -
 * e.g. Dmod_FPrintf, Dmod_FileRead, Dmod_FileWrite, Dmod_FileSeek, Dmod_FileTell,
 * Dmod_FileClose - instead of a handle returned by Dmod_FileOpen, to target the standard
 * input/output/error streams.
 *
 * DMOD_STDLOG is a separate, platform-configurable logging stream: by default it resolves
 * to the same stream as DMOD_STDOUT (see Dmod_GetStdLogFile), but a platform-specific
 * implementation can override Dmod_GetStdLogFile to redirect it elsewhere (a dedicated log
 * file, UART, ...) without affecting DMOD_STDOUT.
 */
#define DMOD_STDIN     ((void*)1)
#define DMOD_STDOUT    ((void*)2)
#define DMOD_STDERR    ((void*)3)
#define DMOD_STDLOG    ((void*)4)

DMOD_BUILTIN_API(Dmod, 1.0, void*, _GetStdLogFile,      ( void ) );
DMOD_BUILTIN_API(Dmod, 1.0, void*, _GetStreamLogFile,   ( Dmod_LogLevel_t LogLevel ));
DMOD_BUILTIN_API(Dmod, 1.0, int  , _VFPrintf,           ( void* File, const char* Format, va_list Args ) );
DMOD_BUILTIN_API(Dmod, 1.0, int  , _FPrintf,            ( void* File, const char* Format, ... ) );

//! @}

/**
 * @defgroup DMOD_SAL_KERNEL Kernel I/O Interface
 * @ingroup DMOD_SAL
 *
 * This interface gives raw, unbuffered access to the kernel's standard output/input -
 * bypassing stdio buffering and the Dmod_FileWrite/Dmod_FileRead/DMOD_STDIN/DMOD_STDOUT
 * abstraction entirely. It is meant for contexts where buffering must not happen, e.g.
 * crash/panic handlers, signal handlers, or output emitted before stdio has been initialized.
 *
 * The default implementation uses the raw POSIX write(2)/read(2) syscalls on file
 * descriptors 1 (stdout) and 0 (stdin).
 *
 * @addtogroup DMOD_SAL_KERNEL
 * @{
 */
DMOD_BUILTIN_API(Dmod, 1.0, size_t,   _WriteKernel, ( const void* Buffer, size_t Size ) );
DMOD_BUILTIN_API(Dmod, 1.0, size_t,   _ReadKernel,  ( void* Buffer, size_t Size ) );
DMOD_BUILTIN_API(Dmod, 1.0, int,      _SetKernelInputFlags, ( uint32_t Flags ) );
DMOD_BUILTIN_API(Dmod, 1.0, uint32_t, _GetKernelInputFlags, ( void ) );

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

/**
 * @brief Name used to attribute heap allocations (Dmod_Malloc/Dmod_MallocEx/...) to their owner.
 *
 * This is deliberately a *separate* identity from Dmod_GetCurrentModuleName(): the module name
 * is not unique when the same module is loaded more than once at the same time (e.g. a shell
 * spawning another instance of itself in the background) - two independent Dmod_Context_t's
 * end up sharing one name. Allocation tracking (dmheap and similar) keys everything off this
 * string, including bulk-freeing all of a module's memory on unload (Dmod_FreeModule) - if two
 * live instances shared that key, unloading one would free memory the other is still using.
 *
 * The default (weak) implementation reads Context->AllocatorName off Dmod_GetCurrentContext() -
 * a per-instance-unique string ("<module name>@<context address>") generated once when the
 * context's header is loaded, so it works for any backend that can report which context is
 * currently executing. Falls back to Default when there is no current context (e.g. no real
 * process tracking is available at all).
 */
#ifdef DMOD_MODULE_NAME
#   define Dmod_GetCurrentAllocatorName()      Dmod_GetCurrentAllocatorNameEx(DMOD_MODULE_NAME)
#else
#   define Dmod_GetCurrentAllocatorName()      Dmod_GetCurrentAllocatorNameEx(NULL)
#endif

DMOD_BUILTIN_API(Dmod, 1.0, const char*, _GetEnv, ( const char* Name ) );
DMOD_BUILTIN_API(Dmod, 1.0, int, _SetEnv, ( const char* Name, const char* Value, int Overwrite ) );
DMOD_BUILTIN_API(Dmod, 1.0, int, _Unsetenv, ( const char* Name ) );
DMOD_BUILTIN_API(Dmod, 1.0, const char*, _GetNextEnvName, ( const char* Last ) );
DMOD_BUILTIN_API(Dmod, 1.0, int, _EnvCtx_Push, ( void ) );
DMOD_BUILTIN_API(Dmod, 1.0, int, _EnvCtx_Pop, ( void ) );
DMOD_BUILTIN_API(Dmod, 1.0, const char*, _GetCurrentModuleNameEx, ( const char* Default ) );
DMOD_BUILTIN_API(Dmod, 1.0, const char*, _GetCurrentAllocatorNameEx, ( const char* Default ) );
DMOD_BUILTIN_API(Dmod, 1.0, Dmod_Context_t*, _GetCurrentContext, ( void ) );

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
DMOD_BUILTIN_API( Dmod, 1.0, int  ,_VPrintf, ( const char* Format, va_list Args ) );
DMOD_BUILTIN_API( Dmod, 1.0, int  ,_Printf, ( const char* Format, ... ) );
DMOD_BUILTIN_API( Dmod, 1.0, int  ,_VSnPrintf, ( char* Buffer, size_t Size, const char* Format, va_list Args ) );
DMOD_BUILTIN_API( Dmod, 1.0, int  ,_SnPrintf, ( char* Buffer, size_t Size, const char* Format, ... ) );
DMOD_BUILTIN_API( Dmod, 1.0, void        ,_Assert,      ( int Condition, const char* Message, const char* File, int Line, const char* Function ) );
DMOD_BUILTIN_API( Dmod, 1.0, const char* ,_GetStepBar,  ( int Percent ) );

#ifdef NDEBUG
#   define DMOD_ASSERT_MSG( Condition, Message )           ((void)0)
#else 
#   define DMOD_ASSERT_MSG( Condition, Message )           Dmod_Assert( Condition, Message, __FILE__, __LINE__, __func__ )
#endif
#ifdef DMOD_NO_LOGGING
#   define DMOD_LOG(...)                                   ((void)0)
#   define DMOD_LOG_STEP_BEGIN(...)                        ((void)0)
#   define DMOD_LOG_STEP_PROGRESS(Percent, ...)            ((void)0)
#   define DMOD_LOG_STEP(Result, ...)                      ((void)0)
#else
#   ifdef DMOD_MODULE_NAME
#       define DMOD_LOG_MODULE_PREFIX  "\033[37;1m" DMOD_MODULE_NAME ": \033[0m"
#   else
#       define DMOD_LOG_MODULE_PREFIX  ""
#   endif
#   if defined(DMOD_MODULE_NAME) && (DMOD_MODULE_EN == ON)
#       define DMOD_LOG(LogLevel,...)                      \
                                if(Dmod_CheckLogLevel(LogLevel)) {\
                                    Dmod_FPrintf( Dmod_GetStreamLogFile(LogLevel), DMOD_LOG_MODULE_PREFIX __VA_ARGS__ );\
                                    Dmod_FPrintf( Dmod_GetStreamLogFile(LogLevel), "\033[0m" );\
                                }
#   else
#       define DMOD_LOG(LogLevel,...)                      \
                                if(Dmod_CheckLogLevel(LogLevel)) {\
                                    Dmod_FPrintf( Dmod_GetStreamLogFile(LogLevel), DMOD_LOG_MODULE_PREFIX __VA_ARGS__ );\
                                    Dmod_FPrintf( Dmod_GetStreamLogFile(LogLevel), "\033[0m" );\
                                }
#   endif
#   define DMOD_LOG_STEP_BEGIN(...)                        \
                                do { \
                                    Dmod_Printf( "\r\033[K[\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91] " DMOD_LOG_MODULE_PREFIX __VA_ARGS__ ); \
                                    Dmod_Printf( "\r" ); \
                                } while(0)
#   define DMOD_LOG_STEP_PROGRESS(Percent, ...)            \
                                do { \
                                    Dmod_Printf( "\r\033[K%s " DMOD_LOG_MODULE_PREFIX, Dmod_GetStepBar(Percent) ); \
                                    Dmod_Printf( __VA_ARGS__ ); \
                                    Dmod_Printf( "\r" ); \
                                } while(0)
#   define DMOD_LOG_STEP(Result, ...)                      \
                                do { \
                                    if ((int)(Result) == 0) { \
                                        Dmod_Printf( "\r\033[K\033[32;1m[  OK  ]\033[0m " DMOD_LOG_MODULE_PREFIX __VA_ARGS__ ); \
                                        Dmod_Printf( "\033[0m" ); \
                                    } else { \
                                        Dmod_Printf( "\r\033[K\033[31;1m[ FAIL ]\033[0m " DMOD_LOG_MODULE_PREFIX __VA_ARGS__ ); \
                                        Dmod_Printf( "\033[0m" ); \
                                    } \
                                } while(0)
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

/**
 * @brief Check whether the caller is running in interrupt/exception context
 *
 * Used by the stdio path to keep Dmod_Printf/DMOD_LOG_* callable from an ISR:
 * when this returns true, Dmod_LockStdio() skips stream resolution and routes
 * the write straight to Dmod_WriteKernel() (the raw kernel log ring), and
 * Dmod_VFPrintf() stays on its stack buffer instead of calling Dmod_Malloc().
 * Both of those would otherwise take RTOS locks that are illegal from an ISR.
 *
 * The default weak implementation returns false, which preserves the previous
 * behaviour on platforms that do not (or cannot) report interrupt context -
 * they simply keep resolving streams exactly as before. Platform backends
 * override it (e.g. dmosi-freertos maps it onto xPortIsInsideInterrupt()).
 *
 * Implementations must not log and must not take any lock - this is called
 * from inside the logging path itself, so anything else recurses.
 *
 * @return true if the caller is inside an interrupt/exception handler
 */
DMOD_BUILTIN_API( Dmod, 1.0, bool, _IsInsideInterrupt, ( void ) );

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
DMOD_BUILTIN_API(Dmod, 1.0, void*, _Semaphore_New, ( uint32_t InitialValue, uint32_t MaxCount ) );
DMOD_BUILTIN_API(Dmod, 1.0, int  , _Semaphore_Wait, ( void* Semaphore, uint32_t Count ) );
DMOD_BUILTIN_API(Dmod, 1.0, int  , _Semaphore_Post, ( void* Semaphore, uint32_t Count ) );
DMOD_BUILTIN_API(Dmod, 1.0, void , _Semaphore_Delete, ( void* Semaphore ) );

/**
 * @brief Suspend the calling thread for at least @p Milliseconds
 *
 * The primitive every polling loop in the system is expected to wait on -
 * notably Dmod_ReadKernel(), which has no interrupt to block on (the host
 * writes straight into the dmlog ring buffer over the debug probe) and so has
 * to re-check for input periodically.
 *
 * Must actually take the caller off the scheduler's ready list for the
 * requested time, not merely yield. A yield only hands the CPU to threads of
 * *equal* priority, so a poll loop in a higher-priority thread would still
 * starve every lower-priority one - which is precisely how a shell polling for
 * console input can stall the service manager running underneath it.
 *
 * Passing 0 is a plain yield: give up the rest of the current timeslice
 * without blocking.
 *
 * The default weak implementation busy-waits, which keeps bare-metal builds
 * (no scheduler to yield to) behaving exactly as before. Platform backends
 * override it - dmosi maps it onto dmosi_thread_sleep().
 *
 * @param Milliseconds Minimum time to sleep, in milliseconds; 0 yields.
 */
DMOD_BUILTIN_API(Dmod, 1.0, void , _ThreadSleep, ( uint32_t Milliseconds ) );

DMOD_BUILTIN_API(Dmod, 1.0, size_t, _GetLeftStackSize, ( void ) );

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

/*
 * Dmod_StrDup() has to attribute its allocation to whoever asked for the copy,
 * exactly the way Dmod_Malloc() above does - and it cannot work that out on its
 * own. The implementation lives in the DMOD system, where DMOD_MODULE_NAME is
 * not defined, so a plain Dmod_Malloc() inside it tags every duplicate with the
 * *system's* allocator instead of the caller's.
 *
 * That matters because allocation tracking keys bulk-freeing on unload
 * (Dmod_FreeModule) off exactly that tag - see Dmod_GetCurrentAllocatorName()
 * above. A string a long-lived module still holds can therefore be reclaimed
 * when some unrelated short-lived process exits, leaving a dangling pointer
 * with nothing in it to trace back to a strdup.
 *
 * Passing the name in at the call site is what fixes it. Plain _StrDup stays
 * declared for the system itself and for modules built against an older
 * header, and delegates with the running context's allocator.
 */
#if defined(DMOD_MODULE_NAME)
#   define Dmod_StrDup(Str)                         Dmod_StrDupEx(Str, DMOD_CURRENT_ALLOCATOR)
#else
    DMOD_BUILTIN_API(Dmod, 1.0, char*     , _StrDup,        ( const char* Str ) );
#endif
#define Dmod_StrMerge(Count, ...)                   Dmod_StrMergeEx(Count, DMOD_CURRENT_ALLOCATOR, ##__VA_ARGS__)

DMOD_BUILTIN_API(Dmod, 1.0, char*     , _StrDupEx,      ( const char* Str, const char* ModuleName ) );
DMOD_BUILTIN_API(Dmod, 1.0, char*     , _StrMergeEx,    ( int Count, const char* ModuleName, ... ) );

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
DMOD_BUILTIN_API(Dmod, 1.0, Dmod_Pid_t, _Spawn, ( Dmod_Context_t* Context, int argc, char *argv[], const Dmod_StreamRedirections_t* Streams ) );
DMOD_BUILTIN_API(Dmod, 1.0, Dmod_Pid_t, _RunDetached, ( Dmod_Context_t* Context, int argc, char *argv[], const Dmod_StreamRedirections_t* Streams ) );
DMOD_BUILTIN_API(Dmod, 1.0, int, _GetProcessResult, ( Dmod_Pid_t Pid ) );
DMOD_BUILTIN_API(Dmod, 1.0, Dmod_Pid_t, _GetCurrentPid, (void));
DMOD_BUILTIN_API(Dmod, 1.0, int, _SetForegroundModule, ( Dmod_Pid_t Pid, Dmod_Context_t* Context ) );
DMOD_BUILTIN_API(Dmod, 1.0, Dmod_Context_t*, _GetForegroundModule, ( Dmod_Pid_t Pid ) );

DMOD_BUILTIN_API(Dmod, 1.0, void*, _ResolveStreamFile, ( Dmod_Pid_t Pid, void* StdHandle ) );
DMOD_BUILTIN_API(Dmod, 1.0, int, _SetStreamFilePath, ( Dmod_Pid_t Pid, void* StdHandle, const char* Path ) );
DMOD_BUILTIN_API(Dmod, 1.0, int, _GetStreamRedirections, ( Dmod_Pid_t Pid, Dmod_StreamRedirection_t* OutEntries, size_t MaxEntries, size_t* OutCount ) );

DMOD_BUILTIN_API(Dmod, 1.0, void*, _LockStdio, ( void* StdHandle ));
DMOD_BUILTIN_API(Dmod, 1.0, void, _UnlockStdio, ( void* StdHandle ));

//! @}

/**
 * @defgroup DMOD_SAL_TIME Time Interface
 * @ingroup DMOD_SAL
 * 
 * This interface is used to get time information from the system.
 * The default implementation is provided in the DMOD system as weak symbols.
 * 
 * @addtogroup DMOD_SAL_TIME
 * @{
 */

DMOD_BUILTIN_API(Dmod, 1.0, Dmod_Timestamp_t, _GetUptime, ( void ) );

//! @}

/**
 * @defgroup DMOD_SAL_POWER System Power Interface
 * @ingroup DMOD_SAL
 * @{
 */

/**
 * @brief Immediately restart the system running the module.
 *
 * A successful restart does not return. On failure, returns a negative errno
 * value; the default weak implementation returns -ENOSYS and has no side effects.
 * Platforms override this function to provide their system reset mechanism.
 * This is an immediate reset: callers must finish any required shutdown or
 * persistence operations before calling it. No module or filesystem cleanup
 * is implied.
 *
 * @return Negative errno value if the system could not be restarted.
 */
DMOD_BUILTIN_API(Dmod, 1.0, int, _Reboot, ( void ) );

//! @}

/**
 * @defgroup DMOD_SAL_FIRMWARE Firmware Interface
 * @ingroup DMOD_SAL
 *
 * What the running firmware tells the platform, and what the platform keeps for it between the runs.
 *
 * A platform that has more than one firmware image (banks) starts a new one on trial: it stays only if it says that it
 * runs properly (Dmod_FirmwareReady()); if it does not, the platform goes back to the previous one after a reset. The
 * persistent data (Dmod_WritePersistency(), Dmod_ReadPersistency()) is kept apart from the images, so that an update of
 * the firmware - or a firmware that did not start - does not touch it.
 *
 * All of the functions are implemented by the platform (the boot loader of the board, for instance); the default
 * (weak) implementations of the host and of the platforms without them return -ENOSYS and have no side effects.
 * @{
 */

/**
 * @brief Tell the platform that the running firmware has started properly and may stay.
 *
 * What "properly" means is up to the firmware, so it is the firmware that calls this - when the console is there, the
 * file system is mounted, the network is up, or whatever its project needs to be able to be updated again. A platform
 * with a firmware on trial keeps it from then on; calling it again, or on a firmware that is not on trial, is harmless.
 * Until it is called a platform may restart the system (a watchdog) and fall back to the previous firmware.
 *
 * @return 0 on success, a negative errno value otherwise (-ENOSYS: the platform has nothing to confirm)
 */
DMOD_BUILTIN_API(Dmod, 1.0, int, _FirmwareReady, ( void ) );

/**
 * @brief Store data that survives a reset and an update of the firmware.
 *
 * The item @p Id is replaced with the @p Size bytes of @p Data. @p Data may be NULL only when @p Size is 0, which
 * removes the item. The platform decides the sizes, and the number of the items, that it can keep (-ENOSPC when it
 * cannot).
 *
 * Writing may erase and program a non-volatile memory: it can take long (hundreds of milliseconds for a sector of a
 * flash) and, where the code runs from the same memory, stop the whole system meanwhile. Do not do it often, nor from
 * a time critical thread; the data is for the settings, not for the logs.
 *
 * @param Id    Identifier of the item, chosen by the caller (the same one reads it back)
 * @param Data  The data
 * @param Size  Size of the data in bytes
 *
 * @return 0 on success, a negative errno value otherwise (-EINVAL: wrong arguments, -ENOSPC: no room,
 *         -EIO: the memory failed, -ENOSYS: the platform keeps nothing)
 */
DMOD_BUILTIN_API(Dmod, 1.0, int, _WritePersistency, ( uint32_t Id, const void* Data, size_t Size ) );

/**
 * @brief Read data stored with Dmod_WritePersistency().
 *
 * Copies at most @p BufferSize bytes of the item @p Id. The result is the size of the whole item, which may be more
 * than @p BufferSize (then the rest was not copied): call it with a NULL @p Buffer and 0 as @p BufferSize to learn the
 * size first.
 *
 * @param Id            Identifier of the item
 * @param Buffer        Where to copy the data; may be NULL when @p BufferSize is 0
 * @param BufferSize    Size of the buffer in bytes
 *
 * @return Size of the item in bytes (0 or more), or a negative errno value (-ENOENT: there is no such item,
 *         -EINVAL: wrong arguments, -EIO: the memory failed, -ENOSYS: the platform keeps nothing)
 */
DMOD_BUILTIN_API(Dmod, 1.0, int, _ReadPersistency, ( uint32_t Id, void* Buffer, size_t BufferSize ) );

/**
 * @brief State of a firmware image in a bank
 */
typedef enum
{
    Dmod_FirmwareBankState_Empty        = 0,    //!< There is no image in the bank
    Dmod_FirmwareBankState_Invalid      = 1,    //!< An image that is damaged, or that was started and never confirmed
    Dmod_FirmwareBankState_Valid        = 2,    //!< A good image that has not been selected to start (or has not run yet)
    Dmod_FirmwareBankState_Trial        = 3,    //!< Selected to start on trial: it stays only when it confirms (Dmod_FirmwareReady)
    Dmod_FirmwareBankState_Confirmed    = 4,    //!< An image that ran properly and confirmed it
} Dmod_FirmwareBankState_t;

/**
 * @brief What the platform knows about a bank (Dmod_FirmwareGetBankInfo)
 *
 * The caller sets @c StructSize to sizeof of the structure it was built with; the platform fills the fields that fit in
 * it and leaves the rest, so the structure can grow without a new function.
 */
typedef struct
{
    uint32_t    StructSize;     //!< [in] sizeof(Dmod_FirmwareBankInfo_t) of the caller
    uint32_t    State;          //!< Dmod_FirmwareBankState_t
    uint32_t    Version;        //!< Version of the image (a number of the platform's image format: the bigger the newer); 0 when empty
    uint32_t    Size;           //!< Size of the image in bytes; 0 when empty
    uint32_t    Tries;          //!< How many times it was started without confirming (a trial)
    uint32_t    Reserved;       //!< 0
    uint64_t    Address;        //!< Where the bank is in the memory of the platform (the flash address)
    uint64_t    Capacity;       //!< How many bytes of an image the bank holds
} Dmod_FirmwareBankInfo_t;

/** The bank that cannot be: "no bank" (Dmod_FirmwareGetRunningBank on a platform where the code does not run from one) */
#define DMOD_FIRMWARE_BANK_NONE     (-1)

/**
 * @brief Number of the firmware banks (images) that the platform keeps: 2 for the platform with banks A and B.
 *
 * The banks are numbered 0 .. count - 1 (0 is A). A platform without banks answers -ENOSYS; treat anything below 1 as
 * "no banks, nothing to update this way".
 *
 * @return Number of the banks, or a negative errno value
 */
DMOD_BUILTIN_API(Dmod, 1.0, int, _FirmwareGetBankCount, ( void ) );

/**
 * @brief The bank that the running firmware was started from.
 *
 * @return Number of the bank (0 ..), DMOD_FIRMWARE_BANK_NONE or a negative errno value (-ENOSYS: no banks)
 */
DMOD_BUILTIN_API(Dmod, 1.0, int, _FirmwareGetRunningBank, ( void ) );

/**
 * @brief Read what the platform knows about a bank: its state, version, size and where it is.
 *
 * The state is the one the platform keeps (a stored one, not a verification of the image: see Dmod_FirmwareVerifyBank()).
 * The address and the capacity tell a tool where an image goes - a block device of the platform (the flash driver) that
 * covers it is how it is written; the platform's own tool of the update is not needed for that.
 *
 * @param Bank  Number of the bank
 * @param Info  Receives the data; Info->StructSize has to be set
 *
 * @return 0 on success, a negative errno value otherwise (-EINVAL: no such bank or wrong arguments, -ENOSYS: no banks)
 */
DMOD_BUILTIN_API(Dmod, 1.0, int, _FirmwareGetBankInfo, ( int Bank, Dmod_FirmwareBankInfo_t* Info ) );

/**
 * @brief Check whether the image in a bank is whole: the platform reads it and verifies it the way it does before it starts it.
 *
 * Call it after writing an image and before Dmod_FirmwareSelectBank(): a damaged image would be refused by the platform
 * at the start anyway, but then only after the reset. The result is also the state that Dmod_FirmwareGetBankInfo()
 * reports (Empty, Invalid or Valid) from now on. It can take long (the platform reads the whole image).
 *
 * @param Bank  Number of the bank
 *
 * @return 0 when the image is good, -ENODATA when the bank is empty, -EBADMSG when the image is damaged (or is not an
 *         image of this platform), another negative errno value for other errors (-EINVAL: no such bank, -ENOSYS: no banks)
 */
DMOD_BUILTIN_API(Dmod, 1.0, int, _FirmwareVerifyBank, ( int Bank ) );

/**
 * @brief Choose the bank that the platform starts from after the next reset.
 *
 * The image has to pass Dmod_FirmwareVerifyBank(). With @p Trial the image stays selected only if it confirms that it runs
 * properly (Dmod_FirmwareReady()); if it does not - in the number of starts that the platform allows - the platform goes
 * back to the bank that ran before. Without @p Trial the bank is selected for good, with no confirmation expected (a way
 * back to a bank, the way to leave the platform at the image that is known to be good). Nothing restarts the system: call
 * Dmod_Reboot() when ready.
 *
 * @param Bank  Number of the bank
 * @param Trial true: on trial, until it confirms; false: for good
 *
 * @return 0 on success, a negative errno value otherwise (-EINVAL: no such bank, -EBADMSG: the image is not good,
 *         -ENODATA: the bank is empty, -EIO: the choice could not be stored, -ENOSYS: no banks)
 */
DMOD_BUILTIN_API(Dmod, 1.0, int, _FirmwareSelectBank, ( int Bank, bool Trial ) );

//! @}

#ifdef __cplusplus
}
#endif
#endif /* INC_DMOD_SAL_H_ */
