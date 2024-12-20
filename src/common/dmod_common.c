#include "dmod.h"
#include <string.h>

//==============================================================================
//                              FUNCTION IMPLEMENTATIONS
//==============================================================================

/**
 * @brief Check if API signature is valid
 * 
 * @param Signature API signature
 * 
 * @return true if signature is valid, false otherwise
 */
bool Dmod_IsApiSignatureValid( const char* Signature )
{
    if( Signature == NULL )
    {
        return false;
    }

    if( strncmp( Signature, DMOD_SIGNATURE_PREFIX, sizeof( DMOD_SIGNATURE_PREFIX ) - 1 ) != 0 )
    {
        return false;
    }

    return true;
}

/**
 * @brief Get number of entries in the API
 * 
 * @param Api API to get number of entries from
 * 
 * @return Number of entries in the API
 */
size_t Dmod_Api_GetNumberOfEntries( Dmod_Api_t* Api )
{
    if( Api == NULL )
    {
        return 0;
    }
    
    if( Api->SectionSize == 0 )
    {
        return 0;
    }
    size_t elementSize = 0;

    if( Api->ApiType == DMOD_API_TYPE_INPUT )
    {
        elementSize = sizeof(Api->InputSection->Entries[0]);
    }
    else if( Api->ApiType == DMOD_API_TYPE_OUTPUT )
    {
        elementSize = sizeof(Api->OutputSection->Entries[0]);
    }
    else 
    {
        DMOD_LOG_ERROR("Cannot get number of entries - invalid API type\n");
        return 0;
    }
}
