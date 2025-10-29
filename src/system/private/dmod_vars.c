#define DMOD_PRIVATE
#include "dmod.h"
#include "private/dmod_vars.h"

Dmod_Api_t Dmod_BuiltinInputApi = {
    .InputSection    = (void*)&__dmod_inputs_start,
    .SectionSize     = (size_t)&__dmod_inputs_size,
    .ApiType         = Dmod_ApiType_Input
};
Dmod_Api_t Dmod_BuiltinOutputApi = {
    .OutputSection    = (void*)&__dmod_outputs_start,
    .SectionSize     = (size_t)&__dmod_outputs_size,
    .ApiType         = Dmod_ApiType_Output
};
Dmod_Context_t* Dmod_Contexts[DMOD_MAX_MODULES] = {0};
Dmod_PackageSlot_t Dmod_Packages[DMOD_MAX_NUMBER_OF_PACKAGES] = {0};