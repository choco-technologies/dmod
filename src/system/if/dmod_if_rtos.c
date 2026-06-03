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
#   include <semaphore.h>
#   ifdef __linux__
/* Forward declarations for Linux-specific thread attribute functions */
extern int pthread_getattr_np(pthread_t th, pthread_attr_t *attr);
extern int pthread_attr_getstack(const pthread_attr_t *attr, void **stackaddr, size_t *stacksize);
#   endif
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
 * @brief Create new semaphore
 * 
 * @param InitialValue Initial semaphore value
 * 
 * @return Pointer to new semaphore
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void*, _Semaphore_New, ( uint32_t InitialValue ))
{
    #if DMOD_USE_PTHREAD
    sem_t* Semaphore = Dmod_Malloc(sizeof(sem_t));
    if( Semaphore == NULL )
    {
        DMOD_LOG_ERROR("Cannot create new semaphore - cannot allocate memory\n");
        return NULL;
    }

    if( sem_init(Semaphore, 0, InitialValue) != 0 )
    {
        DMOD_LOG_ERROR("Cannot create new semaphore - cannot initialize semaphore\n");
        Dmod_Free(Semaphore);
        return NULL;
    }

    return Semaphore;
    #else
    (void)InitialValue;
    return NULL;
    #endif
}

/**
 * @brief Wait for semaphore
 * 
 * @param Semaphore Semaphore to wait for
 * 
 * @return 0 on success, negative errno on error
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _Semaphore_Wait, ( void* Semaphore ))
{
    #if DMOD_USE_PTHREAD
    if( Semaphore == NULL )
    {
        DMOD_LOG_ERROR("Cannot wait for semaphore - invalid semaphore\n");
        return -EINVAL;
    }

    return sem_wait(Semaphore);
    #else
    if( Semaphore == NULL )
    {
        return 0; // No-op if semaphore is NULL
    }
    else
    {
        DMOD_LOG_WARN("Dmod_Semaphore_Wait interface not implemented\n");
        return -ENOSYS;
    }
    #endif
}

/**
 * @brief Post semaphore
 * 
 * @param Semaphore Semaphore to post
 * 
 * @return 0 on success, negative errno on error
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _Semaphore_Post, ( void* Semaphore ))
{
    #if DMOD_USE_PTHREAD
    if( Semaphore == NULL )
    {
        DMOD_LOG_ERROR("Cannot post semaphore - invalid semaphore\n");
        return -EINVAL;
    }

    return sem_post(Semaphore);
    #else
    if( Semaphore == NULL )
    {
        return 0; // No-op if semaphore is NULL
    }
    else
    {
        DMOD_LOG_WARN("Dmod_Semaphore_Post interface not implemented\n");
        return -ENOSYS;
    }
    #endif
}

/**
 * @brief Delete semaphore
 * 
 * @param Semaphore Semaphore to delete
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void, _Semaphore_Delete, ( void* Semaphore ))
{
    #if DMOD_USE_PTHREAD
    if( Semaphore == NULL )
    {
        DMOD_LOG_ERROR("Cannot delete semaphore - invalid semaphore\n");
        return;
    }

    if( sem_destroy(Semaphore) != 0 )
    {
        DMOD_LOG_ERROR("Cannot delete semaphore - cannot destroy semaphore\n");
    }

    Dmod_Free(Semaphore);
    #else
    if( Semaphore == NULL )
    {
        return; // No-op if semaphore is NULL
    }
    else
    {
        DMOD_LOG_WARN("Dmod_Semaphore_Delete interface not implemented\n");
    }
    #endif
}

/**
 * @brief Get the remaining stack size in the current thread
 * 
 * Returns the number of bytes still available on the current thread's stack.
 * On Linux with pthread support, this is calculated from the thread's stack
 * bounds and the current stack pointer. On other platforms, SIZE_MAX is
 * returned to indicate that the size is unknown (skip the check).
 * 
 * @return Remaining stack size in bytes, or SIZE_MAX if not available
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, size_t, _GetLeftStackSize, ( void ))
{
#if DMOD_USE_PTHREAD && defined(__linux__)
    pthread_attr_t attr;
    void* stackAddr;
    size_t stackSize;
    volatile char stackVar;

    if( pthread_getattr_np(pthread_self(), &attr) != 0 )
    {
        return (size_t)-1;
    }

    if( pthread_attr_getstack(&attr, &stackAddr, &stackSize) != 0 )
    {
        pthread_attr_destroy(&attr);
        return (size_t)-1;
    }

    pthread_attr_destroy(&attr);

    /* The stack grows downward: stackAddr is the lowest address of the stack
     * region and (stackAddr + stackSize) is the highest (initial SP position).
     * As functions are called the SP moves toward stackAddr.
     * Remaining space before overflow = currentSp - stackBase. */
    uintptr_t currentSp = (uintptr_t)&stackVar;
    uintptr_t stackBase = (uintptr_t)stackAddr;

    if( currentSp <= stackBase )
    {
        return 0;
    }

    return (size_t)(currentSp - stackBase);
#else
    return (size_t)-1;
#endif
}
