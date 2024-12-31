#include "dmod.h"
#include "example_lib.h"
#include <stdio.h>

extern const Dmod_ModuleHeader_t* DMOD_Header;
extern void HelloWorld();

int main(int argc, char** argv)
{
    Dmod_Printf("Hello from module: %s\n", DMOD_Header->Name);
    Dmod_Printf("Version: %s\n", DMOD_Header->Version);
    Dmod_Printf("Author: %s\n", DMOD_Header->Author);
    Dmod_Printf("Arch: %s\n", DMOD_Header->Arch);

    HelloWorld();

    example_lib_foo("Hello from main");

    return 0;
}