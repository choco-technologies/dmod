#include "dmod.h"

Dmod_Context_t Dmod_Load( const char* Path, const Dmod_MemIf_t* Memory, const Dmod_FileIf_t* File )
{
    if( !Memory || !File )
    {
        return NULL;
    }

    
}