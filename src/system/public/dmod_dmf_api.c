#define DMOD_PRIVATE
#include "dmod.h"
#include "private/dmod_ctx.h"
#include "private/dmod_vars.h"
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
            else if( Dmod_ApiSignature_AreCompatible( OutputsApi->OutputSection->Entries[i], InputsApi->InputSection->Entries[j].Signature ) )
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
        DMOD_LOG_VERBOSE("Connecting API from %s to %s\n", Dmod_Context_GetModuleName( Dmod_Contexts[i] ), Dmod_Context_GetModuleName( Context ));

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

        DMOD_LOG_VERBOSE("Connecting API from %s to %s\n", Dmod_Context_GetModuleName( Context ), Dmod_Context_GetModuleName( Dmod_Contexts[i] ));

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
 * @brief Find the signature of a connected function matching a given pointer
 *
 * This is used by Dmod_VerifyAllApisSignatures to distinguish between an output
 * entry that is already connected (holds a valid function pointer) and one that
 * contains garbage or an unrecognised value.  When a match is found the
 * associated signature string is returned so callers can include it in log
 * messages.
 *
 * @param Pointer Pointer to look up
 *
 * @return Signature string of the matching input entry, or NULL if not found
 */
static const char* FindConnectedFunctionSignature( void* Pointer )
{
    if( Pointer == NULL )
    {
        return NULL;
    }

    // Check system builtin inputs
    if( Dmod_BuiltinInputApi.InputSection != NULL )
    {
        size_t numberOfBuiltinInputs = Dmod_Api_GetNumberOfEntries( &Dmod_BuiltinInputApi );
        for( size_t i = 0; i < numberOfBuiltinInputs; i++ )
        {
            if( Dmod_BuiltinInputApi.InputSection->Entries[i].Function == Pointer )
            {
                return Dmod_BuiltinInputApi.InputSection->Entries[i].Signature;
            }
        }
    }

    // Check loaded module inputs
    for( size_t i = 0; i < DMOD_MAX_MODULES; i++ )
    {
        if( Dmod_Contexts[i] == NULL || Dmod_Contexts[i]->Inputs.InputSection == NULL )
        {
            continue;
        }
        size_t numberOfInputs = Dmod_Api_GetNumberOfEntries( &Dmod_Contexts[i]->Inputs );
        for( size_t j = 0; j < numberOfInputs; j++ )
        {
            if( Dmod_Contexts[i]->Inputs.InputSection->Entries[j].Function == Pointer )
            {
                return Dmod_Contexts[i]->Inputs.InputSection->Entries[j].Signature;
            }
        }
    }

    return NULL;
}

/**
 * @brief Verify that all API signatures are valid
 * 
 * @param Context Context to verify APIs
 */
bool Dmod_VerifyAllApisSignatures( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot verify API signatures - invalid context\n");
        return false;
    }

    Dmod_EnterCritical();

    bool result = true;

    size_t numberOfOutputs = Dmod_Api_GetNumberOfEntries( &Context->Outputs );
    for(size_t i = 0; i < numberOfOutputs; i++)
    {
        if( !Dmod_ApiSignature_IsValid( Context->Outputs.OutputSection->Entries[i] ) )
        {
            const char* connectedSignature = FindConnectedFunctionSignature( Context->Outputs.OutputSection->Entries[i] );
            if( connectedSignature != NULL )
            {
                DMOD_LOG_VERBOSE("Output API at index %zu is already connected: %s\n", i, connectedSignature);
            }
            else
            {
                DMOD_LOG_ERROR("Invalid API signature in output API: %s\n", (const char*)(uintptr_t)Context->Outputs.OutputSection->Entries[i]);
                result = false;
            }
        }
    }

    size_t numberOfInputs = Dmod_Api_GetNumberOfEntries( &Context->Inputs );
    for(size_t i = 0; i < numberOfInputs; i++)
    {
        if( !Dmod_ApiSignature_IsValid( Context->Inputs.InputSection->Entries[i].Signature ) )
        {
            DMOD_LOG_ERROR("Invalid API signature in input API: %s\n", Context->Inputs.InputSection->Entries[i].Signature);
            result = false;
        }
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
    DMOD_LOG_VERBOSE("Connecting all APIs for module: %s\n", Dmod_Context_GetModuleName( Context ));
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot connect APIs - invalid context\n");
        return false;
    }
    if( !Dmod_VerifyAllApisSignatures( Context ) )
    {
        DMOD_LOG_WARN("Cannot connect APIs - invalid API signatures\n");
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
    DMOD_LOG_VERBOSE("Output APIs for %s:\n", Dmod_Context_GetModuleName( Context ));
    size_t numberOfEntries = Dmod_Api_GetNumberOfEntries( &Context->Outputs );
    bool crossplatform = Context->Outputs.Crossplatform;
    for(size_t i = 0; i < numberOfEntries; i++)
    {
        const char* entry = crossplatform ? 
            (const char*)(uintptr_t)Context->Outputs.OutputSectionCross->Entries[i] : 
            (const char*)Context->Outputs.OutputSection->Entries[i];
        DMOD_LOG_VERBOSE("  %s\n", entry);
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
    DMOD_LOG_VERBOSE("Input APIs for %s:\n", Dmod_Context_GetModuleName( Context ));
    size_t numberOfEntries = Dmod_Api_GetNumberOfEntries( &Context->Inputs );
    bool crossplatform = Context->Inputs.Crossplatform;
    for(size_t i = 0; i < numberOfEntries; i++)
    {
        const char* entry = crossplatform ? 
            (const char*)(uintptr_t)Context->Inputs.InputSectionCross->Entries[i].Signature : 
            (const char*)Context->Inputs.InputSection->Entries[i].Signature;
        DMOD_LOG_VERBOSE("  %s\n", entry);
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
 * @brief Get the number of output API entries for a module
 * 
 * @param Context Context to query
 * 
 * @return Number of output API entries, or 0 if the context is invalid
 */
size_t Dmod_GetOutputApiCount( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot get output API count - invalid context\n");
        return 0;
    }
    return Dmod_Api_GetNumberOfEntries( &Context->Outputs );
}

/**
 * @brief Get the number of input API entries for a module
 * 
 * @param Context Context to query
 * 
 * @return Number of input API entries, or 0 if the context is invalid
 */
size_t Dmod_GetInputApiCount( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot get input API count - invalid context\n");
        return 0;
    }
    return Dmod_Api_GetNumberOfEntries( &Context->Inputs );
}

/**
 * @brief Get the signature string for an output API entry by index
 * 
 * @param Context Context to query
 * @param Index   Zero-based index of the output API entry
 * 
 * @return Pointer to the null-terminated signature string, or NULL if the
 *         context is invalid or the index is out of range
 */
const char* Dmod_GetOutputApiSignature( Dmod_Context_t* Context, size_t Index )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot get output API signature - invalid context\n");
        return NULL;
    }
    if( Index >= Dmod_Api_GetNumberOfEntries( &Context->Outputs ) )
    {
        return NULL;
    }
    if( Context->Outputs.Crossplatform )
    {
        return (const char*)(uintptr_t)Context->Outputs.OutputSectionCross->Entries[Index];
    }
    return (const char*)Context->Outputs.OutputSection->Entries[Index];
}

/**
 * @brief Get the signature string for an input API entry by index
 * 
 * @param Context Context to query
 * @param Index   Zero-based index of the input API entry
 * 
 * @return Pointer to the null-terminated signature string, or NULL if the
 *         context is invalid or the index is out of range
 */
const char* Dmod_GetInputApiSignature( Dmod_Context_t* Context, size_t Index )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot get input API signature - invalid context\n");
        return NULL;
    }
    if( Index >= Dmod_Api_GetNumberOfEntries( &Context->Inputs ) )
    {
        return NULL;
    }
    if( Context->Inputs.Crossplatform )
    {
        return (const char*)(uintptr_t)Context->Inputs.InputSectionCross->Entries[Index].Signature;
    }
    return Context->Inputs.InputSection->Entries[Index].Signature;
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
        if( Dmod_ApiSignature_AreCompatible( Context->Inputs.InputSection->Entries[i].Signature, Signature ) )
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
            if( Dmod_ApiSignature_AreCompatible( Dmod_Contexts[i]->Inputs.InputSection->Entries[j].Signature, DifSignature ) )
            {
                Dmod_ExitCritical();
                return Dmod_Contexts[i];
            }
        }
    }

    Dmod_ExitCritical();
    return NULL;
}

