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
 * @brief Process interface for DMOD SAL
 * @date 29 11 2025
 * 
 * The process interface is used to control process execution in the system.
 *
 * @file dmod_if_proc.c
 * @version 0.1
 */

#include "dmod_sal.h"
#include "dmod_system.h"
#include <inttypes.h>
#include <errno.h>
#if DMOD_USE_STDLIB
#   include <stdlib.h>
#endif
#if DMOD_USE_STDIO
#   include <stdio.h>
#endif
#if DMOD_USE_DIRENT
#   include <unistd.h>
#endif

//==============================================================================
//                              FUNCTIONS DECLARATIONS
//==============================================================================

/**
 * @brief Exit the process
 * 
 * Terminates the calling process immediately with the specified status code.
 * 
 * @param Status Exit status code (0 typically indicates success)
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void, _Exit, ( int Status ))
{
#if DMOD_USE_STDLIB
    exit(Status);
#else
    DMOD_LOG_ERROR("Dmod_Exit interface not implemented\n");
    (void)Status;
    /* In environments without stdlib, this becomes a no-op */
    while(1) { /* Infinite loop as fallback */ }
#endif
}

/**
 * @brief Spawn a module in a new child process
 *
 * This is a weak implementation that falls back to running the module in the current process.
 * The real implementation should be provided by the dmosi layer.
 *
 * @param Context Module context to spawn
 * @param argc Number of arguments
 * @param argv Argument array
 * @param Streams Stream redirections to apply, or NULL if none are requested. Ignored by this
 *                weak implementation - it runs the module in the current process, and there is
 *                no way to redirect its streams without affecting (and not restoring) the
 *                caller's own streams. A real dmosi implementation should honor it.
 * @return Process ID on success (weak implementation returns DMOD_CURRENT_PROCESS_PID as placeholder),
 *         negative error code on failure
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, Dmod_Pid_t, _Spawn, ( Dmod_Context_t* Context, int argc, char *argv[], const Dmod_StreamRedirections_t* Streams ))
{
    (void)Streams;
    int result = Dmod_Run(Context, argc, argv);
    if(result < 0)
    {
        return (Dmod_Pid_t)result;
    }
    // This weak implementation runs the module synchronously - no thread is spawned,
    // so nothing else will take ownership of unloading the context. The real
    // (dmosi-backed) implementation transfers that responsibility to the spawned
    // thread instead (its caller, Dmod_SpawnModule, only unloads on failure), so this
    // weak fallback must do it itself here now that Dmod_Run() has finished.
    Dmod_Unload(Context, false);
    return DMOD_CURRENT_PROCESS_PID;
}

/**
 * @brief Run a module in a detached process
 *
 * This is a weak implementation that falls back to running the module in the current process.
 * The real implementation should be provided by the dmosi layer.
 *
 * @param Context Module context to run detached
 * @param argc Number of arguments
 * @param argv Argument array
 * @param Streams Stream redirections to apply, or NULL if none are requested. Ignored by this
 *                weak implementation - it runs the module in the current process, and there is
 *                no way to redirect its streams without affecting (and not restoring) the
 *                caller's own streams. A real dmosi implementation should honor it.
 * @return Process ID on success (weak implementation returns DMOD_CURRENT_PROCESS_PID as placeholder),
 *         negative error code on failure
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, Dmod_Pid_t, _RunDetached, ( Dmod_Context_t* Context, int argc, char *argv[], const Dmod_StreamRedirections_t* Streams ))
{
    (void)Streams;
    int result = Dmod_Run(Context, argc, argv);
    if(result < 0)
    {
        return (Dmod_Pid_t)result;
    }
    // See the matching comment in Dmod_Spawn above.
    Dmod_Unload(Context, false);
    return DMOD_CURRENT_PROCESS_PID;
}

/**
 * @brief Get the result of a process
 * 
 * This is a weak implementation that returns 0 (success) for the current process placeholder.
 * The real implementation should be provided by the dmosi layer to wait for and
 * retrieve the exit status of a spawned process.
 * 
 * @param Pid Process ID to get result for
 * @return Exit status of the process, or negative error code
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _GetProcessResult, ( Dmod_Pid_t Pid ))
{
    if(Pid == DMOD_CURRENT_PROCESS_PID)
    {
        return 0;
    }
    DMOD_LOG_ERROR("Dmod_GetProcessResult interface not implemented for PID %" PRId32 "\n", Pid);
    return -1;
}

/**
 * @brief Get the PID of the calling process
 *
 * This is a weak implementation that returns the real PID via getpid() when available,
 * or DMOD_CURRENT_PROCESS_PID as a placeholder otherwise.
 * The real implementation should be provided by the dmosi layer.
 *
 * @return Process ID of the calling process
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, Dmod_Pid_t, _GetCurrentPid, ( void ))
{
    #if DMOD_USE_DIRENT
    return (Dmod_Pid_t)getpid();
    #else
    return DMOD_CURRENT_PROCESS_PID;
    #endif
}

/**
 * @brief Get the real file handle registered for one of a process's standard streams
 *
 * This is a weak implementation that always returns NULL.
 * The real implementation should be provided by the dmosi layer.
 *
 * @param Pid Process ID to get the file handle for
 * @param StdHandle One of DMOD_STDIN/DMOD_STDOUT/DMOD_STDERR/DMOD_STDLOG
 * @return NULL
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void*, _ResolveStreamFile, ( Dmod_Pid_t Pid, void* StdHandle ))
{
    if(StdHandle == DMOD_STDIN)
    {
        #if DMOD_USE_STDIO
        return stdin;
        #else
        return NULL;
        #endif
    }
    else if(StdHandle == DMOD_STDOUT)
    {
        #if DMOD_USE_STDIO
        return stdout;
        #else
        return NULL;
        #endif
    }
    else if(StdHandle == DMOD_STDERR)
    {
        #if DMOD_USE_STDIO
        return stderr;
        #else
        return NULL;
        #endif
    }
    else if(StdHandle == DMOD_STDLOG)
    {
        #if DMOD_USE_STDIO
        return stdout;
        #else
        return NULL;
        #endif
    }
    return StdHandle;
}

/**
 * @brief Register the path of the file backing one of a process's standard streams
 *
 * This is a weak implementation that does nothing. Path == NULL is meant to clear an
 * existing binding, but since this weak implementation never binds anything in the first
 * place, there is nothing to clear either.
 * The real implementation should be provided by the dmosi layer.
 *
 * @param Pid Process ID to set the stream file path for
 * @param StdHandle One of DMOD_STDIN/DMOD_STDOUT/DMOD_STDERR/DMOD_STDLOG
 * @param Path Path of the file to associate with this (Pid, StdHandle) pair, or NULL to clear it
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _SetStreamFilePath, ( Dmod_Pid_t Pid, void* StdHandle, const char* Path ))
{
    (void)Pid;
    (void)StdHandle;
    (void)Path;
    return -ENOSYS;
}

/**
 * @brief Snapshot the currently explicitly-bound standard streams of a process
 *
 * This is a weak implementation that always reports an empty snapshot (nothing bound),
 * since this weak implementation never binds anything via Dmod_SetStreamFilePath either.
 * The real implementation should be provided by the dmosi layer.
 *
 * @param Pid Process ID to snapshot
 * @param OutEntries Buffer to receive the snapshot entries
 * @param MaxEntries Capacity of OutEntries, in entries
 * @param OutCount Receives the number of entries written to OutEntries (always 0 here)
 * @return 0
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _GetStreamRedirections, ( Dmod_Pid_t Pid, Dmod_StreamRedirection_t* OutEntries, size_t MaxEntries, size_t* OutCount ))
{
    (void)Pid;
    (void)OutEntries;
    (void)MaxEntries;
    if( OutCount != NULL )
    {
        *OutCount = 0;
    }
    return 0;
}

/**
 * @brief Locks the stdio buffer (for current pid) to protect against recursive calls
 * 
 * @param File File handle or one of DMOD_STDIN/DMOD_STDOUT/DMOD_STDERR/DMOD_STDLOG
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void*, _LockStdio, ( void* File ))
{
    return Dmod_ResolveStreamFile( Dmod_GetCurrentPid(), File );
}

/**
 * @brief unlocks the stdio buffer 
 * 
 * @param File File handle or one of DMOD_STDIN/DMOD_STDOUT/DMOD_STDERR/DMOD_STDLOG
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void, _UnlockStdio, ( void* File ))
{

}