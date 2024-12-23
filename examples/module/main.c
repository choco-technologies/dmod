#include "dmod.h"
#include <stdio.h>

extern Dmod_ModuleHeader_t ModuleHeader;

int main(int argc, char** argv)
{
    Dmod_Printf("Hello from module: %s\n", ModuleHeader.Name);
    Dmod_Printf("Version: %s\n", ModuleHeader.Version);
    return 0;
}