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
 * @brief DMOD-System   Dynamic Module Loader System
 * @date 2024-12-20
 * @author Patryk Kubiak <patryk.kubiak90@gmail.com>
 * 
 * @file dmod_system.h
 * 
 * @defgroup DMOD-System DMOD System
 * @ingroup DMOD
 * @brief System side of the DMOD
 * 
 * This file contains the system side of the DMOD. The system side is responsible
 * for loading and unloading the modules. This API is not available to the modules.
 * 
 * @version 0.1
 */
#ifndef DMOD_SYSTEM_H
#define DMOD_SYSTEM_H

#include "dmod_types.h"


//==============================================================================
//                              FUNCTION PROTOTYPES
//==============================================================================

/**
 * @addtogroup DMOD-System
 * @{
 */
extern Dmod_Context_t*  Dmod_LoadFile           ( const char* Path );
extern Dmod_Context_t*  Dmod_Load               ( const void* Data, size_t Size );
extern void             Dmod_Unload             ( Dmod_Context_t* Context );
extern bool             Dmod_ConnectApi         ( Dmod_Api_t* OutputsApi, Dmod_Api_t* InputsApi );
extern bool             Dmod_DisconnectApi      ( Dmod_Api_t* OutputsApi, Dmod_Api_t* InputsApi );
extern bool             Dmod_ConnectOutputApis  ( Dmod_Context_t* Context );
extern bool             Dmod_ConnectInputApis   ( Dmod_Context_t* Context );
extern bool             Dmod_ConnectAllApis     ( Dmod_Context_t* Context );
extern bool             Dmod_DisconnectOutputApis( Dmod_Context_t* Context );
extern bool             Dmod_DisconnectInputApis ( Dmod_Context_t* Context );
extern bool             Dmod_DisconnectAllApis  ( Dmod_Context_t* Context );
extern void*            Dmod_GetFunction        ( Dmod_Context_t* Context, const char* Signature );
extern int              Dmod_Init               ( Dmod_Context_t* Context, Dmod_Config_t* Config );
extern int              Dmod_Main               ( Dmod_Context_t* Context, int argc, char *argv[] );
extern int              Dmod_Deinit             ( Dmod_Context_t* Context );
extern int              Dmod_Signal             ( Dmod_Context_t* Context, int SignalNumber );
extern int              Dmod_Irq                ( Dmod_Context_t* Context, const char* Signature );
extern uint64_t         Dmod_GetStackSize       ( Dmod_Context_t* Context );
extern Dmod_ModuleType_t Dmod_GetModuleType     ( Dmod_Context_t* Context );
extern bool             Dmod_Enable             ( Dmod_Context_t* Context );
extern bool             Dmod_Disable            ( Dmod_Context_t* Context );
extern bool             Dmod_IsEnabled          ( Dmod_Context_t* Context );
extern int              Dmod_Run                ( Dmod_Context_t* Context, int argc, char *argv[] );
extern bool             Dmod_IsRunning          ( Dmod_Context_t* Context );
//! @}

#endif // DMOD_SYSTEM_H
