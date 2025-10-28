#include "dmod.h"

#ifndef DMOD_MODULE_NAME
#   define DMOD_MODULE_NAME "<Unnamed module>"
#endif

#ifndef DMOD_AUTHOR_NAME
#   define DMOD_AUTHOR_NAME "<Unknown author>"
#endif

#ifndef DMOD_MODULE_VERSION
#   define DMOD_MODULE_VERSION "0.0"
#endif

#ifndef DMOD_MODULE_TYPE
#   define DMOD_MODULE_TYPE Unknown
#endif

#ifndef DMOD_STACK_SIZE
#   define DMOD_STACK_SIZE 1024
#endif

#ifndef DMOD_PRIORITY
#   define DMOD_PRIORITY 0
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
    .Name               = "vfs",
    .Author             = "DMOD Team",
    .Version            = "1.0",
    .Preinit            = dmod_preinit,
    .Init               = dmod_init,
    .Main               = main,
    .Deinit             = dmod_deinit,
    .Signal             = dmod_signal,
    .RequiredStackSize  = 2048,
    .Priority           = 1,
    .ModuleType         = Dmod_ModuleType_Library,
    .License            = &License,
    .Footer             = &__footer_start,
    .ManualLoad         = OFF,
};

volatile const Dmod_ModuleHeader_t* DMOD_Header DMOD_GLOBAL_POINTER = &ModuleHeader;
