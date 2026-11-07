#define DMOD_ENABLE_REGISTRATION ON
#include "dmod.h"
#include "library.h"
#include <string.h>

#ifdef DMOD_EXPECTED_VERSION
_Static_assert(dmod_VERSION_MAJOR == DMOD_EXPECTED_MAJOR, "SDK major version mismatch");
_Static_assert(dmod_VERSION_MINOR == DMOD_EXPECTED_MINOR, "SDK minor version mismatch");
_Static_assert(dmod_VERSION_PATCH == DMOD_EXPECTED_PATCH, "SDK patch version mismatch");
#endif

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
#ifdef DMOD_EXPECTED_VERSION
    if (strcmp(DMOD_VERSION_STRING, DMOD_EXPECTED_VERSION) != 0)
        return 1;
#endif
    if (sdk_math_add(20, 22) != 42)
        return 1;
    Dmod_Printf("SDK application passed\n");
    return 0;
}
