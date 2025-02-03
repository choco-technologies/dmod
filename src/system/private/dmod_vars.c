#define DMOD_PRIVATE
#include "dmod.h"
#include "private/dmod_vars.h"

#ifdef DMOD_UNIT_TESTS
void* __dmod_inputs_start = 0;
void* __dmod_inputs_end = 0;
void* __dmod_inputs_size = 0;
void* __dmod_outputs_start = 0;
void* __dmod_outputs_end = 0;
void* __dmod_outputs_size = 0;
#endif 

Dmod_Api_t Dmod_BuiltinInputApi = {
    .InputSection    = (void*)&__dmod_inputs_start,
    .SectionSize     = (size_t)&__dmod_inputs_size,
    .ApiType         = Dmod_ApiType_Input
};
Dmod_Api_t Dmod_BuiltinOutputApi = {
    #ifndef DMOD_UNIT_TESTS
    .OutputSection    = (void*)&__dmod_outputs_start,
    .SectionSize     = (size_t)&__dmod_outputs_size,
    #endif
    .ApiType         = Dmod_ApiType_Output
};
Dmod_Context_t* Dmod_Contexts[DMOD_MAX_MODULES] = {0};
