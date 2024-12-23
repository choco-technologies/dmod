#include "dmod.h"

extern int DMOD_WEAK_SYMBOL main(int argc, char** argv);

extern void* __footer_start;

volatile const Dmod_ModuleHeader_t ModuleHeader DMOD_SECTION(".header") DMOD_USED = 
{
    .Signature = DMOD_HEADER_SIGNATURE,
    .Version = DMOD_VERSION,
    .Arch = DMOD_ARCH,
    .Name = "MyModule",
    .Company = "Company",
    .Preinit = NULL,
    .Init = NULL,
    .Main = main,
    .Deinit = NULL,
    .Signal = NULL,
    .RequiredStackSize = 1024,
    .Priority = 1,
    .ModuleType = Dmod_ModuleType_Application,
    .License = NULL,
    .Footer = &__footer_start
};