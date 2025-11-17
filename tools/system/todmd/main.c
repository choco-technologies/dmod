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
    printf("Usage: %s path/to/file.dmf [output.dmd]\n", AppName);
}

// -----------------------------------------
//
//      Prints help message
//
// -----------------------------------------
void PrintHelp( const char* AppName )
{
    printf("-- DMD Dependencies Generator ver. " DMOD_VERSION_STRING " --\n\n");
    printf("This application reads a module's required dependencies and generates a .dmd file.\n");
    printf("The .dmd file can be used with dmf-get to download all required modules.\n\n");
    PrintUsage( AppName );
    printf("\nOptions:\n");
    printf("  -h, --help            Print this help message\n");
    printf("  -v, --version         Print version information\n");
    printf("\nArguments:\n");
    printf("  path/to/file.dmf      Path to the DMF module file\n");
    printf("  [output.dmd]          (optional) Output .dmd file path (default: module_name.dmd)\n");
    printf("\nDescription:\n");
    printf("  The tool loads a module in crossplatform mode, reads its dependencies,\n");
    printf("  and creates a .dmd file listing all non-system required modules.\n");
    printf("  System modules are automatically filtered out.\n");
    printf("\nExamples:\n");
    printf("  %s myapp.dmf                    # Creates myapp.dmd\n", AppName);
    printf("  %s myapp.dmf dependencies.dmd   # Creates dependencies.dmd\n", AppName);
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
        return 0;
    }

    if( strcmp( argv[1], "-h" ) == 0 || strcmp( argv[1], "--help" ) == 0 )
    {
        PrintHelp( argv[0] );
        return 0;
    }

    if( strcmp( argv[1], "-v" ) == 0 || strcmp( argv[1], "--version" ) == 0 )
    {
        printf("Dynamic Module Loader ver. " DMOD_VERSION_STRING "\n");
        return 0;
    }

    if( argc > 3 )
    {
        printf("Error: Too many arguments\n");
        PrintUsage( argv[0] );
        return -1;
    }

    const char* dmfPath = argv[1];
    const char* outputPath = NULL;
    char defaultOutputPath[256];

    // Determine output path
    if( argc >= 3 )
    {
        outputPath = argv[2];
    }
    else
    {
        // Extract module name from path and create default output name
        const char* lastSlash = strrchr( dmfPath, '/' );
        const char* fileName = lastSlash ? lastSlash + 1 : dmfPath;
        
        // Find the extension
        const char* lastDot = strrchr( fileName, '.' );
        size_t nameLen = lastDot ? (size_t)(lastDot - fileName) : strlen(fileName);
        
        // Create default name: module_name.dmd
        if( nameLen >= sizeof(defaultOutputPath) - 5 )
        {
            nameLen = sizeof(defaultOutputPath) - 5;
        }
        
        strncpy( defaultOutputPath, fileName, nameLen );
        defaultOutputPath[nameLen] = '\0';
        strcat( defaultOutputPath, ".dmd" );
        
        outputPath = defaultOutputPath;
    }

    printf("Reading module: %s\n", dmfPath);

    // Initialize Dmod system
    if (!Dmod_Initialize())
    {
        printf("Error: Failed to initialize Dmod system\n");
        return -1;
    }

    // Enable crossplatform mode to allow loading modules without execution
    Dmod_SetCrossplatformMode( true );

    // Load the module
    Dmod_Context_t* context = Dmod_LoadFile( dmfPath );
    if( context == NULL )
    {
        printf("Error: Cannot load module: %s\n", dmfPath);
        Dmod_Deinitialize();
        return -1;
    }

    const char* moduleName = Dmod_GetName( context );
    printf("Module name: %s\n", moduleName ? moduleName : "<unknown>");

    // Open output file
    FILE* outputFile = fopen( outputPath, "w" );
    if( outputFile == NULL )
    {
        printf("Error: Cannot create output file: %s\n", outputPath);
        Dmod_Unload( context, false );
        Dmod_Deinitialize();
        return -1;
    }

    // Write header comment to the .dmd file
    fprintf( outputFile, "# DMOD Dependencies File\n" );
    fprintf( outputFile, "# Generated from module: %s\n", moduleName ? moduleName : "<unknown>" );
    fprintf( outputFile, "# Source file: %s\n", dmfPath );
    fprintf( outputFile, "#\n");
    fprintf( outputFile, "# This file lists all non-system modules required by the module.\n" );
    fprintf( outputFile, "# Use with dmf-get: dmf-get -d %s\n", outputPath );
    fprintf( outputFile, "\n" );

    // Iterate through required modules
    int moduleCount = 0;
    int systemModuleCount = 0;
    const Dmod_RequiredModule_t* reqModule = Dmod_GetNextRequiredModule( context, NULL );
    
    while( reqModule != NULL )
    {
        // Check if this is a valid module entry (name is not empty)
        if( reqModule->Name[0] != '\0' )
        {
            if( reqModule->SystemModule )
            {
                // Skip system modules but count them
                systemModuleCount++;
                DMOD_LOG_INFO("Skipping system module: %s\n", reqModule->Name);
            }
            else
            {
                // Write non-system module to .dmd file
                if( reqModule->Version[0] != '\0' )
                {
                    // Module with version
                    fprintf( outputFile, "%s@%s\n", reqModule->Name, reqModule->Version );
                    printf("  + %s@%s\n", reqModule->Name, reqModule->Version);
                }
                else
                {
                    // Module without version
                    fprintf( outputFile, "%s\n", reqModule->Name );
                    printf("  + %s\n", reqModule->Name);
                }
                moduleCount++;
            }
        }
        
        reqModule = Dmod_GetNextRequiredModule( context, reqModule );
    }

    // Close the output file
    fclose( outputFile );

    // Clean up
    Dmod_Unload( context, false );
    Dmod_Deinitialize();

    // Print summary
    printf("\nSummary:\n");
    printf("  Non-system modules: %d\n", moduleCount);
    printf("  System modules (skipped): %d\n", systemModuleCount);
    printf("  Output file: %s\n", outputPath);
    
    if( moduleCount == 0 && systemModuleCount == 0 )
    {
        printf("\nNote: No dependencies found in this module.\n");
    }
    else if( moduleCount == 0 )
    {
        printf("\nNote: Module only has system dependencies (all were filtered out).\n");
    }
    else
    {
        printf("\nSuccess! .dmd file created successfully.\n");
    }

    return 0;
}
