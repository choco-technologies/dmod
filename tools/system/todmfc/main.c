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
    printf("Usage: %s path/to/file.dmf path/to/output.dmfc [compression_method] [level]\n", AppName);
}

// -----------------------------------------
//
//      Prints supported compression methods
//
// -----------------------------------------
void PrintSupportedCompressionMethods()
{
    printf("Supported compression methods: ");
    for( const char* compression = Dmod_Compression_GetNextSupported(NULL); compression != NULL; compression = Dmod_Compression_GetNextSupported(compression) )
    {
        printf("%s ", compression);
    }
    printf("\n");
}

// -----------------------------------------
//
//      Prints help message
//
// -----------------------------------------
void PrintHelp( const char* AppName )
{
    printf("-- DMFC Creator ver. " DMOD_VERSION_STRING " --\n\n");
    printf("This application allows you to compress a DMF file into the DMFC\n");
    PrintUsage( AppName );
    printf("Options:\n");
    printf("  -h, --help            Print this help message\n");
    printf("  -v, --version         Print version information\n");
    printf("  [compression_method]  (default: %s) Compression method method to be used. ", Dmod_Compression_GetNextSupported(NULL));
    PrintSupportedCompressionMethods();
    printf("  [level]               (default: 2)  Compression level - depends on the [compression_method]\n");
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

    if( argc > 5 )
    {
        printf("Too many arguments\n");
        PrintUsage( argv[0] );
        return -1;
    }
    else if( argc < 3 )
    {
        printf("Too few arguments\n");
        PrintUsage( argv[0] );
        return -1;
    }

    const char* compression = "fastlz";
    if( argc >= 4 )
    {
        compression = argv[3];
        if( Dmod_Compression_IsSupported( compression ) == false )
        {
            printf("Compression method '%s' is not supported\n", compression);
            PrintSupportedCompressionMethods();
            return -1;
        }
    }

    int level = 2;
    if( argc >= 5 )
    {
        level = atoi( argv[4] );
    }

    const char* dmfPath = argv[1];
    const char* dmfcPath = argv[2];

    if(Dmod_ToDMFCFile( compression, level, dmfPath, dmfcPath ) == false)
    {
        printf("Failed to convert DMF to DMFC\n");
        return -1;
    }

    printf("DMF file '%s' was successfully converted to DMFC file '%s'\n", dmfPath, dmfcPath);

    // Comparison of the size of the original file and the compressed file
    void* dmfFile = Dmod_FileOpen( dmfPath, "rb" );
    void* dmfcFile = Dmod_FileOpen( dmfcPath, "rb" );
    if( dmfFile == NULL || dmfcFile == NULL )
    {
        printf("Failed to open files for comparison\n");
        return -1;
    }

    size_t dmfSize = Dmod_FileSize( dmfFile );
    size_t dmfcSize = Dmod_FileSize( dmfcFile );

    Dmod_FileClose( dmfFile );
    Dmod_FileClose( dmfcFile );

    printf("Original file size: %lu bytes\n", dmfSize);
    printf("Compressed file size: %lu bytes\n", dmfcSize);
    printf("Compression ratio: %.2f%%\n", (float)dmfcSize / dmfSize * 100.0f);

    return 0;
}

