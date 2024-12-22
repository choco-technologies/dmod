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

#include "dmod_types.h"

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
extern void*    Dmod_Malloc( size_t Size );
extern void     Dmod_Free( void* Ptr );
extern void*    Dmod_AlignedMalloc( size_t Size, size_t Alignment );
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
extern void*    Dmod_FileOpen( const char* Path, const char* Mode );
extern size_t   Dmod_FileRead( void* Buffer, size_t Size, size_t Count, void* File );
extern int      Dmod_FileSeek( void* File, long Offset, int Origin );
extern size_t   Dmod_FileSize( void* File );
extern void     Dmod_FileClose( void* File );

#ifndef DMOD_SEEK_SET
#   define DMOD_SEEK_SET   0
#   define DMOD_SEEK_CUR   1
#   define DMOD_SEEK_END   2
#endif
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
extern int     Dmod_Printf( const char* Format, ... );
extern void    Dmod_Assert( int Condition, const char* Message, const char* File, int Line, const char* Function );

#ifdef NDEBUG
#   define DMOD_ASSERT_MSG( Condition, Message )           ((void)0)
#else 
#   define DMOD_ASSERT_MSG( Condition, Message )           Dmod_Assert( Condition, Message, __FILE__, __LINE__, __func__ )
#endif

#define DMOD_ASSERT( Condition )        DMOD_ASSERT_MSG( Condition, #Condition )

#ifndef DMOD_LOG_VERBOSE
#   define DMOD_LOG_VERBOSE(...)    Dmod_Printf( "[VERBOSE] " __VA_ARGS__ )
#endif

#ifndef DMOD_LOG_INFO
#   define DMOD_LOG_INFO(...)      Dmod_Printf( "[INFO] " __VA_ARGS__ )
#endif

#ifndef DMOD_LOG_WARN
#   define DMOD_LOG_WARN(...)      Dmod_Printf( "[WARN] " __VA_ARGS__ )
#endif

#ifndef DMOD_LOG_ERROR
#   define DMOD_LOG_ERROR(...)     Dmod_Printf( "[ERROR] " __VA_ARGS__ )
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

extern void Dmod_EnterCritical( void );
extern void Dmod_ExitCritical( void );

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

extern void Dmod_Event_ModuleLoaded( Dmod_Context_t* Context );
extern void Dmod_Event_ModuleUnloaded( Dmod_Context_t* Context );
extern void Dmod_Event_ModuleLoadingInProgress( const char* Name, uint16_t Progress );
extern void Dmod_Event_ModuleEnabled( Dmod_Context_t* Context );
extern void Dmod_Event_ModuleDisabled( Dmod_Context_t* Context );
extern void Dmod_Event_ModuleRunning( Dmod_Context_t* Context );
extern void Dmod_Event_ModuleStopped( Dmod_Context_t* Context );

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

extern void* Dmod_Mutex_New( void );
extern int   Dmod_Mutex_Lock( void* Mutex );
extern int   Dmod_Mutex_Unlock( void* Mutex );
extern void  Dmod_Mutex_Delete( void* Mutex );

//! @}


#endif /* INC_DMOD_SAL_H_ */
