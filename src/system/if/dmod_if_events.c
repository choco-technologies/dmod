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
 * @brief File interface for DMOD SAL Events
 * @date 21 12 2024 
 * 
 */
#include "dmod.h"

//==============================================================================
//                              FUNCTIONS DECLARATIONS
//==============================================================================

/**
 * @brief Event when module is loaded
 * 
 * @param Context Context of the module
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void, _Event_ModuleLoaded, ( Dmod_Context_t* Context ))
{

}

/**
 * @brief Event when module is unloaded
 * 
 * @param Context Context of the module
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void, _Event_ModuleUnloaded, ( Dmod_Context_t* Context ))
{

}

/**
 * @brief Event when module is loading in progress
 * 
 * @param Name Name of the module
 * @param Progress Progress of the loading
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void, _Event_ModuleLoadingInProgress, ( const char* Name, uint16_t Progress ))
{
    DMOD_LOG_INFO("Loading module %s in progress: %d\n", Name, Progress);
}

/**
 * @brief Event when module is enabled
 * 
 * @param Context Context of the module
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void, _Event_ModuleEnabled, ( Dmod_Context_t* Context ))
{

}

/**
 * @brief Event when module is disabled
 * 
 * @param Context Context of the module
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void, _Event_ModuleDisabled, ( Dmod_Context_t* Context ))
{

}

/**
 * @brief Event when module is running
 * 
 * @param Context Context of the module
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void, _Event_ModuleRunning, ( Dmod_Context_t* Context ))
{

}

/**
 * @brief Event when module is stopped
 * 
 * @param Context Context of the module
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, void, _Event_ModuleStopped, ( Dmod_Context_t* Context ))
{

}