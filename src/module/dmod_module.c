#include "dmod.h"

/* Every module binary defines DMOD_Header in its generated _header.c file. */
extern volatile const Dmod_ModuleHeader_t* DMOD_Header;

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
    static Dmod_Context_t* s_ctx = NULL;

    Dmod_EnterCritical();
    if( !s_ctx )
    {
        s_ctx = Dmod_GetModuleContext(Dmod_GetCurrentModuleName());
    }
    Dmod_ExitCritical();

    return Dmod_GetModuleLogLevel(s_ctx);
}

/**
 * @brief Get the input API registrations of the running module.
 *
 * Reads the module footer to locate the .dmod.inputs section and returns
 * a pointer to the first Dmod_ApiRegistration_t entry together with the
 * number of entries.  This encapsulates the footer pointer arithmetic so
 * callers do not need to duplicate it.
 *
 * @param outEntries  Set to the first entry in the .dmod.inputs section.
 * @param outCount    Set to the number of entries.
 * @return true on success, false if a required pointer is NULL.
 */
bool Dmod_Module_GetInputs( Dmod_ApiRegistration_t** outEntries, uint32_t* outCount )
{
    if( outEntries == NULL || outCount == NULL )
    {
        return false;
    }

    if( DMOD_Header == NULL || DMOD_Header->Footer.Ptr == NULL )
    {
        return false;
    }

    Dmod_ModuleFooter_t* footer = (Dmod_ModuleFooter_t*)DMOD_Header->Footer.Ptr;
    uint8_t*             base   = (uint8_t*)(uintptr_t)DMOD_Header;

    *outEntries = (Dmod_ApiRegistration_t*)( base + footer->Inputs.SectionStart );
    *outCount   = footer->Inputs.SectionSize / sizeof( Dmod_ApiRegistration_t );

    return true;
}

