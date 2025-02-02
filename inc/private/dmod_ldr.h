#ifndef DMOD_LDR_H
#define DMOD_LDR_H

#ifndef DMOD_PRIVATE
#   error "This is private DMOD header. Don't include this outside DMOD library"
#endif

#include "dmod_types.h"

extern bool Dmod_Ldr_Load( Dmod_Context_t* Context );
extern bool Dmod_Ldr_LoadHeader( Dmod_Context_t* Context );
extern bool Dmod_Ldr_LoadFooter( Dmod_Context_t* Context );
extern bool Dmod_Ldr_LoadOutput( Dmod_Context_t* Context );
extern bool Dmod_Ldr_LoadInput( Dmod_Context_t* Context );
extern bool Dmod_Ldr_LoadGot( Dmod_Context_t* Context );
extern bool Dmod_Ldr_LoadBss( Dmod_Context_t* Context );

#endif // DMOD_LDR_H