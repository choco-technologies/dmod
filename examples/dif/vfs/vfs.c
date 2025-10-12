#include "dmod.h"
#include "difs.h"

/**
 * @brief VFS - Virtual File System that uses DIFS interface
 * 
 * This module demonstrates how to discover and use modules that implement
 * the DIFS interface.
 */

int dmod_init(const Dmod_Config_t *Config)
{
    Dmod_Printf("\n=== VFS Demo - Using DIF Interfaces ===\n\n");
    
    // Iterate through all modules that implement DIFS _fopen
    Dmod_Printf("Looking for file systems implementing DIFS interface...\n\n");
    
    Dmod_Context_t* fs = Dmod_GetNextDifModule( dmod_difs_fopen, NULL );
    
    int fs_count = 0;
    while(fs != NULL)
    {
        fs_count++;
        Dmod_Printf("Found file system #%d\n", fs_count);
        
        // Get the function pointer for _fopen from this module
        dmod_difs_fopen_t fopen_func = (dmod_difs_fopen_t)Dmod_GetDifFunction( fs, dmod_difs_fopen );
        dmod_difs_fclose_t fclose_func = (dmod_difs_fclose_t)Dmod_GetDifFunction( fs, dmod_difs_fclose );
        dmod_difs_fread_t fread_func = (dmod_difs_fread_t)Dmod_GetDifFunction( fs, dmod_difs_fread );
        dmod_difs_fwrite_t fwrite_func = (dmod_difs_fwrite_t)Dmod_GetDifFunction( fs, dmod_difs_fwrite );
        
        if( fopen_func != NULL && fclose_func != NULL && fread_func != NULL && fwrite_func != NULL )
        {
            // Test the file system
            void* file_handle = NULL;
            int result = fopen_func( &file_handle, "test.txt", 1, 0 );
            
            if( result == 0 )
            {
                // Write some data
                size_t written = 0;
                const char* data = "Hello from VFS!";
                fwrite_func( file_handle, data, 16, &written );
                Dmod_Printf("  Wrote %zu bytes\n", written);
                
                // Read some data
                size_t read = 0;
                char buffer[32];
                fread_func( file_handle, buffer, 16, &read );
                Dmod_Printf("  Read %zu bytes\n", read);
                
                // Close the file
                fclose_func( file_handle );
                Dmod_Printf("  File closed successfully\n");
            }
        }
        
        Dmod_Printf("\n");
        
        // Get next file system
        fs = Dmod_GetNextDifModule( dmod_difs_fopen, fs );
    }
    
    if( fs_count == 0 )
    {
        Dmod_Printf("No file systems found implementing DIFS interface!\n");
    }
    else
    {
        Dmod_Printf("Total file systems found: %d\n", fs_count);
    }
    
    Dmod_Printf("\n=== VFS Demo Complete ===\n\n");
    
    return 0;
}

int dmod_deinit(void)
{
    Dmod_Printf("VFS module deinitialized\n");
    return 0;
}
