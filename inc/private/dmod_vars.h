#ifndef DMOD_VARS_H
#define DMOD_VARS_H
#ifdef __cplusplus
extern "C" {
#endif
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
extern Dmod_PackageSlot_t Dmod_Packages[DMOD_MAX_NUMBER_OF_PACKAGES];
extern bool Dmod_SystemCrossplatformMode;
extern Dmod_LogLevel_t Dmod_LogLevel;
extern Dmod_ModuleLogLevel_t* Dmod_ModuleLogLevels;

#ifdef __cplusplus
}
#endif
#endif // DMOD_VARS_H