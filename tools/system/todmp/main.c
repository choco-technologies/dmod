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
    printf("Usage: %s <package_name> <input_dir> [output_file] [module_name]\n", AppName);
    printf("\n");
    printf("Arguments:\n");
    printf("  <package_name>   - Name of the package (for the header)\n");
    printf("  <input_dir>      - Folder with modules to pack (.dmf or .dmfc files)\n");
    printf("  [output_file]    - (optional) Path to output .dmp file (default: ./package_name.dmp)\n");
    printf("  [module_name]    - (optional) Name of the main module in the package\n");
}

// -----------------------------------------
//
//      Prints help message
//
// -----------------------------------------
void PrintHelp( const char* AppName )
{
    printf("-- DMP Package Creator ver. " DMOD_VERSION_STRING " --\n\n");
    printf("This application allows you to create a DMP package from multiple modules.\n");
    printf("DMP packages can contain multiple DMF or DMFC modules and be loaded together.\n\n");
    PrintUsage( AppName );
    printf("\n");
    printf("Options:\n");
    printf("  -h, --help            Print this help message\n");
    printf("  -v, --version         Print version information\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s kernel ./dmfc main-app ./out/kernel.dmp\n", AppName);
    printf("  %s mypackage ./modules\n", AppName);
    printf("  %s mypackage ./modules ./output/mypackage.dmp\n", AppName);
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

    if( argc < 3 )
    {
        printf("Error: Missing required arguments\n");
        PrintUsage( argv[0] );
        return -1;
    }

    if( argc > 5 )
    {
        printf("Error: Too many arguments\n");
        PrintUsage( argv[0] );
        return -1;
    }

    const char* packageName = argv[1];
    const char* inputDir = argv[2];
    const char* outputFile = NULL;
    const char* mainModuleName = NULL;

    // Default output file: ./package_name.dmp
    char defaultOutputFile[256];
    if( argc >= 4 )
    {
        outputFile = argv[3];
    }
    else
    {
        snprintf( defaultOutputFile, sizeof(defaultOutputFile), "./%s.dmp", packageName );
        outputFile = defaultOutputFile;
    }

    // Optional main module name
    if( argc >= 5 )
    {
        mainModuleName = argv[4];
    }

    printf("Creating DMP package...\n");
    printf("  Package name: %s\n", packageName);
    printf("  Input directory: %s\n", inputDir);
    printf("  Output file: %s\n", outputFile);
    if( mainModuleName != NULL )
    {
        printf("  Main module: %s\n", mainModuleName);
    }

    if( !Dmod_ToDMPFile( packageName, inputDir, outputFile, mainModuleName ) )
    {
        printf("Error: Failed to create DMP package\n");
        return -1;
    }

    printf("DMP package '%s' was successfully created at '%s'\n", packageName, outputFile);

    return 0;
}
