#include "dmod.h"

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