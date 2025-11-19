#include <stdio.h>
#include <string.h>
#include "dmod.h"

/**
 * @brief Simple test application to list all modules
 * 
 * This application demonstrates the usage of Dmod_ReadModules API
 */
int main( int argc, char *argv[] )
{
    // Initialize Dmod system
    if (!Dmod_Initialize())
    {
        printf("Error: Failed to initialize Dmod system\n");
        return -1;
    }

    printf("=== DMOD Module Lister ===\n\n");

    // If a module name is provided, try to load it first
    if( argc > 1 )
    {
        for( int i = 1; i < argc; i++ )
        {
            printf("Loading module: %s\n", argv[i]);
            if( Dmod_LoadModuleByName( argv[i] ) )
            {
                printf("  Successfully loaded module: %s\n", argv[i]);
            }
            else
            {
                printf("  Failed to load module: %s\n", argv[i]);
            }
        }
        printf("\n");
    }

    // Allocate array for module info
    Dmod_ModuleInfo_t modules[100];
    size_t count = Dmod_ReadModules( modules, 100 );

    printf("Found %zu modules:\n\n", count);

    if( count > 0 )
    {
        // Print header
        printf("%-30s %-15s %-15s\n", "Module Name", "Version", "State");
        printf("%-30s %-15s %-15s\n", "--------------------------------", "---------------", "---------------");

        // Print each module
        for( size_t i = 0; i < count; i++ )
        {
            const char* stateStr = "Unknown";
            switch( modules[i].State )
            {
                case Dmod_ModuleState_Available:
                    stateStr = "Available";
                    break;
                case Dmod_ModuleState_Loaded:
                    stateStr = "Loaded";
                    break;
                case Dmod_ModuleState_Enabled:
                    stateStr = "Enabled";
                    break;
                case Dmod_ModuleState_Running:
                    stateStr = "Running";
                    break;
                default:
                    stateStr = "Unknown";
                    break;
            }

            printf("%-30s %-15s %-15s\n", 
                   modules[i].ModuleName,
                   modules[i].Version[0] != '\0' ? modules[i].Version : "N/A",
                   stateStr);
        }
    }
    else
    {
        printf("No modules found. Try:\n");
        printf("  - Setting DMOD_REPO_PATHS environment variable to a directory with .dmf/.dmfc files\n");
        printf("  - Running with module name arguments: %s <module_name>\n", argv[0]);
    }

    printf("\n");
    return 0;
}
