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
#define Dmod_Free(Ptr)                           Dmod_FreeEx(Ptr, false)
DMOD_BUILTIN_API(Dmod, 1.0, void*,  _MallocEx,          ( size_t Size, const char* ModuleName ) );
DMOD_BUILTIN_API(Dmod, 1.0, void*,  _ReallocEx,         ( void* Ptr, size_t Size, const char* ModuleName ) );
DMOD_BUILTIN_API(Dmod, 1.0, void ,  _FreeEx ,           ( void* Ptr, bool Concatenate ) );
DMOD_BUILTIN_API(Dmod, 1.0, void*,  _AlignedMallocEx,   ( size_t Size, size_t Alignment, const char* ModuleName ) );
DMOD_BUILTIN_API(Dmod, 1.0, void,   _FreeModule,        ( const char* ModuleName )          );
DMOD_BUILTIN_API(Dmod, 1.0, bool,   _RetagEx,           ( void* Ptr, const char* ModuleName ) );
DMOD_BUILTIN_API(Dmod, 1.0, bool,   _RenameTag,         ( const char* OldTag, const char* NewTag ) );
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
DMOD_BUILTIN_API(Dmod, 1.0, char*     , _StrDupEx,      ( const char* Str, const char* ModuleName ) );

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

#ifdef __cplusplus
}
#endif
#endif /* INC_DMOD_SAL_H_ */
