#ifndef DMOD_RMOD_H
#define DMOD_RMOD_H

#ifndef DMOD_PRIVATE
#   error "This is private DMOD header. Don't include this outside DMOD library"
#endif

#include "dmod_types.h"

extern Dmod_RequiredModule_t*   Dmod_RMod_FindRequiredModule( Dmod_Context_t* Context, const char* ModuleName );
extern Dmod_RequiredModule_t*   Dmod_RMod_FindEmptyRequiredModule( Dmod_Context_t* Context );
extern bool                     Dmod_RMod_IsModuleRequired( Dmod_Context_t* Context, const char* ModuleName );
extern bool                     Dmod_RMod_ReadRequiredModules( Dmod_Context_t* Context );
extern bool                     Dmod_RMod_AddRequiredModule( Dmod_Context_t* Context, const char* ApiSignature );
extern bool                     Dmod_RMod_AreRequiredModulesEnabled( Dmod_Context_t* Context );
extern Dmod_Context_t*          Dmod_RMod_FindDependentModule( Dmod_Context_t* Context, bool OnlyEnabled );
extern bool                     Dmod_RMod_LoadRequiredModules( Dmod_Context_t* Context );
extern bool                     Dmod_RMod_EnableRequiredModules( Dmod_Context_t* Context );


#endif // DMOD_RMOD_H