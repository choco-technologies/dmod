#include "dmodex.h"

void dmodex_foo(const char* str)
{
    Dmod_Printf("Foo: %s\n", str);
    dmodex_bar("Hello from example_lib - bar");
    _example("Hello from example_lib - example");
}

void _print(const char* str)
{
    Dmod_Printf("Print: %s\n", str);
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