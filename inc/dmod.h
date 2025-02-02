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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "config.h"
#include "dmod_types.h"
#include "dmod_sal.h"

#if DMOD_MODULE_EN == ON
#   include "dmod_module.h"
#elif DMOD_SYSTEM_EN == ON
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
extern bool         Dmod_ApiSignature_IsModuleNameGiven( const char* Signature );
extern bool         Dmod_ApiSignature_IsModule( const char* Signature, const char* ModuleName );
extern const char*  Dmod_ApiSignature_GetName( const char *Signature );
extern const char*  Dmod_ApiSignature_GetVersion( const char* Signature );
extern const char*  Dmod_ApiSignature_GetModule( const char* Signature );
extern bool         Dmod_ApiSignature_ReadModuleName( const char* Signature, char* ModuleName, size_t MaxLength );
extern bool         Dmod_ApiSignature_ReadVersion( const char* Signature, char* Version, size_t MaxLength );
extern bool         Dmod_ApiSignature_AreEqual( const char* Signature1, const char* Signature2 );
extern bool         Dmod_ApiSignature_IsMal( const char* Signature );

DMOD_BUILTIN_API( Dmod, 1.0, bool       , _ReadModuleHeader, (const char* FilePath, Dmod_ModuleHeader_t* Header) );
DMOD_BUILTIN_API( Dmod, 1.0, void       , _BeginUsage, (const char* ModuleName) );
DMOD_BUILTIN_API( Dmod, 1.0, void       , _EndUsage, (const char* ModuleName) );
DMOD_BUILTIN_API( Dmod, 1.0, bool       , _IsApplicationModuleFile, (const char* FilePath) );
DMOD_BUILTIN_API( Dmod, 1.0, bool       , _IsModuleUsed, (const char* ModuleName) );
DMOD_BUILTIN_API( Dmod, 1.0, bool       , _IsModuleLoaded, (const char* ModuleName) );
DMOD_BUILTIN_API( Dmod, 1.0, bool       , _IsModuleFileLoaded, (const char* FilePath) );
DMOD_BUILTIN_API( Dmod, 1.0, bool       , _IsModuleEnabled, (const char* ModuleName) );
DMOD_BUILTIN_API( Dmod, 1.0, bool       , _IsModuleRequired, (const char* ModuleName, const char* RequiredModuleName) );
DMOD_BUILTIN_API( Dmod, 1.0, const char*, _GetModuleVersion, (const char* ModuleName) );
DMOD_BUILTIN_API( Dmod, 1.0, bool       , _LoadModule, (const char* FilePath) );
DMOD_BUILTIN_API( Dmod, 1.0, bool       , _LoadModuleByName, (const char* ModuleName) );
DMOD_BUILTIN_API( Dmod, 1.0, bool       , _UnloadModule, (const char* ModuleName, bool Force) );
DMOD_BUILTIN_API( Dmod, 1.0, bool       , _EnableModule, (const char* ModuleName, bool Force, const Dmod_Config_t* Config) );
DMOD_BUILTIN_API( Dmod, 1.0, bool       , _DisableModule, (const char* ModuleName, bool Force ) );
DMOD_BUILTIN_API( Dmod, 1.0, int        , _RunModule, (const char* ModuleName, int argc, char *argv[]) );

//! @}

#ifdef __cplusplus
}
#endif

#endif /* INC_DMOD_H_ */
