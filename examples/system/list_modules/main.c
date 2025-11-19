#include <stdio.h>
#include <string.h>
#include "dmod.h"

/**
 * @brief Simple test application to list all modules
 * 
 * This application demonstrates the usage of Dmod_OpenModules/ReadModule/CloseModules API
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

    // Open modules iterator
    Dmod_ModulesIterator_t iterator = Dmod_OpenModules();
    if( iterator == NULL )
    {
        printf("Error: Failed to open modules iterator\n");
        return -1;
    }

    printf("Listing modules:\n\n");
    printf("%-30s %-15s %-15s\n", "Module Name", "Version", "State");
    printf("%-30s %-15s %-15s\n", "--------------------------------", "---------------", "---------------");

    // Iterate through modules
    size_t count = 0;
    const Dmod_ModuleInfo_t* module;
    while( (module = Dmod_ReadModule( iterator )) != NULL )
    {
        const char* stateStr = "Unknown";
        switch( module->State )
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
               module->ModuleName,
               module->Version[0] != '\0' ? module->Version : "N/A",
               stateStr);
        count++;
    }

    // Close iterator
    Dmod_CloseModules( iterator );

    if( count == 0 )
    {
        printf("\nNo modules found. Try:\n");
        printf("  - Setting DMOD_REPO_PATHS environment variable to a directory with .dmf/.dmfc files\n");
        printf("  - Running with module name arguments: %s <module_name>\n", argv[0]);
    }
    else
    {
        printf("\nTotal modules found: %zu\n", count);
    }

    printf("\n");
    return 0;
}
