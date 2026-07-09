#ifndef DMOD_CTX_H
#define DMOD_CTX_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DMOD_PRIVATE
#   error "This is private DMOD header. Don't include this outside DMOD library"
#endif

#include "dmod_types.h"

extern Dmod_Context_t*   Dmod_Context_New( void* Data, size_t FileSize, const char* ModuleName );
extern bool              Dmod_Context_IsValid( Dmod_Context_t* Context );
extern void              Dmod_Context_Delete( Dmod_Context_t* Context );
extern const char*       Dmod_Context_GetModuleName( Dmod_Context_t* Context );
extern Dmod_ModuleType_t Dmod_Context_GetModuleType( Dmod_Context_t* Context );

extern bool            Dmod_Context_Add( Dmod_Context_t* Context );
extern bool            Dmod_Context_Remove( Dmod_Context_t* Context );
extern Dmod_Context_t* Dmod_Context_Get( const char* ModuleName );

#ifdef __cplusplus
}
#endif

#endif // DMOD_CTX_H