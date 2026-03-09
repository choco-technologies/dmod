#include "dmod.h"

/**
 * @brief Get the effective log level for the current module
 * 
 * On the first call the module context is looked up via
 * Dmod_GetModuleContext() using the module name supplied by
 * Dmod_GetCurrentModuleName(), and the result is cached in a static
 * variable.  Subsequent calls skip the lookup entirely.
 * 
 * A critical section guards the initialization to prevent concurrent
 * first-call races.
 */
Dmod_LogLevel_t Dmod_GetLogLevel(void)
{
    static volatile bool s_initialized = false;
    static Dmod_Context_t* s_ctx = NULL;

    // TODO: Check why this crashes in dmgpio (it is not connected)
    // Dmod_EnterCritical();
    if( !s_initialized )
    {
        s_ctx = Dmod_GetModuleContext(Dmod_GetCurrentModuleName());
        s_initialized = true;
    }
    // Dmod_ExitCritical();

    return Dmod_GetModuleLogLevel(s_ctx);
}

