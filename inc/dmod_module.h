#ifndef DMOD_MODULE_H
#define DMOD_MODULE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dmod_defs.h"
#include "dmod_types.h"

/**
 * @brief Get the effective log level for the current module
 * 
 * On the first call the module context is looked up once and cached in a
 * module-wide static variable.  Subsequent calls use the cached context
 * directly, making them essentially a single indirect function call.
 * 
 * The initialization is protected by a critical section to prevent
 * concurrent first-call races.
 */
extern Dmod_LogLevel_t Dmod_GetLogLevel(void);

#ifdef __cplusplus
}
#endif

#endif // DMOD_MODULE_H