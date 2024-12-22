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
 * @brief DMOD  Dynamic Module Loader
 * @date 2024-12-20
 * @author Patryk Kubiak <patryk.kubiak90@gmail.com>
 * 
 * @file dmod.h
 * 
 * @defgroup DMOD DMOD Dynamic Module Loader
 * 
 * @version 0.1
 * 
 * The DMOD is a dynamic module loader that allows to load and unload modules
 * at runtime. The DMOD is designed to be used in embedded systems where the
 * memory is limited and the system must be optimized for speed and size.
 * 
 * The DMOD provides a simple API to load and unload modules. The modules are
 * loaded from a file and can be executed by calling the main function of the
 * module. The modules can be written in C or C++ and can be compiled with any
 * compiler that supports the target architecture.
 * 
 * The DMOD is designed to be used with the DMOD system abstraction layer (SAL).
 * The SAL provides an abstraction layer for the system. This allows the DMOD to
 * be used on different systems without changing the core code. The default
 * implementation is provided in the DMOD system as weak symbols.
 * 
 * The DMOD module can be also used in the high level operating systems 
 * like Linux, Windows or MacOS, where the default library functions are used.
 * This can be useful for testing and debugging the modules before deploying
 * them to the embedded system.
 * 
 */
#ifndef INC_DMOD_H_
#define INC_DMOD_H_

#include <stdbool.h>
#include "dmod_types.h"
#include "dmod_sal.h"

#ifdef DMOD_MODULE
#   include "dmod_module.h"
#elif defined(DMOD_SYSTEM)
#   include "dmod_system.h"
#else 
#   error "DMOD_MODULE or DMOD_SYSTEM must be defined"
#endif

//==============================================================================
//                              FUNCTION PROTOTYPES
//==============================================================================

/**
 * @addtogroup DMOD
 * @{
 */

extern size_t       Dmod_Api_GetNumberOfEntries( Dmod_Api_t* Api );
extern bool         Dmod_ApiSignature_IsValid( const char* Signature );
extern bool         Dmod_ApiSignature_IsModule( const char* Signature, const char* ModuleName );
extern const char*  Dmod_ApiSignature_GetName( const char *Signature );
extern const char*  Dmod_ApiSignature_GetVersion( const char* Signature );
extern const char*  Dmod_ApiSignature_GetModule( const char* Signature );
extern bool         Dmod_ApiSignature_ReadModuleName( const char* Signature, char* ModuleName, size_t MaxLength );
extern bool         Dmod_ApiSignature_ReadVersion( const char* Signature, char* Version, size_t MaxLength );
extern bool         Dmod_ApiSignature_AreEqual( const char* Signature1, const char* Signature2 );

//! @}

#endif /* INC_DMOD_H_ */
