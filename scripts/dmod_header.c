#include "dmod.h"

#ifndef DMOD_MODULE_NAME
#   define DMOD_MODULE_NAME "<Unnamed module>"
#   error "DMOD_MODULE_NAME is not defined"
#endif

#ifndef DMOD_AUTHOR_NAME
#   define DMOD_AUTHOR_NAME "<Unknown author>"
#   warning "DMOD_AUTHOR_NAME is not defined"
#endif

#ifndef DMOD_MODULE_VERSION
#   define DMOD_MODULE_VERSION "0.0"
#   error "Module version is not defined"
#endif

#ifndef DMOD_MODULE_TYPE
#   define DMOD_MODULE_TYPE Dmod_ModuleType_Unknown
#endif

#ifndef DMOD_STACK_SIZE
#   define DMOD_STACK_SIZE 1024
#endif

#ifndef DMOD_PRIORITY
#   define DMOD_PRIORITY 0
#endif

#ifndef DMOD_MANUAL_LOAD
#   define DMOD_MANUAL_LOAD false
#endif

extern void DMOD_WEAK_SYMBOL dmod_preinit(void);
extern int  DMOD_WEAK_SYMBOL dmod_init(const Dmod_Config_t *Config);
extern int  DMOD_WEAK_SYMBOL main(int argc, char** argv);
extern int  DMOD_WEAK_SYMBOL dmod_deinit(void);
extern int  DMOD_WEAK_SYMBOL dmod_signal( int SignalNumber );

extern Dmod_License_t DMOD_WEAK_SYMBOL License;
extern void* __footer_start;

volatile const Dmod_ModuleHeader_t ModuleHeader DMOD_SECTION(".header") DMOD_USED = 
{
    .Signature          = DMOD_HEADER_SIGNATURE,
    .DmodVersion        = DMOD_VERSION,
    .Arch               = DMOD_ARCH,
    .CpuName            = DMOD_CPU_NAME,
    .Name               = DMOD_MODULE_NAME,
    .Author             = DMOD_AUTHOR_NAME,
    .Version            = DMOD_MODULE_VERSION,
    .Preinit            = (uint64_t)(uintptr_t)dmod_preinit,
    .Init               = (uint64_t)(uintptr_t)dmod_init,
    .Main               = (uint64_t)(uintptr_t)main,
    .Deinit             = (uint64_t)(uintptr_t)dmod_deinit,
    .Signal             = (uint64_t)(uintptr_t)dmod_signal,
    .RequiredStackSize  = DMOD_STACK_SIZE,
    .Priority           = DMOD_PRIORITY,
    .ModuleType         = DMOD_MODULE_TYPE,
    .License            = (uint64_t)(uintptr_t)&License,
    .Footer             = &__footer_start,
    .ManualLoad         = DMOD_MANUAL_LOAD,
};

volatile const Dmod_ModuleHeader_t* DMOD_Header DMOD_GLOBAL_POINTER = &ModuleHeader;