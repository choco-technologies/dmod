#include <stdio.h>
#include <string.h>
#include "dmod.h"

/**
 * @brief Test application for DIF (Dmod Interface) functionality
 * 
 * This application demonstrates loading multiple modules that implement
 * the same DIF interface and using them dynamically.
 */

int main( int argc, char *argv[] )
{
    printf("\n");
    printf("========================================\n");
    printf("  DIF (Dmod Interface) Test Application\n");
    printf("========================================\n\n");

    // Load the DIFS interface module first
    printf("Loading DIFS interface module...\n");
    Dmod_Context_t* difs = Dmod_LoadFile( "build-modules/dmf/difs.dmf" );
    if( difs == NULL )
    {
        printf("ERROR: Cannot load DIFS interface module!\n");
        return -1;
    }
    
    // Initialize the DIFS module
    if( !Dmod_Enable( difs, false, NULL ) )
    {
        printf("ERROR: Cannot enable DIFS module!\n");
        return -1;
    }
    printf("DIFS interface module loaded successfully!\n\n");

    // Load FatFS implementation
    printf("Loading FatFS implementation module...\n");
    Dmod_Context_t* fatfs = Dmod_LoadFile( "build-modules/dmf/fatfs.dmf" );
    if( fatfs == NULL )
    {
        printf("ERROR: Cannot load FatFS module!\n");
        return -1;
    }
    
    if( !Dmod_Enable( fatfs, false, NULL ) )
    {
        printf("ERROR: Cannot enable FatFS module!\n");
        return -1;
    }
    printf("FatFS module loaded successfully!\n\n");

    // Load FlashFS implementation
    printf("Loading FlashFS implementation module...\n");
    Dmod_Context_t* flashfs = Dmod_LoadFile( "build-modules/dmf/flashfs.dmf" );
    if( flashfs == NULL )
    {
        printf("ERROR: Cannot load FlashFS module!\n");
        return -1;
    }
    
    if( !Dmod_Enable( flashfs, false, NULL ) )
    {
        printf("ERROR: Cannot enable FlashFS module!\n");
        return -1;
    }
    printf("FlashFS module loaded successfully!\n\n");

    // Load VFS module that will use the DIF implementations
    printf("Loading VFS module...\n");
    Dmod_Context_t* vfs = Dmod_LoadFile( "build-modules/dmf/vfs.dmf" );
    if( vfs == NULL )
    {
        printf("ERROR: Cannot load VFS module!\n");
        return -1;
    }
    
    if( !Dmod_Enable( vfs, false, NULL ) )
    {
        printf("ERROR: Cannot enable VFS module!\n");
        return -1;
    }
    printf("VFS module loaded successfully!\n\n");

    printf("\n========================================\n");
    printf("  All modules loaded successfully!\n");
    printf("========================================\n\n");

    // Cleanup
    printf("Cleaning up...\n");
    Dmod_Disable( vfs, false );
    Dmod_Unload( vfs, false );
    
    Dmod_Disable( flashfs, false );
    Dmod_Unload( flashfs, false );
    
    Dmod_Disable( fatfs, false );
    Dmod_Unload( fatfs, false );
    
    Dmod_Disable( difs, false );
    Dmod_Unload( difs, false );

    printf("\nDIF test completed successfully!\n\n");

    return 0;
}
