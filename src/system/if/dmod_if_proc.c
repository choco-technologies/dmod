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
#if DMOD_USE_STDLIB
#   include <stdlib.h>
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
 * @return Process ID on success, 0 or negative error code on failure
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, Dmod_Pid_t, _Spawn, ( Dmod_Context_t* Context, int argc, char *argv[] ))
{
    int result = Dmod_Run(Context, argc, argv);
    if(result < 0)
    {
        return (Dmod_Pid_t)result;
    }
    return 1;
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
 * @return Process ID on success, 0 or negative error code on failure
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, Dmod_Pid_t, _RunDetached, ( Dmod_Context_t* Context, int argc, char *argv[] ))
{
    int result = Dmod_Run(Context, argc, argv);
    if(result < 0)
    {
        return (Dmod_Pid_t)result;
    }
    return 1;
}

/**
 * @brief Get the result of a process
 * 
 * This is a weak implementation that returns 0 (success) for the current process (PID 1).
 * The real implementation should be provided by the dmosi layer to wait for and
 * retrieve the exit status of a spawned process.
 * 
 * @param Pid Process ID to get result for
 * @return Exit status of the process, or negative error code
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _GetProcessResult, ( Dmod_Pid_t Pid ))
{
    if(Pid == 1)
    {
        return 0;
    }
    DMOD_LOG_ERROR("Dmod_GetProcessResult interface not implemented for PID %d\n", Pid);
    return -1;
}
