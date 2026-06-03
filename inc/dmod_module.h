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

/**
 * @brief Get the input API registrations of the running module.
 *
 * Reads the module footer to locate the .dmod.inputs section and returns
 * a pointer to its entries together with the entry count.  This is the
 * canonical way for module-side code to iterate its own registrations;
 * it avoids open-coding the footer pointer arithmetic in multiple places.
 *
 * @param outEntries  Set to the first Dmod_ApiRegistration_t in the section.
 *                    Must not be NULL.
 * @param outCount    Set to the number of entries in the section.
 *                    Must not be NULL.
 * @return true on success, false if the module header or footer is unavailable.
 */
extern bool Dmod_Module_GetInputs( Dmod_ApiRegistration_t** outEntries, uint32_t* outCount );

#ifdef __cplusplus
}
#endif

#endif // DMOD_MODULE_H