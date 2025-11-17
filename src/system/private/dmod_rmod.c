#define DMOD_PRIVATE
#include "dmod.h"
#include "private/dmod_rmod.h"
#include "private/dmod_mgr.h"
#include "private/dmod_vars.h"
#include "private/dmod_ctx.h"

#include <string.h>


/**
 * @brief Find context
 * 
 * @param ModuleName Name of the module to find
 * 
 * @return Pointer to the context
 */
Dmod_RequiredModule_t* Dmod_RMod_FindRequiredModule( Dmod_Context_t* Context, const char* ModuleName )
{
    if( Context == NULL || ModuleName == NULL )
    {
        return NULL;
    }

    for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
    {
        if( Context->RequiredModules[i].Name[0] == 0 )
        {
            continue;
        }

        if( strcmp( Context->RequiredModules[i].Name, ModuleName ) == 0 )
        {
            return &Context->RequiredModules[i];
        }
    }

    return NULL;
}

/**
 * @brief Find empty required module
 * 
 * @param Context Context to find empty required module in
 * 
 * @return Pointer to the empty required module
 */
Dmod_RequiredModule_t* Dmod_RMod_FindEmptyRequiredModule( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return NULL;
    }

    for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
    {
        if( Context->RequiredModules[i].Name[0] == 0 )
        {
            return &Context->RequiredModules[i];
        }
    }

    return NULL;
}

/**
 * @brief checks if the given module is required by the current module
 * 
 * @param Context Context to check
 * @param ModuleName Name of the module to check
 * 
 * @return True if module is required, false otherwise
 */
bool Dmod_RMod_IsModuleRequired( Dmod_Context_t* Context, const char* ModuleName )
{
    return Dmod_RMod_FindRequiredModule( Context, ModuleName ) != NULL;
}

/**
 * @brief Add required module
 * 
 * @param Context Context to add required module to
 * @param ApiSignature Signature of the API
 * 
 * @return True if required module was added successfully, false otherwise
 */
bool Dmod_RMod_AddRequiredModule( Dmod_Context_t* Context, const char* ApiSignature )
{
    if( Context == NULL || ApiSignature == NULL )
    {
        return false;
    }

    char moduleName[DMOD_MAX_MODULE_NAME_LENGTH] = {0};
    if( !Dmod_ApiSignature_ReadModuleName( ApiSignature, moduleName, sizeof(moduleName) ) )
    {
        DMOD_LOG_ERROR("Cannot add required module - cannot read module name\n");
        return false;
    }

    if(strncmp(Dmod_Context_GetModuleName(Context), moduleName, sizeof(moduleName)) == 0)
    {
        DMOD_LOG_ERROR("Cannot add required module - module cannot require itself\n");
        return false;
    }

    if(Dmod_RMod_IsModuleRequired( Context, moduleName ))
    {
        return true;
    }

    Dmod_RequiredModule_t* requiredModule = Dmod_RMod_FindEmptyRequiredModule( Context );
    if( requiredModule == NULL )
    {
        DMOD_LOG_ERROR("Cannot add required module - no space left\n");
        return false;
    }

    strncpy( requiredModule->Name, moduleName, sizeof(requiredModule->Name) );
    requiredModule->SystemModule = Dmod_Mgr_IsSystemModule( moduleName );
    if(requiredModule->SystemModule)
    {
        strncpy( requiredModule->Version, DMOD_VERSION_STRING, sizeof(requiredModule->Version) );
    }
    else if(Dmod_ApiSignature_ReadModuleVersion( ApiSignature, requiredModule->Version, sizeof(requiredModule->Version) ) == false)
    {
        DMOD_LOG_WARN("Version of the module is not given for: %s\n", moduleName);
    }
    bool versionGiven = requiredModule->Version[0] != '\0';

    DMOD_LOG_VERBOSE("Required %smodule '%s%s%s' added to '%s'\n", 
        requiredModule->SystemModule ? "system " : "", 
        requiredModule->Name, 
        versionGiven ? "@" : "",
        requiredModule->Version, 
        Dmod_Context_GetModuleName( Context ));

    return true;
}

/**
 * @brief Read required modules
 * 
 * @param Context Context to read required modules to
 * 
 * @return True if required modules were read successfully, false otherwise
 */
bool Dmod_RMod_ReadRequiredModules( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    // Clear required modules
    memset( Context->RequiredModules, 0, sizeof(Context->RequiredModules) );

    size_t numberOfOuptuts = Dmod_Api_GetNumberOfEntries( &Context->Outputs );
    for(size_t outputIndex = 0; outputIndex < numberOfOuptuts; outputIndex++)
    {
        const char* apiSignature = Context->Outputs.OutputSection->Entries[outputIndex];
        if(apiSignature == NULL)
        {
            continue;
        }
        if(!Dmod_ApiSignature_IsModuleNameGiven(apiSignature) || Dmod_ApiSignature_IsMal(apiSignature))
        {
            continue;
        }
        if(!Dmod_RMod_AddRequiredModule( Context, apiSignature ))
        {
            DMOD_LOG_ERROR("Cannot read required modules for %s - cannot add required module\n", Dmod_Context_GetModuleName( Context ));
            return false;
        }
    }

    return true;
}


/**
 * @brief Are required modules enabled
 * 
 * @param Context Context to check
 * 
 * @return True if required modules are enabled, false otherwise
 */
bool Dmod_RMod_AreRequiredModulesEnabled( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        DMOD_LOG_ERROR("Cannot check required modules - invalid context\n");
        return false;
    }

    for(size_t i = 0; i < DMOD_MAX_REQUIRED_MODULES; i++)
    {
        if( Context->RequiredModules[i].Name[0] == 0 )
        {
            continue;
        }

        if( !Dmod_Mgr_IsEnabled( Context->RequiredModules[i].Name ) )
        {
            DMOD_LOG_VERBOSE("Required module '%s' is not enabled\n", Context->RequiredModules[i].Name);
            return false;
        }
    }

    return true;
}


/**
 * @brief Checks if the given module is required by another module
 * 
 * @param Context Context to check
 * @param OnlyEnabled If true, only enabled modules will be checked
 * 
 * @return Pointer to the module that requires the given module
 */
Dmod_Context_t* Dmod_RMod_FindDependentModule( Dmod_Context_t* Context, bool OnlyEnabled )
{
    if( Context == NULL )
    {
        return NULL;
    }

    for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
    {
        if( Dmod_Contexts[i] == NULL )
        {
            continue;
        }

        if( Dmod_Contexts[i] == Context )
        {
            continue;
        }

        if( OnlyEnabled && !Dmod_IsEnabled( Dmod_Contexts[i] ) )
        {
            continue;
        }

        if( Dmod_RMod_IsModuleRequired( Dmod_Contexts[i], Dmod_Context_GetModuleName( Context ) ) )
        {
            return Dmod_Contexts[i];
        }
    }

    return NULL;
}

/**
 * @brief Load required modules
 * 
 * @param Context Context to load required modules to
 * 
 * @return True if required modules were loaded successfully, false otherwise
 */
bool Dmod_RMod_LoadRequiredModules( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    for(size_t i = 0; i < DMOD_MAX_REQUIRED_MODULES; i++)
    {
        if( Context->RequiredModules[i].Name[0] == 0 )
        {
            continue;
        }

        if(Dmod_Mgr_IsSystemModule(Context->RequiredModules[i].Name))
        {
            continue;
        }

        if(Context->PackageName != NULL)
        {
            DMOD_LOG_VERBOSE("Loading required module '%s' for '%s' from package '%s'\n", Context->RequiredModules[i].Name, Dmod_Context_GetModuleName( Context ), Context->PackageName);
            if( Dmod_LoadModuleFromPackage( Context->RequiredModules[i].Name, Context->PackageName ) )
            {
                continue;
            }
            DMOD_LOG_VERBOSE("Cannot load required module '%s' for '%s' from package '%s' - trying to load from repository\n", Context->RequiredModules[i].Name, Dmod_Context_GetModuleName( Context ), Context->PackageName);
        }

        if( !Dmod_LoadModuleByName( Context->RequiredModules[i].Name ) )
        {
            DMOD_LOG_ERROR("Cannot load required module '%s'\n", Context->RequiredModules[i].Name);
            return false;
        }
    }

    DMOD_LOG_VERBOSE("All required modules loaded for '%s'\n", Dmod_Context_GetModuleName( Context ));
    return true;
}

/**
 * @brief Enable required modules
 * 
 * @param Context Context to enable required modules to
 * 
 * @return True if required modules were enabled successfully, false otherwise
 */
bool Dmod_RMod_EnableRequiredModules( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    for(size_t i = 0; i < DMOD_MAX_REQUIRED_MODULES; i++)
    {
        if( Context->RequiredModules[i].Name[0] == 0 )
        {
            continue;
        }

        if(Dmod_Mgr_IsSystemModule(Context->RequiredModules[i].Name))
        {
            continue;
        }

        if( !Dmod_EnableModule( Context->RequiredModules[i].Name, false, NULL ) )
        {
            DMOD_LOG_ERROR("Cannot enable required module '%s'\n", Context->RequiredModules[i].Name);
            return false;
        }
    }

    return true;
}