#include "dmod.h"
#include <stdio.h>

extern Dmod_ModuleHeader_t ModuleHeader;

extern void HelloWorld();

int main(int argc, char** argv)
{
    Dmod_Printf("Hello from module: %s\n", ModuleHeader.Name);
    Dmod_Printf("Version: %s\n", ModuleHeader.Version);
    Dmod_Printf("Author: %s\n", ModuleHeader.Author);
    Dmod_Printf("Arch: %s\n", ModuleHeader.Arch);

    HelloWorld();

    return 0;
}