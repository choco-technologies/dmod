#include "dmod.h"
#include "example_lib.h"
#include <stdio.h>

extern const Dmod_ModuleHeader_t* DMOD_Header;
extern void HelloWorld();

/**
 * @brief example MAL function
 * 
 * @param str String to print
 * 
 * This is an example of a function, that is the Module Abstraction Layer (MAL) function 
 * for the example library. You can use this function to implement the functions required 
 * by the library (it's output functions). 
 */
void mal_example_lib_bar(const char* str)
{
    Dmod_Printf("Bar: %s\n", str);
}

int main(int argc, char** argv)
{
    Dmod_Printf("Hello from module: %s\n", DMOD_Header->Name);
    Dmod_Printf("Version: %s\n", DMOD_Header->Version);
    Dmod_Printf("Author: %s\n", DMOD_Header->Author);
    Dmod_Printf("Arch: %s\n", DMOD_Header->Arch);

    HelloWorld();

    example_lib_foo("Hello from main");

    _print("Hello from main using _print");

    return 0;
}