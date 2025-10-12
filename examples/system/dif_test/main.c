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
    if( !Dmod_LoadModuleByName( "difs" ) )
    {
        printf("ERROR: Cannot load DIFS interface module!\n");
        return -1;
    }
    printf("DIFS interface module loaded successfully!\n\n");

    // Load FatFS implementation
    printf("Loading FatFS implementation module...\n");
    if( !Dmod_LoadModuleByName( "fatfs" ) )
    {
        printf("ERROR: Cannot load FatFS module!\n");
        return -1;
    }
    printf("FatFS module loaded successfully!\n\n");

    // Load FlashFS implementation
    printf("Loading FlashFS implementation module...\n");
    if( !Dmod_LoadModuleByName( "flashfs" ) )
    {
        printf("ERROR: Cannot load FlashFS module!\n");
        return -1;
    }
    printf("FlashFS module loaded successfully!\n\n");

    // Load VFS module that will use the DIF implementations
    printf("Loading VFS module...\n");
    if( !Dmod_LoadModuleByName( "vfs" ) )
    {
        printf("ERROR: Cannot load VFS module!\n");
        return -1;
    }
    printf("VFS module loaded successfully!\n\n");

    printf("\n========================================\n");
    printf("  All modules loaded successfully!\n");
    printf("========================================\n\n");

    // Cleanup
    printf("Cleaning up...\n");
    Dmod_DisableModule( "vfs", false );
    Dmod_UnloadModule( "vfs", false );
    
    Dmod_DisableModule( "flashfs", false );
    Dmod_UnloadModule( "flashfs", false );
    
    Dmod_DisableModule( "fatfs", false );
    Dmod_UnloadModule( "fatfs", false );
    
    Dmod_DisableModule( "difs", false );
    Dmod_UnloadModule( "difs", false );

    printf("\nDIF test completed successfully!\n\n");

    return 0;
}
