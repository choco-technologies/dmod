#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dmod.h"

// -----------------------------------------
//
//      Lists contents of a DMP package
//
// -----------------------------------------
int ListDMPPackage( const char* packageFile )
{
    printf("Reading DMP package: %s\n", packageFile);
    
    // Open the file
    void* file = Dmod_FileOpen( packageFile, "rb" );
    if( file == NULL )
    {
        printf("Error: Cannot open file '%s'\n", packageFile);
        return -1;
    }
    
    // Read header
    Dmod_DmpHeader_t header;
    if( Dmod_FileRead( &header, sizeof(header), 1, file ) != 1 )
    {
        printf("Error: Cannot read DMP header\n");
        Dmod_FileClose( file );
        return -1;
    }
    
    // Verify signature
    if( header.Signature != DMOD_DMP_SIGNATURE )
    {
        printf("Error: Invalid DMP signature (0x%08X)\n", header.Signature);
        Dmod_FileClose( file );
        return -1;
    }
    
    // Print header info
    printf("\nPackage Information:\n");
    printf("  Name: %s\n", header.Name);
    printf("  Version: 0x%04X\n", header.HeaderVersion);
    printf("  Module Count: %u\n", header.ModuleCount);
    printf("  Main Module Index: %u\n", header.MainIndex);
    printf("  Header Size: %u bytes\n", header.HeaderSize);
    
    // Read module entries
    if( header.ModuleCount == 0 )
    {
        printf("\nNo modules in package.\n");
        Dmod_FileClose( file );
        return 0;
    }
    
    Dmod_DmpModuleEntry_t* entries = (Dmod_DmpModuleEntry_t*)malloc( header.ModuleCount * sizeof(Dmod_DmpModuleEntry_t) );
    if( entries == NULL )
    {
        printf("Error: Cannot allocate memory for module entries\n");
        Dmod_FileClose( file );
        return -1;
    }
    
    if( Dmod_FileRead( entries, sizeof(Dmod_DmpModuleEntry_t), header.ModuleCount, file ) != header.ModuleCount )
    {
        printf("Error: Cannot read module entries\n");
        free( entries );
        Dmod_FileClose( file );
        return -1;
    }
    
    // Print module list
    printf("\nModules:\n");
    for( uint32_t i = 0; i < header.ModuleCount; i++ )
    {
        printf("  [%u] %s\n", i, entries[i].ModuleName);
        printf("      Offset: %u bytes\n", entries[i].ModuleOffset);
        printf("      Size: %u bytes\n", entries[i].FileSize);
        if( i == header.MainIndex )
        {
            printf("      [MAIN MODULE]\n");
        }
    }
    
    free( entries );
    Dmod_FileClose( file );
    
    return 0;
}

// -----------------------------------------
//
//      Prints usage message
//
// -----------------------------------------
void PrintUsage( const char* AppName )
{
    printf("Usage: %s <package_name> <input_dir> [output_file] [module_name]\n", AppName);
    printf("       %s -d <package_name> <main_module_path> [output_file]\n", AppName);
    printf("       %s -l <package_file>\n", AppName);
    printf("\n");
    printf("Arguments:\n");
    printf("  <package_name>      - Name of the package (for the header)\n");
    printf("  <input_dir>         - Folder with modules to pack (.dmf or .dmfc files)\n");
    printf("  [output_file]       - (optional) Path to output .dmp file (default: ./package_name.dmp)\n");
    printf("  [module_name]       - (optional) Name of the main module in the package\n");
    printf("  -d <package_name> <main_module_path> [output_file]\n");
    printf("                      - Create package with dependencies\n");
    printf("  -l <package_file>   - List contents of a DMP package\n");
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
    printf("  -h, --help               Print this help message\n");
    printf("  -v, --version            Print version information\n");
    printf("  -l, --list <file>        List contents of a DMP package\n");
    printf("  -d, --dependencies       Create package with dependencies\n");
    printf("\n");
    printf("Dependency Mode:\n");
    printf("  In dependency mode, todmp will:\n");
    printf("  1. Analyze the main module to find its dependencies\n");
    printf("  2. Search for dependencies in DMOD_REPO_PATHS environment variable\n");
    printf("  3. Package the main module and all found dependencies together\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s kernel ./dmfc main-app ./out/kernel.dmp\n", AppName);
    printf("  %s mypackage ./modules\n", AppName);
    printf("  %s mypackage ./modules ./output/mypackage.dmp\n", AppName);
    printf("  %s -d myapp ./build/myapp.dmf ./myapp.dmp\n", AppName);
    printf("  %s -l ./mypackage.dmp\n", AppName);
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

    // Handle list option
    if( strcmp( argv[1], "-l" ) == 0 || strcmp( argv[1], "--list" ) == 0 )
    {
        if( argc < 3 )
        {
            printf("Error: Missing package file argument\n");
            printf("Usage: %s -l <package_file>\n", argv[0]);
            return -1;
        }
        return ListDMPPackage( argv[2] );
    }

    // Handle dependencies option
    if( strcmp( argv[1], "-d" ) == 0 || strcmp( argv[1], "--dependencies" ) == 0 )
    {
        if( argc < 4 )
        {
            printf("Error: Missing required arguments for dependency mode\n");
            printf("Usage: %s -d <package_name> <main_module_path> [output_file]\n", argv[0]);
            return -1;
        }

        if( argc > 5 )
        {
            printf("Error: Too many arguments for dependency mode\n");
            printf("Usage: %s -d <package_name> <main_module_path> [output_file]\n", argv[0]);
            return -1;
        }

        const char* packageName = argv[2];
        const char* mainModulePath = argv[3];
        const char* outputFile = NULL;

        // Default output file: ./package_name.dmp
        char defaultOutputFile[256];
        if( argc >= 5 )
        {
            outputFile = argv[4];
        }
        else
        {
            snprintf( defaultOutputFile, sizeof(defaultOutputFile), "./%s.dmp", packageName );
            outputFile = defaultOutputFile;
        }

        printf("Creating DMP package with dependencies...\n");
        printf("  Package name: %s\n", packageName);
        printf("  Main module: %s\n", mainModulePath);
        printf("  Output file: %s\n", outputFile);

        if( !Dmod_ToDMPFileWithDependencies( packageName, mainModulePath, outputFile ) )
        {
            printf("Error: Failed to create DMP package with dependencies\n");
            return -1;
        }

        printf("DMP package '%s' was successfully created at '%s'\n", packageName, outputFile);
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
