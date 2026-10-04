#define DMOD_ENABLE_REGISTRATION ON
#include "dmod.h"
#include "library.h"

int dmod_init(const Dmod_Config_t *config)
{
    (void)config;
    Dmod_Printf("SDK library initialized\n");
    return 0;
}

int dmod_deinit(void)
{
    return 0;
}

dmod_sdk_math_api_declaration(0.1, int, _add, (int a, int b))
{
    return a + b;
}
