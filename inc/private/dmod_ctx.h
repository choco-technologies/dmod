#ifndef DMOD_CTX_H
#define DMOD_CTX_H

#ifndef DMOD_PRIVATE
#   error "This is private DMOD header. Don't include this outside DMOD library"
#endif

#include "dmod_types.h"

extern Dmod_Context_t* Dmod_Context_New( void* Data, size_t FileSize );
extern bool            Dmod_Context_IsValid( Dmod_Context_t* Context );
extern void            Dmod_Context_Delete( Dmod_Context_t* Context );
extern const char*     Dmod_Context_GetModuleName( Dmod_Context_t* Context );

extern bool            Dmod_Context_Add( Dmod_Context_t* Context );
extern bool            Dmod_Context_Remove( Dmod_Context_t* Context );
extern Dmod_Context_t* Dmod_Context_Get( const char* ModuleName );

#endif // DMOD_CTX_H