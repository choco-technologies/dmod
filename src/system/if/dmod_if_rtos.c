/**
 * MIT License
 * 
 * Copyright (c) 2024 Patryk Kubiak
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
 * @brief File interface for DMOD SAL RTOS
 * @date 21 12 2024 
 * 
 */

#include <errno.h>
#include "dmod.h"
#if DMOD_USE_PTHREAD
#   define __USE_UNIX98
#   include <pthread.h>
#endif
#if defined(__unix__) || defined(__APPLE__)
#   include <unistd.h>
#   include <time.h>
#endif

//==============================================================================
//                              FUNCTIONS DECLARATIONS
//==============================================================================

/**
 * @brief Create new mutex
 * 
 * @param Recursive Recursive mutex
 * 
 * @return Pointer to new mutex
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void*, _Mutex_New, ( bool Recursive ))
{
    #if DMOD_USE_PTHREAD
    pthread_mutex_t* Mutex = Dmod_Malloc( sizeof( pthread_mutex_t ) );
    if( Mutex == NULL )
    {
        DMOD_LOG_ERROR("Cannot create new mutex - cannot allocate memory\n");
        return NULL;
    }

    pthread_mutexattr_t Attr;
    if( pthread_mutexattr_init( &Attr ) != 0 )
    {
        DMOD_LOG_ERROR("Cannot create new mutex - cannot initialize mutex attribute\n");
        Dmod_Free( Mutex );
        return NULL;
    }

    if( Recursive )
    {
        if( pthread_mutexattr_settype( &Attr, PTHREAD_MUTEX_RECURSIVE_NP ) != 0 )
        {
            DMOD_LOG_ERROR("Cannot create new mutex - cannot set mutex attribute\n");
            Dmod_Free( Mutex );
            return NULL;
        }
    }

    if( pthread_mutex_init( Mutex, &Attr ) != 0 )
    {
        DMOD_LOG_ERROR("Cannot create new mutex - cannot initialize mutex\n");
        Dmod_Free( Mutex );
        return NULL;
    }

    return Mutex;
    #else
    DMOD_LOG_WARN("Dmod_Mutex_New interface not implemented\n");
    return NULL;
    #endif
}

/**
 * @brief Lock mutex
 * 
 * @param Mutex Mutex to lock
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _Mutex_Lock, ( void* Mutex ))
{
    #if DMOD_USE_PTHREAD
    if( Mutex == NULL )
    {
        DMOD_LOG_ERROR("Cannot lock mutex - invalid mutex\n");
        return -EINVAL;
    }

    return pthread_mutex_lock( Mutex );
    #else
    if( Mutex == NULL )
    {
        return 0; // No-op if mutex is NULL
    }
    else 
    {
        DMOD_LOG_WARN("Dmod_Mutex_Lock interface not implemented\n");
        return -ENOSYS;
    }
    #endif
}

/**
 * @brief Unlock mutex
 * 
 * @param Mutex Mutex to unlock
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _Mutex_Unlock, ( void* Mutex ))
{
    #if DMOD_USE_PTHREAD
    if( Mutex == NULL )
    {
        DMOD_LOG_ERROR("Cannot unlock mutex - invalid mutex\n");
        return -EINVAL;
    }

    return pthread_mutex_unlock( Mutex );
    #else
    if(Mutex == NULL )
    {
        return 0; // No-op if mutex is NULL
    }
    else 
    {
        DMOD_LOG_WARN("Dmod_Mutex_Unlock interface not implemented\n");
        return -ENOSYS;
    }
    #endif
}

/**
 * @brief Delete mutex
 * 
 * @param Mutex Mutex to delete
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void, _Mutex_Delete, ( void* Mutex ))
{
    #if DMOD_USE_PTHREAD
    if( Mutex == NULL )
    {
        DMOD_LOG_ERROR("Cannot delete mutex - invalid mutex\n");
        return;
    }

    if( pthread_mutex_destroy( Mutex ) != 0 )
    {
        DMOD_LOG_ERROR("Cannot delete mutex - cannot destroy mutex\n");
    }

    Dmod_Free( Mutex );
    #else
    if(Mutex == NULL )
    {
        return; // No-op if mutex is NULL
    }
    else
    {
        DMOD_LOG_WARN("Dmod_Mutex_Delete interface not implemented\n");
    }
    #endif
}

/**
 * @brief Delay execution for a specified number of microseconds
 * 
 * @param Microseconds Number of microseconds to delay
 * 
 * @return true if delay was successful, false otherwise
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, bool, _DelayUs, ( uint64_t Microseconds ))
{
    #if defined(__unix__) || defined(__APPLE__)
    if (Microseconds == 0)
    {
        return true;
    }
    
    // For delays less than 1 second, use usleep (deprecated but widely available)
    // For longer delays, use nanosleep for better precision
    if (Microseconds < 1000000)
    {
        #ifdef _POSIX_C_SOURCE
        // Use nanosleep for better precision
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = Microseconds * 1000;
        
        while (nanosleep(&ts, &ts) == -1)
        {
            // Continue if interrupted by signal
            if (errno != EINTR)
            {
                return false;
            }
        }
        #else
        // Fallback to usleep
        if (usleep(Microseconds) != 0)
        {
            return false;
        }
        #endif
    }
    else
    {
        // Split into seconds and remaining microseconds
        struct timespec ts;
        ts.tv_sec = Microseconds / 1000000;
        ts.tv_nsec = (Microseconds % 1000000) * 1000;
        
        while (nanosleep(&ts, &ts) == -1)
        {
            if (errno != EINTR)
            {
                return false;
            }
        }
    }
    
    return true;
    #else
    // Platform not supported
    (void)Microseconds;
    DMOD_LOG_WARN("Dmod_DelayUs interface not implemented for this platform\n");
    return false;
    #endif
}

/**
 * @brief Sleep for a specified number of milliseconds
 * 
 * @param Milliseconds Number of milliseconds to sleep
 * 
 * @return true if sleep was successful, false otherwise
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, bool, _SleepMs, ( uint64_t Milliseconds ))
{
    #if DMOD_USE_PTHREAD
    if (Milliseconds == 0)
    {
        return true;
    }
    
    // Use nanosleep for better precision
    struct timespec ts;
    ts.tv_sec = Milliseconds / 1000;
    ts.tv_nsec = (Milliseconds % 1000) * 1000000;
    
    while (nanosleep(&ts, &ts) == -1)
    {
        // Continue if interrupted by signal
        if (errno != EINTR)
        {
            return false;
        }
    }
    
    return true;
    #elif defined(__unix__) || defined(__APPLE__)
    if (Milliseconds == 0)
    {
        return true;
    }
    
    // Convert milliseconds to microseconds
    if (usleep(Milliseconds * 1000) != 0)
    {
        return false;
    }
    
    return true;
    #else
    // Platform not supported
    (void)Milliseconds;
    DMOD_LOG_WARN("Dmod_SleepMs interface not implemented for this platform\n");
    return false;
    #endif
}
