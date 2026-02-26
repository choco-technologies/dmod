#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dmod.h"

// Maximum path length used internally
#define TODMM_MAX_PATH_LEN 1024

// -----------------------------------------
//
//      Returns true if the filename has a DMF or DMFC extension
//
// -----------------------------------------
static bool IsDmfFile( const char* fileName )
{
    size_t len = strlen( fileName );
    if( len > 4 && strcmp( fileName + len - 4, ".dmf" ) == 0 )
    {
        return true;
    }
    if( len > 5 && strcmp( fileName + len - 5, ".dmfc" ) == 0 )
    {
        return true;
    }
    return false;
}

// -----------------------------------------
//
//      Returns true if the filename has a ZIP extension
//
// -----------------------------------------
static bool IsZipFile( const char* fileName )
{
    size_t len = strlen( fileName );
    if( len > 4 && strcmp( fileName + len - 4, ".zip" ) == 0 )
    {
        return true;
    }
    return false;
}

// -----------------------------------------
//
//      Prints usage message
//
// -----------------------------------------
void PrintUsage( const char* AppName )
{
    printf("Usage: %s <folder> <base_url> [-o output.dmm] [--zip]\n", AppName);
}

// -----------------------------------------
//
//      Prints help message
//
// -----------------------------------------
void PrintHelp( const char* AppName )
{
    printf("-- DMM Manifest Generator ver. " DMOD_VERSION_STRING " --\n\n");
    printf("This application scans a folder of DMF module files and generates\n");
    printf("a manifest.dmm file listing every module found, prefixed with a base URL.\n\n");
    PrintUsage( AppName );
    printf("\nArguments:\n");
    printf("  <folder>       Path to the folder containing .dmf / .dmfc files\n");
    printf("  <base_url>     Base URL or path prepended to each module file entry\n");
    printf("\nOptions:\n");
    printf("  -h, --help     Print this help message\n");
    printf("  -v, --version  Print version information\n");
    printf("  -o <file>      Output manifest file path (default: manifest.dmm)\n");
    printf("  -z, --zip      Scan for .zip release packages instead of .dmf / .dmfc files\n");
    printf("\nExamples:\n");
    printf("  %s ./dmf https://registry.example.com/modules\n", AppName);
    printf("  %s ./dmf https://registry.example.com/modules/ -o output.dmm\n", AppName);
    printf("  %s ./build/packages https://releases.example.com/packages --zip\n", AppName);
    printf("\nDescription:\n");
    printf("  The tool scans the given folder for .dmf and .dmfc files, reads\n");
    printf("  the module name and version from each file header, and writes a\n");
    printf("  manifest.dmm entry of the form:\n");
    printf("    module_name@version <base_url>/<filename>\n");
    printf("  If base_url already ends with '/', no extra separator is added.\n");
    printf("  When --zip is given, the tool scans for .zip files instead and\n");
    printf("  derives the module name from the filename (without extension).\n");
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
        printf("Error: Too few arguments\n");
        PrintUsage( argv[0] );
        return -1;
    }

    const char* folderPath = argv[1];
    const char* baseUrl    = argv[2];
    const char* outputPath = "manifest.dmm";
    bool        zipMode    = false;

    // Parse optional arguments
    for( int i = 3; i < argc; i++ )
    {
        if( strcmp( argv[i], "-o" ) == 0 )
        {
            if( i + 1 < argc )
            {
                outputPath = argv[i + 1];
                i++; // Skip next argument
            }
            else
            {
                printf("Error: -o option requires a file path\n");
                PrintUsage( argv[0] );
                return -1;
            }
        }
        else if( strcmp( argv[i], "-z" ) == 0 || strcmp( argv[i], "--zip" ) == 0 )
        {
            zipMode = true;
        }
        else
        {
            printf("Error: Unknown argument: %s\n", argv[i]);
            PrintUsage( argv[0] );
            return -1;
        }
    }

    // Initialize Dmod system
    if( !Dmod_Initialize(0, 0) )
    {
        printf("Error: Failed to initialize Dmod system\n");
        return -1;
    }

    // Enable crossplatform mode so headers can be read without execution
    Dmod_SetCrossplatformMode( true );

    // Open the directory
    void* dir = Dmod_OpenDir( folderPath );
    if( dir == NULL )
    {
        printf("Error: Cannot open folder: %s\n", folderPath);
        Dmod_Deinitialize();
        return -1;
    }

    // Open the output manifest file
    void* outputFile = Dmod_FileOpen( outputPath, "w" );
    if( outputFile == NULL )
    {
        printf("Error: Cannot create output file: %s\n", outputPath);
        Dmod_CloseDir( dir );
        Dmod_Deinitialize();
        return -1;
    }

    // Write manifest header
    Dmod_FPrintf( outputFile, "# DMOD Manifest\n" );
    Dmod_FPrintf( outputFile, "# Generated by todmm from folder: %s\n", folderPath );
    Dmod_FPrintf( outputFile, "# Base URL: %s\n", baseUrl );
    Dmod_FPrintf( outputFile, "\n" );

    // Determine separator: add '/' between base_url and filename unless already present
    size_t baseUrlLen = strlen( baseUrl );
    const char* separator = (baseUrlLen > 0 && baseUrl[baseUrlLen - 1] == '/') ? "" : "/";

    int moduleCount = 0;
    int errorCount  = 0;

    const Dmod_DirEntry_t* entry;
    while( (entry = Dmod_ReadDirEx( dir )) != NULL )
    {
        // Only process regular files
        if( entry->type != Dmod_DirEntryType_File )
        {
            continue;
        }

        if( zipMode )
        {
            // ZIP mode: scan for .zip release packages
            if( !IsZipFile( entry->name ) )
            {
                continue;
            }

            // Build the full URL for this zip file
            char url[TODMM_MAX_PATH_LEN];
            Dmod_SnPrintf( url, sizeof(url), "%s%s%s", baseUrl, separator, entry->name );

            // Derive module name from filename (strip .zip extension)
            // IsZipFile() guarantees len > 4, so nameLen is at least 1
            size_t nameLen = strlen( entry->name ) - 4; // len(".zip") == 4
            char moduleName[TODMM_MAX_PATH_LEN];
            Dmod_SnPrintf( moduleName, sizeof(moduleName), "%.*s", (int)nameLen, entry->name );

            // Write manifest entry: name url
            Dmod_FPrintf( outputFile, "%s %s\n", moduleName, url );
            Dmod_Printf("  + %s %s\n", moduleName, url);

            moduleCount++;
        }
        else
        {
            // DMF mode: scan for .dmf / .dmfc module files
            if( !IsDmfFile( entry->name ) )
            {
                continue;
            }

            // Build full file path
            char filePath[TODMM_MAX_PATH_LEN];
            Dmod_SnPrintf( filePath, sizeof(filePath), "%s/%s", folderPath, entry->name );

            // Read module header
            Dmod_ModuleHeader_t header;
            if( !Dmod_ReadModuleHeader( filePath, &header ) )
            {
                printf("Warning: Cannot read header from '%s', skipping\n", entry->name);
                errorCount++;
                continue;
            }

            // Build the full URL for this module file
            char url[TODMM_MAX_PATH_LEN];
            Dmod_SnPrintf( url, sizeof(url), "%s%s%s", baseUrl, separator, entry->name );

            // Write manifest entry: name@version url  (omit @version if version is empty)
            if( header.Version[0] != '\0' )
            {
                Dmod_FPrintf( outputFile, "%s@%s %s\n", header.Name, header.Version, url );
                Dmod_Printf("  + %s@%s %s\n", header.Name, header.Version, url);
            }
            else
            {
                Dmod_FPrintf( outputFile, "%s %s\n", header.Name, url );
                Dmod_Printf("  + %s %s\n", header.Name, url);
            }

            moduleCount++;
        }
    }

    // Close directory and output file
    Dmod_CloseDir( dir );
    Dmod_FileClose( outputFile );
    Dmod_Deinitialize();

    // Print summary
    Dmod_Printf("\nSummary:\n");
    Dmod_Printf("  Modules added: %d\n", moduleCount);
    if( errorCount > 0 )
    {
        Dmod_Printf("  Skipped (errors): %d\n", errorCount);
    }
    Dmod_Printf("  Output file: %s\n", outputPath);

    if( moduleCount == 0 )
    {
        if( zipMode )
        {
            printf("Warning: No ZIP packages found in folder: %s\n", folderPath);
        }
        else
        {
            printf("Warning: No DMF modules found in folder: %s\n", folderPath);
        }
    }
    else
    {
        Dmod_Printf("\nSuccess! Manifest file created successfully.\n");
    }

    return 0;
}
