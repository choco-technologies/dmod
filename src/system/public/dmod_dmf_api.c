#define DMOD_PRIVATE
#include "dmod.h"
#include "private/dmod_ctx.h"
#include "private/dmod_vars.h"
#include "private/dmod_pck.h"
#include "private/dmod_mgr.h"
#include <string.h>


/**
 * @brief Connect API
 * 
 * @param Outputs Outputs section
 * @param Inputs Inputs section
 */
bool Dmod_ConnectApi( Dmod_Api_t* OutputsApi, Dmod_Api_t* InputsApi )
{
    if( OutputsApi == NULL || InputsApi == NULL )
    {
        DMOD_LOG_ERROR("Cannot connect API - invalid API pointers\n");
        return false;
    }
    
    size_t numberOfOutputs = Dmod_Api_GetNumberOfEntries( OutputsApi );
    size_t numberOfInputs = Dmod_Api_GetNumberOfEntries( InputsApi );

    for(size_t i = 0; i < numberOfOutputs; i++)
    {
        for(size_t j = 0; j < numberOfInputs; j++)
        {
            if( !Dmod_ApiSignature_IsValid( OutputsApi->OutputSection->Entries[i] ) )
            {
                continue;
            }
            else if( Dmod_ApiSignature_AreEqual( OutputsApi->OutputSection->Entries[i], InputsApi->InputSection->Entries[j].Signature ) )
            {
                DMOD_LOG_VERBOSE("Connected: %s 0x%08X\n", InputsApi->InputSection->Entries[j].Signature, InputsApi->InputSection->Entries[j].Function);
                OutputsApi->OutputSection->Entries[i] = InputsApi->InputSection->Entries[j].Function;
            }
        }
    }
    return true;
}

/**
 * @brief Disconnect API
 * 
 * @param Outputs Outputs section
 * @param Inputs Inputs section
 */
bool Dmod_DisconnectApi( Dmod_Api_t* OutputsApi, Dmod_Api_t* InputsApi )
{
    if( OutputsApi == NULL || InputsApi == NULL )
    {
        DMOD_LOG_ERROR("Cannot disconnect API - invalid API pointers\n");
        return false;
    }
    
    size_t numberOfOutputs = Dmod_Api_GetNumberOfEntries( OutputsApi );
    size_t numberOfInputs = Dmod_Api_GetNumberOfEntries( InputsApi );

    for(size_t i = 0; i < numberOfOutputs; i++)
    {
        for(size_t j = 0; j < numberOfInputs; j++)
        {
            if( !Dmod_ApiSignature_IsValid( OutputsApi->OutputSection->Entries[i] ) )
            {
                continue;
            }
            else if( OutputsApi->OutputSection->Entries[i] == InputsApi->InputSection->Entries[j].Function )
            {
                DMOD_LOG_VERBOSE("Disconnected: %s\n", InputsApi->InputSection->Entries[j].Signature);
                OutputsApi->OutputSection->Entries[i] = (void*)InputsApi->InputSection->Entries[j].Signature;
            }
        }
    }
    return true;
}

/**
 * @brief Connect output APIs
 * 
 * @param Context Context to connect APIs
 */
bool Dmod_ConnectOutputApis( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot connect APIs - invalid context\n");
        return false;
    }
    Dmod_EnterCritical();

    Dmod_BuiltinInputApi.SectionSize = (size_t)((void*)&__dmod_inputs_end - (void*)&__dmod_inputs_start);
    if( !Dmod_ConnectApi( &Context->Outputs, &Dmod_BuiltinInputApi ) )
    {
        DMOD_LOG_ERROR("Cannot connect system API to '%s' APIs\n", Dmod_Context_GetModuleName( Context ));
        Dmod_ExitCritical();
        return false;
    }

    bool result = true;
    for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
    {
        if( Dmod_Contexts[i] == NULL || Context == Dmod_Contexts[i] )
        {
            continue;
        }
        DMOD_LOG_INFO("Connecting API from %s to %s\n", Dmod_Context_GetModuleName( Dmod_Contexts[i] ), Dmod_Context_GetModuleName( Context ));

        if( !Dmod_ConnectApi( &Context->Outputs, &Dmod_Contexts[i]->Inputs ) )
        {
            DMOD_LOG_ERROR("Cannot connect API from '%s' to '%s'\n", Dmod_Context_GetModuleName( Dmod_Contexts[i] ), Dmod_Context_GetModuleName( Context ));
            result = false;
        }
    }

    if(!result)
    {
        Dmod_DisconnectOutputApis( Context );
    }
    
    Dmod_ExitCritical();
    return result;
}

/**
 * @brief Connect input APIs
 * 
 * @param Context Context to connect APIs
 */
bool Dmod_ConnectInputApis( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot connect APIs - invalid context\n");
        return false;
    }

    Dmod_EnterCritical();

    Dmod_BuiltinOutputApi.SectionSize = (size_t)((void*)&__dmod_outputs_end - (void*)&__dmod_outputs_start);
    if( !Dmod_ConnectApi( &Dmod_BuiltinOutputApi, &Context->Inputs ) )
    {
        DMOD_LOG_ERROR("Cannot connect API '%s' to system\n", Dmod_Context_GetModuleName( Context ));
        Dmod_ExitCritical();
        return false;
    }

    bool result = true;

    for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
    {
        if( Dmod_Contexts[i] == NULL )
        {
            continue;
        }

        DMOD_LOG_INFO("Connecting API from %s to %s\n", Dmod_Context_GetModuleName( Context ), Dmod_Context_GetModuleName( Dmod_Contexts[i] ));

        if( !Dmod_ConnectApi( &Dmod_Contexts[i]->Outputs, &Context->Inputs ) )
        {
            DMOD_LOG_ERROR("Cannot connect API from '%s' to '%s'\n", Dmod_Context_GetModuleName( Context ), Dmod_Context_GetModuleName( Dmod_Contexts[i] ));
            result = false;
        }
    }

    if(!result)
    {
        Dmod_DisconnectInputApis( Context );
    }

    Dmod_ExitCritical();

    return result;
}

/**
 * @brief Connect all APIs
 * 
 * @param Context Context to connect APIs
 */
bool Dmod_ConnectAllApis( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot connect APIs - invalid context\n");
        return false;
    }

    Dmod_EnterCritical();
    bool result = Dmod_ConnectOutputApis( Context ) && Dmod_ConnectInputApis( Context );
    Dmod_ExitCritical();

    return result;
}

/**
 * @brief Disconnect output APIs
 * 
 * @param Context Context to disconnect APIs
 */
bool Dmod_DisconnectOutputApis( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot disconnect APIs - invalid context\n");
        return false;
    }

    Dmod_EnterCritical();
    bool result = true;

    for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
    {
        if( Dmod_Contexts[i] == NULL )
        {
            continue;
        }

        if( !Dmod_DisconnectApi( &Context->Outputs, &Dmod_Contexts[i]->Inputs ) )
        {
            DMOD_LOG_ERROR("Cannot disconnect %s's API from '%s'\n", Dmod_Context_GetModuleName( Dmod_Contexts[i] ), Dmod_Context_GetModuleName( Context ));
            result = false;
        }
    }

    if( !Dmod_DisconnectApi( &Context->Outputs, &Dmod_BuiltinInputApi ) )
    {
        DMOD_LOG_ERROR("Cannot disconnect system API from '%s'\n", Dmod_Context_GetModuleName( Context ));
        result = false;
    }
    Dmod_ExitCritical();

    return result;
}

/**
 * @brief Disconnect input APIs
 * 
 * @param Context Context to disconnect APIs
 */
bool Dmod_DisconnectInputApis( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot disconnect APIs - invalid context\n");
        return false;
    }

    Dmod_EnterCritical();
    bool result = true;

    for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
    {
        if( Dmod_Contexts[i] == NULL )
        {
            continue;
        }

        if( !Dmod_DisconnectApi( &Dmod_Contexts[i]->Outputs, &Context->Inputs ) )
        {
            DMOD_LOG_ERROR("Cannot disconnect %s's API from '%s'\n", Dmod_Context_GetModuleName( Context ), Dmod_Context_GetModuleName( Dmod_Contexts[i] ));
            result = false;
        }
    }

    if( !Dmod_DisconnectApi( &Dmod_BuiltinOutputApi, &Context->Inputs ) )
    {
        DMOD_LOG_ERROR("Cannot disconnect %s's API from system\n", Dmod_Context_GetModuleName( Context ));
        result = false;
    }

    Dmod_ExitCritical();

    return result;
}

/**
 * @brief Disconnect all APIs
 * 
 * @param Context Context to disconnect APIs
 */
bool Dmod_DisconnectAllApis( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot disconnect APIs - invalid context\n");
        return false;
    }

    Dmod_EnterCritical();
    bool result = Dmod_DisconnectInputApis( Context ) && Dmod_DisconnectOutputApis( Context );
    Dmod_ExitCritical();
    if( !result )
    {
        DMOD_LOG_ERROR("Cannot disconnect APIs - failed to disconnect\n");
        return false;
    }

    return true;
}

/**
 * @brief Print output APIs
 * 
 * @param Context Context to print APIs
 */
void Dmod_PrintOutputApis( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot print APIs - invalid context\n");
        return;
    }
    DMOD_LOG_INFO("Output APIs for %s:\n", Dmod_Context_GetModuleName( Context ));
    size_t numberOfEntries = Dmod_Api_GetNumberOfEntries( &Context->Outputs );
    for(size_t i = 0; i < numberOfEntries; i++)
    {
        DMOD_LOG_INFO("  %s\n", Context->Outputs.OutputSection->Entries[i]);
    }
}

/**
 * @brief Print input APIs
 * 
 * @param Context Context to print APIs
 */
void Dmod_PrintInputApis( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot print APIs - invalid context\n");
        return;
    }
    DMOD_LOG_INFO("Input APIs for %s:\n", Dmod_Context_GetModuleName( Context ));
    size_t numberOfEntries = Dmod_Api_GetNumberOfEntries( &Context->Inputs );
    for(size_t i = 0; i < numberOfEntries; i++)
    {
        DMOD_LOG_INFO("  %s\n", Context->Inputs.InputSection->Entries[i].Signature);
    }
}

/**
 * @brief Print all APIs
 * 
 * @param Context Context to print APIs
 */
void Dmod_PrintAllApis( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot print APIs - invalid context\n");
        return;
    }
    Dmod_PrintOutputApis( Context );
    Dmod_PrintInputApis( Context );
}

/**
 * @brief Get function
 * 
 * @param Context Context to get function from
 * @param Signature Signature of the function
 * 
 * @return Pointer to the function
 */
void* Dmod_GetFunction( Dmod_Context_t* Context, const char* Signature )
{
    if( !Dmod_Context_IsValid( Context ) || !Dmod_ApiSignature_IsValid( Signature ) )
    {
        DMOD_LOG_ERROR("Cannot get function - invalid context or signature\n");
        return NULL;
    }

    if( Context->Inputs.InputSection == NULL )
    {
        DMOD_LOG_ERROR("Cannot get function - no output section\n");
        return NULL;
    }

    size_t numberOfEntries = Dmod_Api_GetNumberOfEntries( &Context->Inputs );
    for(size_t i = 0; i < numberOfEntries; i++)
    {
        if( Dmod_ApiSignature_AreEqual( Context->Inputs.InputSection->Entries[i].Signature, Signature ) )
        {
            return Context->Inputs.InputSection->Entries[i].Function;
        }
    }

    DMOD_LOG_ERROR("Cannot get function - function not found: %s\n", Signature);
    return NULL;
}

/**
 * @brief Get next module that implements the given DIF interface
 * 
 * @param DifSignature DIF function signature to search for
 * @param Previous Previous module context (NULL to start from beginning)
 * 
 * @return Pointer to module context or NULL if no more modules found
 */
Dmod_Context_t* Dmod_GetNextDifModule( const char* DifSignature, Dmod_Context_t* Previous )
{
    if( !Dmod_ApiSignature_IsValid( DifSignature ) )
    {
        DMOD_LOG_ERROR("Cannot get next DIF module - invalid signature\n");
        return NULL;
    }

    // Check if signature is a DIF signature
    if( strncmp( DifSignature, DMOD_DIF_SIGNATURE_PREFIX, sizeof( DMOD_DIF_SIGNATURE_PREFIX ) - 1 ) != 0 )
    {
        DMOD_LOG_ERROR("Cannot get next DIF module - signature is not a DIF signature\n");
        return NULL;
    }

    Dmod_EnterCritical();
    
    size_t startIndex = 0;
    if( Previous != NULL )
    {
        // Find the previous context in the list
        for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
        {
            if( Dmod_Contexts[i] == Previous )
            {
                startIndex = i + 1;
                break;
            }
        }
    }

    // Search for modules implementing this DIF
    for(size_t i = startIndex; i < DMOD_MAX_MODULES; i++)
    {
        if( Dmod_Contexts[i] == NULL || !Dmod_Context_IsValid( Dmod_Contexts[i] ) )
        {
            continue;
        }

        // Only consider enabled modules
        if( !Dmod_IsEnabled( Dmod_Contexts[i] ) )
        {
            continue;
        }

        // Check if this module implements the DIF
        size_t numberOfInputs = Dmod_Api_GetNumberOfEntries( &Dmod_Contexts[i]->Inputs );
        for(size_t j = 0; j < numberOfInputs; j++)
        {
            if( Dmod_ApiSignature_AreEqual( Dmod_Contexts[i]->Inputs.InputSection->Entries[j].Signature, DifSignature ) )
            {
                Dmod_ExitCritical();
                return Dmod_Contexts[i];
            }
        }
    }

    Dmod_ExitCritical();
    return NULL;
}

/**
 * @brief Get DIF function pointer from module context
 * 
 * @param Context Module context
 * @param DifSignature DIF function signature
 * 
 * @return Function pointer or NULL if not found
 */
void* Dmod_GetDifFunction( Dmod_Context_t* Context, const char* DifSignature )
{
    if( !Dmod_Context_IsValid( Context ) || !Dmod_ApiSignature_IsValid( DifSignature ) )
    {
        DMOD_LOG_ERROR("Cannot get DIF function - invalid context or signature\n");
        return NULL;
    }

    // Check if signature is a DIF signature
    if( strncmp( DifSignature, DMOD_DIF_SIGNATURE_PREFIX, sizeof( DMOD_DIF_SIGNATURE_PREFIX ) - 1 ) != 0 )
    {
        DMOD_LOG_ERROR("Cannot get DIF function - signature is not a DIF signature\n");
        return NULL;
    }

    if( Context->Inputs.InputSection == NULL )
    {
        DMOD_LOG_ERROR("Cannot get DIF function - no input section\n");
        return NULL;
    }

    size_t numberOfEntries = Dmod_Api_GetNumberOfEntries( &Context->Inputs );
    for(size_t i = 0; i < numberOfEntries; i++)
    {
        if( Dmod_ApiSignature_AreEqual( Context->Inputs.InputSection->Entries[i].Signature, DifSignature ) )
        {
            return Context->Inputs.InputSection->Entries[i].Function;
        }
    }

    DMOD_LOG_ERROR("Cannot get DIF function - function not found: %s in module %s\n", DifSignature, Dmod_Context_GetModuleName( Context ));
    return NULL;
}

/**
 * @brief Get next module in the system
 * 
 * @param Last Pointer to the last module info (NULL to start from beginning)
 * 
 * @return Pointer to the next module info or NULL if no more modules
 * 
 * @note This function iterates through all loaded modules and available modules
 *       in loaded packages. Call with NULL to start iteration, then pass the 
 *       returned pointer repeatedly until NULL is returned.
 */
const Dmod_ModuleInfo_t* Dmod_GetNextModule( const Dmod_ModuleInfo_t* Last )
{
    static Dmod_ModuleInfo_t moduleInfo;
    static size_t contextIndex = 0;
    static size_t packageIndex = 0;
    static size_t moduleInPackageIndex = 0;
    
    Dmod_EnterCritical();
    
    // Reset iteration state if starting from beginning
    if( Last == NULL )
    {
        contextIndex = 0;
        packageIndex = 0;
        moduleInPackageIndex = 0;
    }
    else
    {
        // Advance to next position based on current state
        // The logic will increment indices as we iterate
    }
    
    // First, iterate through loaded modules
    for(size_t i = contextIndex; i < DMOD_MAX_MODULES; i++)
    {
        if( Dmod_Contexts[i] == NULL || !Dmod_Context_IsValid( Dmod_Contexts[i] ) )
        {
            continue;
        }
        
        if( Dmod_Contexts[i]->Header == NULL )
        {
            continue;
        }
        
        // Fill in module info
        strncpy( moduleInfo.ModuleName, Dmod_Contexts[i]->Header->Name, DMOD_MAX_MODULE_NAME_LENGTH - 1 );
        moduleInfo.ModuleName[DMOD_MAX_MODULE_NAME_LENGTH - 1] = '\0';
        
        strncpy( moduleInfo.Version, Dmod_Contexts[i]->Header->Version, DMOD_MAX_VERSION_LENGTH - 1 );
        moduleInfo.Version[DMOD_MAX_VERSION_LENGTH - 1] = '\0';
        
        // Determine module state
        if( Dmod_Contexts[i]->Running )
        {
            moduleInfo.State = Dmod_ModuleState_Running;
        }
        else if( Dmod_Contexts[i]->Enabled )
        {
            moduleInfo.State = Dmod_ModuleState_Enabled;
        }
        else
        {
            moduleInfo.State = Dmod_ModuleState_Loaded;
        }
        
        // Save state for next iteration
        contextIndex = i + 1;
        
        Dmod_ExitCritical();
        return &moduleInfo;
    }
    
    // After loaded modules, iterate through modules in packages
    for(size_t pkgIdx = packageIndex; pkgIdx < DMOD_MAX_NUMBER_OF_PACKAGES; pkgIdx++)
    {
        Dmod_PackageSlot_t* slot = &Dmod_Packages[pkgIdx];
        
        if( !Dmod_Pck_IsSlotUsed( slot ) || !Dmod_Pck_IsValidSlot( slot ) )
        {
            continue;
        }
        
        if( slot->DmpHeader == NULL || slot->ModuleEntries == NULL )
        {
            continue;
        }
        
        // Iterate through modules in this package
        for(size_t modIdx = moduleInPackageIndex; modIdx < slot->DmpHeader->ModuleCount; modIdx++)
        {
            const char* moduleName = slot->ModuleEntries[modIdx].ModuleName;
            
            // Skip if this module is already loaded
            if( Dmod_Mgr_IsLoaded( moduleName ) )
            {
                continue;
            }
            
            // Fill in module info for available module
            strncpy( moduleInfo.ModuleName, moduleName, DMOD_MAX_MODULE_NAME_LENGTH - 1 );
            moduleInfo.ModuleName[DMOD_MAX_MODULE_NAME_LENGTH - 1] = '\0';
            
            // We don't have version info for modules in packages until they're loaded
            moduleInfo.Version[0] = '\0';
            
            // Module is available but not loaded
            moduleInfo.State = Dmod_ModuleState_Available;
            
            // Save state for next iteration
            packageIndex = pkgIdx;
            moduleInPackageIndex = modIdx + 1;
            
            Dmod_ExitCritical();
            return &moduleInfo;
        }
        
        // Reset module index when moving to next package
        moduleInPackageIndex = 0;
    }
    
    Dmod_ExitCritical();
    return NULL;
}

