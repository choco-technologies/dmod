#define DMOD_PRIVATE
#include "dmod.h"
#include "private/dmod_vars.h"
#include "private/dmod_ctx.h"
#include "private/dmod_mgr.h"

/**
 * @brief checks if the given module is a system module
 * 
 * @param ModuleName Name of the module to check
 * 
 * @return True if module is a system module, false otherwise
 */
bool Dmod_Mgr_IsSystemModule( const char* ModuleName )
{
    if(ModuleName == NULL || ModuleName[0] == 0)
    {
        DMOD_LOG_ERROR("Cannot check if module is system module - invalid module name (empty or NULL)\n");
        return false;
    }
    Dmod_BuiltinInputApi.SectionSize = (size_t)((void*)&__dmod_inputs_end - (void*)&__dmod_inputs_start);
    size_t numberOfEntries = Dmod_Api_GetNumberOfEntries( &Dmod_BuiltinInputApi );
    for(size_t i = 0; i < numberOfEntries; i++)
    {
        if( Dmod_ApiSignature_IsModule(Dmod_BuiltinInputApi.InputSection->Entries[i].Signature, ModuleName) )
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief Checks if the given module is loaded
 * 
 * @param ModuleName Name of the module to check
 * 
 * @return True if module is loaded, false otherwise
 */
bool Dmod_Mgr_IsLoaded( const char* ModuleName )
{
    if(Dmod_Mgr_IsSystemModule( ModuleName ))
    {
        return true; 
    }

    return Dmod_Context_Get( ModuleName ) != NULL;
}

/**
 * @brief Checks if the given module is enabled
 * 
 * @param ModuleName Name of the module to check
 * 
 * @return True if module is enabled, false otherwise
 */
bool Dmod_Mgr_IsEnabled( const char* ModuleName )
{
    if(Dmod_Mgr_IsSystemModule( ModuleName ))
    {
        return true; 
    }
    Dmod_Context_t* context = Dmod_Context_Get( ModuleName );
    return context != NULL && Dmod_IsEnabled( context );
}
