/**
 * @file main.c
 * @brief whereisdmf - Find module file path by module name
 * 
 * This tool uses the Dmod_FindModuleFile API to locate a module file
 * and prints its full path.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dmod.h"

// -----------------------------------------
//
//      Prints usage message
//
// -----------------------------------------
void PrintUsage( const char* AppName )
{
    printf("Usage: %s <module_name> [arch_name]\n", AppName);
}

// -----------------------------------------
//
//      Prints help message
//
// -----------------------------------------
void PrintHelp( const char* AppName )
{
    printf("-- whereisdmf - Module File Locator ver. " DMOD_VERSION_STRING " --\n\n");
    printf("This tool finds and prints the full path to a module file.\n");
    printf("It searches in the configured DMOD repository directories.\n\n");
    PrintUsage( AppName );
    printf("\nArguments:\n");
    printf("  <module_name>    Name of the module to find (without .dmf/.dmfc extension)\n");
    printf("  [arch_name]      (optional) Architecture name (default: current system architecture)\n");
    printf("\nOptions:\n");
    printf("  -h, --help       Print this help message\n");
    printf("  -v, --version    Print version information\n");
    printf("\nExamples:\n");
    printf("  %s mymodule                    # Find mymodule for current architecture\n", AppName);
    printf("  %s mymodule x86_64             # Find mymodule for x86_64 architecture\n", AppName);
    printf("  %s mymodule armv7-cortex-m7    # Find mymodule for armv7-cortex-m7\n", AppName);
    printf("\nExit Codes:\n");
    printf("  0    Module found successfully\n");
    printf("  1    Module not found or error occurred\n");
}

// -----------------------------------------
//
//      Main function
//
// -----------------------------------------
int main( int argc, char *argv[] )
{
    if( argc < 2 )
    {
        PrintUsage( argv[0] );
        return 1;
    }

    if( strcmp( argv[1], "-h" ) == 0 || strcmp( argv[1], "--help" ) == 0 )
    {
        PrintHelp( argv[0] );
        return 0;
    }

    if( strcmp( argv[1], "-v" ) == 0 || strcmp( argv[1], "--version" ) == 0 )
    {
        printf("whereisdmf ver. " DMOD_VERSION_STRING "\n");
        return 0;
    }

    if( argc > 3 )
    {
        printf("Error: Too many arguments\n");
        PrintUsage( argv[0] );
        return 1;
    }

    const char* moduleName = argv[1];
    const char* archName = NULL;

    if( argc >= 3 )
    {
        archName = argv[2];
    }

    // Initialize Dmod system
    if (!Dmod_Initialize())
    {
        DMOD_LOG_ERROR("Failed to initialize Dmod system\n");
        return 1;
    }

    // Enable crossplatform mode to allow loading modules without execution
    Dmod_SetCrossplatformMode( true );

    // Buffer for the output file path
    char filePath[512];

    // Try to find the module file
    bool found = Dmod_FindModuleFile( moduleName, archName, filePath, sizeof(filePath) );

    // Clean up
    Dmod_Deinitialize();

    if( found )
    {
        Dmod_Printf("%s\n", filePath);
        return 0;
    }
    else
    {
        DMOD_LOG_ERROR("Module '%s' not found", moduleName);
        if( archName != NULL )
        {
            DMOD_LOG_ERROR(" for architecture '%s'", archName);
        }
        DMOD_LOG_ERROR("\n");
        return 1;
    }
}
