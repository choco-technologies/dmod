#ifndef INC_DMOD_H_
#define INC_DMOD_H_

#include "dmod_types.h"

#ifdef DMOD_MODULE
#   include "dmod_module.h"
#elif defined(DMOD_SYSTEM)
#   include "dmod_system.h"
#else 
#   error "DMOD_MODULE or DMOD_SYSTEM must be defined"
#endif

//==============================================================================
//                              FUNCTION PROTOTYPES
//==============================================================================

extern Dmod_Context_t Dmod_Load( const char* Path, const Dmod_MemIf_t* Memory, const Dmod_FileIf_t* File );
extern int Dmod_Unload( Dmod_Context_t Handle );

#endif /* INC_DMOD_H_ */
