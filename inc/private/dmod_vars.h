#ifndef DMOD_VARS_H
#define DMOD_VARS_H

#ifndef DMOD_PRIVATE
#   error "This is private DMOD header. Don't include this outside DMOD library"
#endif

#include "dmod_types.h"

extern void* __dmod_inputs_start;
extern void* __dmod_inputs_end;
extern void* __dmod_inputs_size;
extern void* __dmod_outputs_start;
extern void* __dmod_outputs_end;
extern void* __dmod_outputs_size;
extern Dmod_Api_t Dmod_BuiltinInputApi;
extern Dmod_Api_t Dmod_BuiltinOutputApi;
extern Dmod_Context_t* Dmod_Contexts[DMOD_MAX_MODULES];

#endif // DMOD_VARS_H