#define ENABLE_DIF_REGISTRATIONS ON
#include "difs.h"
#include <stdio.h>
#include "dmod.h"
#include "dmod_system.h"

/**
 * @brief Test if DIF functions can be accessed directly from system level
 * 
 * This test loads modules implementing DIF and tries to call their functions
 * directly without going through the VFS abstraction.
 */
int main( int argc, char *argv[] )
{
    // Initialize Dmod system
    if (!Dmod_Initialize(0, 0))
    {
        printf("Error: Failed to initialize Dmod system\n");
        return -1;
    }

    printf("\n=== Testing Direct DIF Access from System ===\n\n");

    // Load modules
    printf("Loading DIFS, FatFS, and FlashFS modules...\n");
    if( !Dmod_LoadModuleByName( "difs" ) || 
        !Dmod_LoadModuleByName( "fatfs" ) || 
        !Dmod_LoadModuleByName( "flashfs" ) )
    {
        printf("ERROR: Cannot load modules!\n");
        return -1;
    }
    printf("Modules loaded successfully.\n\n");

    // Enable modules (required for DIF discovery!)
    if( !Dmod_EnableModule( "difs", false, NULL ) ||
        !Dmod_EnableModule( "fatfs", false, NULL ) ||
        !Dmod_EnableModule( "flashfs", false, NULL ) )
    {
        printf("ERROR: Cannot enable modules!\n");
        return -1;
    }
    printf("Modules loaded and enabled successfully.\n\n");

    // Try to get DIF implementations directly from system
    printf("Testing direct access to DIF implementations:\n\n");

    // Test: Get first DIF module implementing fopen
    // Using auto-generated _sig signature from difs module
    
    printf("Attempting to discover FatFS implementation...\n");
    Dmod_Context_t* fatfs_ctx = Dmod_GetNextDifModule( 
        dmod_difs_fopen_sig,  // Use auto-generated _sig signature
        NULL 
    );
    
    if( fatfs_ctx != NULL )
    {
        printf("✓ Successfully discovered first implementation\n");
        
        // Get the function pointer
        void* fopen_func = Dmod_GetDifFunction( fatfs_ctx, dmod_difs_fopen_sig );
        if( fopen_func != NULL )
        {
            printf("✓ Successfully retrieved function pointer from system\n");
            printf("  Function pointer: %p\n", fopen_func);
        }
        else
        {
            printf("✗ Failed to get function pointer\n");
        }
        
        // Try to get second implementation
        printf("\nAttempting to discover FlashFS implementation...\n");
        Dmod_Context_t* flashfs_ctx = Dmod_GetNextDifModule( 
            dmod_difs_fopen_sig,
            fatfs_ctx 
        );
        
        if( flashfs_ctx != NULL )
        {
            printf("✓ Successfully discovered second implementation\n");
            
            void* fopen_func2 = Dmod_GetDifFunction( flashfs_ctx, dmod_difs_fopen_sig );
            if( fopen_func2 != NULL )
            {
                printf("✓ Successfully retrieved second function pointer\n");
                printf("  Function pointer: %p\n", fopen_func2);
            }
        }
        else
        {
            printf("✗ Failed to discover second implementation\n");
        }
    }
    else
    {
        printf("✗ Failed to discover any implementation\n");
        printf("  This means DIF functions are NOT accessible from system level!\n");
    }

    // Cleanup
    printf("\nCleaning up...\n");
    Dmod_DisableModule( "flashfs", false );
    Dmod_UnloadModule( "flashfs", false );
    Dmod_DisableModule( "fatfs", false );
    Dmod_UnloadModule( "fatfs", false );
    Dmod_DisableModule( "difs", false );
    Dmod_UnloadModule( "difs", false );

    printf("\n=== Test Complete ===\n\n");
    return 0;
}
