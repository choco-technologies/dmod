/**
 * @file example_read_modules.c
 * @brief Example demonstrating the Dmod_ReadNextModule function
 * 
 * This example shows how to use Dmod_OpenModules, Dmod_ReadNextModule,
 * and Dmod_CloseModules to iterate through all available modules.
 */

#include <stdio.h>
#include "dmod.h"

int main(void)
{
    printf("=== Module Listing Example ===\n\n");
    
    // Initialize DMOD system
    if (!Dmod_Initialize())
    {
        fprintf(stderr, "Failed to initialize DMOD system\n");
        return 1;
    }
    
    // Allocate module node structure (user-allocated)
    Dmod_ModuleNode_t moduleNode;
    moduleNode._Data = NULL;
    
    // Open module iteration
    if (!Dmod_OpenModules(&moduleNode))
    {
        fprintf(stderr, "Failed to open module iteration\n");
        Dmod_Deinitialize();
        return 1;
    }
    
    int moduleCount = 0;
    
    printf("Available modules:\n");
    printf("%-40s %-32s %-16s\n", "Path", "Name", "Version");
    printf("--------------------------------------------------------------------------------\n");
    
    // Iterate through all available modules
    while (Dmod_ReadNextModule(&moduleNode))
    {
        printf("%-40s %-32s %-16s\n", 
               moduleNode.path, 
               moduleNode.header.Name, 
               moduleNode.header.Version);
        moduleCount++;
    }
    
    printf("--------------------------------------------------------------------------------\n");
    printf("Total modules found: %d\n", moduleCount);
    
    // Close module iteration (free resources)
    Dmod_CloseModules(&moduleNode);
    
    // Cleanup DMOD system
    Dmod_Deinitialize();
    
    return 0;
}
