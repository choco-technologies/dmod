#define DMOD_ENABLE_REGISTRATION ON
#include "dmod.h"
#include "library.h"

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    if (sdk_math_add(20, 22) != 42)
        return 1;
    Dmod_Printf("SDK application passed\n");
    return 0;
}
