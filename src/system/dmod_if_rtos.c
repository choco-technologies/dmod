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
#   include <pthread.h>
#endif

//==============================================================================
//                              FUNCTIONS DECLARATIONS
//==============================================================================

/**
 * @brief Create new mutex
 * 
 * @return Pointer to new mutex
 */
void* DMOD_WEAK_SYMBOL Dmod_Mutex_New( void )
{
    #if DMOD_USE_PTHREAD
    pthread_mutex_t* Mutex = Dmod_Malloc( sizeof( pthread_mutex_t ) );
    if( Mutex == NULL )
    {
        DMOD_LOG_ERROR("Cannot create new mutex - cannot allocate memory\n");
        return NULL;
    }

    if( pthread_mutex_init( Mutex, NULL ) != 0 )
    {
        DMOD_LOG_ERROR("Cannot create new mutex - cannot initialize mutex\n");
        Dmod_Free( Mutex );
        return NULL;
    }

    return Mutex;
    #else
    DMOD_LOG_WARN("Dmod_Mutex_New interface not implemented");
    return NULL;
    #endif
}

/**
 * @brief Lock mutex
 * 
 * @param Mutex Mutex to lock
 */
int DMOD_WEAK_SYMBOL Dmod_Mutex_Lock( void* Mutex )
{
    #if DMOD_USE_PTHREAD
    if( Mutex == NULL )
    {
        DMOD_LOG_ERROR("Cannot lock mutex - invalid mutex\n");
        return -EINVAL;
    }

    return pthread_mutex_lock( Mutex );
    #else
    DMOD_LOG_WARN("Dmod_Mutex_Lock interface not implemented");
    return -ENOSYS;
    #endif
}

/**
 * @brief Unlock mutex
 * 
 * @param Mutex Mutex to unlock
 */
int DMOD_WEAK_SYMBOL Dmod_Mutex_Unlock( void* Mutex )
{
    #if DMOD_USE_PTHREAD
    if( Mutex == NULL )
    {
        DMOD_LOG_ERROR("Cannot unlock mutex - invalid mutex\n");
        return -EINVAL;
    }

    return pthread_mutex_unlock( Mutex );
    #else
    DMOD_LOG_WARN("Dmod_Mutex_Unlock interface not implemented");
    return -ENOSYS;
    #endif
}

/**
 * @brief Delete mutex
 * 
 * @param Mutex Mutex to delete
 */
void DMOD_WEAK_SYMBOL Dmod_Mutex_Delete( void* Mutex )
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
    DMOD_LOG_WARN("Dmod_Mutex_Delete interface not implemented");
    #endif
}
