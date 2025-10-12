#define DMOD_ENABLE_REGISTRATION    ON
#ifndef DMOD_fatfs
#   define DMOD_fatfs
#endif

#include "dmod.h"
#include "difs.h"

/**
 * @brief FatFS implementation of DIFS interface
 */

// Implement _fopen for FatFS
dmod_difs_dif_api_declaration( 1.0, FatFS, int, _fopen, (void** fp, const char* path, int mode, int attr) )
{
    Dmod_Printf("FatFS: Opening file '%s' with mode %d, attr %d\n", path, mode, attr);
    // Dummy implementation - just allocate some memory to simulate file handle
    *fp = Dmod_Malloc(32);
    return 0; // Success
}

// Implement _fclose for FatFS
dmod_difs_dif_api_declaration( 1.0, FatFS, int, _fclose, (void* fp) )
{
    Dmod_Printf("FatFS: Closing file\n");
    Dmod_Free(fp);
    return 0; // Success
}

// Implement _fread for FatFS
dmod_difs_dif_api_declaration( 1.0, FatFS, int, _fread, (void* fp, void* buffer, size_t size, size_t* read) )
{
    Dmod_Printf("FatFS: Reading %zu bytes\n", size);
    *read = size; // Pretend we read all requested bytes
    return 0; // Success
}

// Implement _fwrite for FatFS
dmod_difs_dif_api_declaration( 1.0, FatFS, int, _fwrite, (void* fp, const void* buffer, size_t size, size_t* written) )
{
    Dmod_Printf("FatFS: Writing %zu bytes\n", size);
    *written = size; // Pretend we wrote all bytes
    return 0; // Success
}

int dmod_init(const Dmod_Config_t *Config)
{
    Dmod_Printf("FatFS module initialized\n");
    return 0;
}

int dmod_deinit(void)
{
    Dmod_Printf("FatFS module deinitialized\n");
    return 0;
}
