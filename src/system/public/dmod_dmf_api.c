#define DMOD_PRIVATE
#include "dmod.h"
#include "private/dmod_ctx.h"
#include "private/dmod_vars.h"


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
