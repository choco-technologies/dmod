#ifndef DMOD_HLP_H
#define DMOD_HLP_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DMOD_PRIVATE
#   error "This is private DMOD header. Don't include this outside DMOD library"
#endif

#include "dmod_types.h"

extern bool Dmod_Hlp_InitPointer( Dmod_Context_t* Context, void** PointerRef, const char* PointerName );

#ifdef __cplusplus
}
#endif
#endif // DMOD_HLP_H