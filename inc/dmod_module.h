#ifndef DMOD_MODULE_H
#define DMOD_MODULE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dmod_defs.h"
#include "dmod_types.h"
#include "dmod_sal.h"

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
static inline Dmod_LogLevel_t Dmod_GetLogLevel(void)
{
    static volatile Dmod_Context_t* s_ctx = NULL;

    Dmod_EnterCritical();
    if( s_ctx == NULL )
    {
        s_ctx = Dmod_GetModuleContext(Dmod_GetCurrentModuleName());
    }
    Dmod_ExitCritical();

    return Dmod_GetModuleLogLevel(s_ctx);
}

#ifdef __cplusplus
}
#endif

#endif // DMOD_MODULE_H