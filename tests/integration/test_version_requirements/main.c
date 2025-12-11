/**
 * Test module for version requirements feature
 * This module uses dmod_link_modules with version specifications
 */

#define DMOD_ENABLE_REGISTRATION ON
#include "dmod.h"
#include "test_version_reqs_defs.h"

int main(int argc, char** argv)
{
    Dmod_Printf("Test module with version requirements\n");
    return 0;
}
