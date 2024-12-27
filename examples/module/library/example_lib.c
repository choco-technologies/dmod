#include "example_lib.h"

void example_lib_foo(const char* str)
{
    Dmod_Printf("Foo: %s\n", str);
}

int dmod_init(const Dmod_Config_t *Config)
{
    Dmod_Printf("Init\n");
    return 0;
}

int dmod_deinit(void)
{
    Dmod_Printf("Deinit\n");
    return 0;
}