#define DMOD_ENABLE_REGISTRATION    ON
#ifndef DMOD_flashfs
#   define DMOD_flashfs
#endif

#include "dmod.h"
#include "difs.h"

/**
 * @brief FlashFS implementation of DIFS interface
 */

// Implement _fopen for FlashFS
dmod_difs_dif_api_declaration( 1.0, FlashFS, int, _fopen, (void** fp, const char* path, int mode, int attr) )
{
    Dmod_Printf("FlashFS: Opening file '%s' with mode %d, attr %d\n", path, mode, attr);
    // Dummy implementation - just allocate some memory to simulate file handle
    *fp = Dmod_Malloc(32);
    return 0; // Success
}

// Implement _fclose for FlashFS
dmod_difs_dif_api_declaration( 1.0, FlashFS, int, _fclose, (void* fp) )
{
    Dmod_Printf("FlashFS: Closing file\n");
    Dmod_Free(fp);
    return 0; // Success
}

// Implement _fread for FlashFS
dmod_difs_dif_api_declaration( 1.0, FlashFS, int, _fread, (void* fp, void* buffer, size_t size, size_t* read) )
{
    Dmod_Printf("FlashFS: Reading %zu bytes\n", size);
    *read = size; // Pretend we read all requested bytes
    return 0; // Success
}

// Implement _fwrite for FlashFS
dmod_difs_dif_api_declaration( 1.0, FlashFS, int, _fwrite, (void* fp, const void* buffer, size_t size, size_t* written) )
{
    Dmod_Printf("FlashFS: Writing %zu bytes\n", size);
    *written = size; // Pretend we wrote all bytes
    return 0; // Success
}

int dmod_init(const Dmod_Config_t *Config)
{
    Dmod_Printf("FlashFS module initialized\n");
    return 0;
}

int dmod_deinit(void)
{
    Dmod_Printf("FlashFS module deinitialized\n");
    return 0;
}
