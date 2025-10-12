/**
 * @brief DIFS interface registration file
 * 
 * This file is responsible for registering the DIF signatures.
 * It must enable registration before including the header.
 */

#define DMOD_ENABLE_REGISTRATION    ON
#ifndef DMOD_difs
#   define DMOD_difs
#endif

#include "difs.h"

// Define the DIF signature strings
const char* dmod_difs_fopen = DMOD_MAKE_DIF_SIGNATURE( "DIFS", 1.0, "_fopen" );
const char* dmod_difs_fclose = DMOD_MAKE_DIF_SIGNATURE( "DIFS", 1.0, "_fclose" );
const char* dmod_difs_fread = DMOD_MAKE_DIF_SIGNATURE( "DIFS", 1.0, "_fread" );
const char* dmod_difs_fwrite = DMOD_MAKE_DIF_SIGNATURE( "DIFS", 1.0, "_fwrite" );

// This module doesn't have init/deinit since it's just an interface definition
int dmod_init(const Dmod_Config_t *Config)
{
    Dmod_Printf("DIFS interface module initialized\n");
    return 0;
}

int dmod_deinit(void)
{
    Dmod_Printf("DIFS interface module deinitialized\n");
    return 0;
}
