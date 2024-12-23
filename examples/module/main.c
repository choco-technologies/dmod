#include "dmod.h"
#include <stdio.h>

extern Dmod_ModuleHeader_t ModuleHeader;

int main(int argc, char** argv)
{
    Dmod_Printf("Hello from module: %s\n", ModuleHeader.Name);
    Dmod_Printf("Version: 0x%08X\n", ModuleHeader.Version);
    return 0;
}