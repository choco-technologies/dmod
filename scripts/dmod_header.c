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

typedef void (*func_ptr)(void);
extern func_ptr __init_array_start[];
extern func_ptr __init_array_end[];
extern func_ptr __fini_array_start[];
extern func_ptr __fini_array_end[];

/**
 * @brief Call functions from the init array
 * 
 * This function calls all the functions from the init array.
 */
static int _init(const Dmod_Config_t *Config) 
{
    Dmod_Printf("Initializing module: %s\n", DMOD_MODULE_NAME);
    for (func_ptr *func = __init_array_start; func < __init_array_end; ++func) 
    {
        if (*func)
        {
            (*func)();
        }
    }

    volatile void* init = dmod_init;
    Dmod_Printf("Constructor step has been finished. dmod_init: %p\n", init);

    int result = 0;
    if(init)
    {
        result = dmod_init(Config);
    }
    return result;
}

/**
 * @brief Call functions from the fini array
 * 
 * This function calls all the functions from the fini array.
 */
static int _fini(void)
{
    Dmod_Printf("Deinitializing module: %s\n", DMOD_MODULE_NAME);
    for (func_ptr *func = __fini_array_end - 1; func >= __fini_array_start; --func) 
    {
        if (*func) 
        {
            (*func)();
        }
    }

    volatile void* deinit = dmod_deinit;
    Dmod_Printf("Destructor step has been finished. dmod_deinit: %p\n", deinit);

    int result = 0;
    if(deinit)
    {
        result = dmod_deinit();
    }
    return result;
}

volatile const Dmod_ModuleHeader_t ModuleHeader DMOD_SECTION(".header") DMOD_USED = 
{
    .Signature          = DMOD_HEADER_SIGNATURE,
    .DmodVersion        = DMOD_VERSION,
    .Arch               = DMOD_ARCH,
    .CpuName            = DMOD_CPU_NAME,
    .Name               = DMOD_MODULE_NAME,
    .Author             = DMOD_AUTHOR_NAME,
    .Version            = DMOD_MODULE_VERSION,
    .Preinit            = dmod_preinit,
    .Init               = _init,
    .Main               = main,
    .Deinit             = _fini,
    .Signal             = dmod_signal,
    .RequiredStackSize  = DMOD_STACK_SIZE,
    .Priority           = DMOD_PRIORITY,
    .ModuleType         = DMOD_MODULE_TYPE,
    .License            = &License,
    .Footer             = &__footer_start,
    .ManualLoad         = DMOD_MANUAL_LOAD,
};

volatile const Dmod_ModuleHeader_t* DMOD_Header DMOD_GLOBAL_POINTER = &ModuleHeader;