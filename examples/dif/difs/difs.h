#ifndef DIFS_H
#define DIFS_H

#include "dmod.h"
#include "difs_defs.h"

/**
 * @brief DIFS - Dmod Interface for File System
 * 
 * This is an example DIF (Dmod Interface) that defines file system operations.
 * Multiple modules can implement these interfaces (e.g., FatFS, FlashFS).
 */

// Define DIF signature strings as macros so they're compile-time constants
#define dmod_difs_fopen_sig   DMOD_MAKE_DIF_SIGNATURE( difs, 1.0, _fopen )
#define dmod_difs_fclose_sig  DMOD_MAKE_DIF_SIGNATURE( difs, 1.0, _fclose )
#define dmod_difs_fread_sig   DMOD_MAKE_DIF_SIGNATURE( difs, 1.0, _fread )
#define dmod_difs_fwrite_sig  DMOD_MAKE_DIF_SIGNATURE( difs, 1.0, _fwrite )

// Define DIF signatures that will be used to identify implementations
dmod_difs_dif( 1.0, int, _fopen, (void** fp, const char* path, int mode, int attr) );
dmod_difs_dif( 1.0, int, _fclose, (void* fp) );
dmod_difs_dif( 1.0, int, _fread, (void* fp, void* buffer, size_t size, size_t* read) );
dmod_difs_dif( 1.0, int, _fwrite, (void* fp, const void* buffer, size_t size, size_t* written) );

#endif // DIFS_H
