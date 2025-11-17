#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>
#include "dmod.h"
#include "dmod_dependencies.h"

// Use standard POSIX functions directly since we're a standalone tool
// These override the weak API definitions by redefining as inline functions
static inline void* DmodOpenDir( const char* path ) { return opendir(path); }
static inline const char* DmodReadDir( void* dir ) { 
    struct dirent* entry = readdir((DIR*)dir); 
    return entry ? entry->d_name : NULL; 
}
static inline void DmodCloseDir( void* dir ) { closedir((DIR*)dir); }

// Forward declaration - implementation copied from dmod_dmp_api.c
bool Dmod_ToDMPFile( const char* PackageName, const char* InputDir, const char* OutputFile, const char* MainModuleName );

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
//      Helper: Check if file exists
//
// -----------------------------------------
int FileExists( const char* path )
{
    void* file = Dmod_FileOpen( path, "rb" );
    if( file == NULL )
    {
        return 0;
    }
    Dmod_FileClose( file );
    return 1;
}

// -----------------------------------------
//
//      Helper: Search for module file in directories
//      Returns allocated string with full path or NULL
//
// -----------------------------------------
char* FindModuleFile( const char* moduleName, const char* searchDir, 
                       const char* dmfDir, const char* dmfcDir )
{
    char path[512];
    
    // Try .dmf extension first in search dir
    snprintf( path, sizeof(path), "%s/%s.dmf", searchDir, moduleName );
    if( FileExists( path ) )
    {
        return strdup( path );
    }
    
    // Try .dmfc extension in search dir
    snprintf( path, sizeof(path), "%s/%s.dmfc", searchDir, moduleName );
    if( FileExists( path ) )
    {
        return strdup( path );
    }
    
    // Try DMOD_DMF_DIR
    if( dmfDir != NULL && dmfDir[0] != '\0' )
    {
        snprintf( path, sizeof(path), "%s/%s.dmf", dmfDir, moduleName );
        if( FileExists( path ) )
        {
            return strdup( path );
        }
    }
    
    // Try DMOD_DMFC_DIR
    if( dmfcDir != NULL && dmfcDir[0] != '\0' )
    {
        snprintf( path, sizeof(path), "%s/%s.dmfc", dmfcDir, moduleName );
        if( FileExists( path ) )
        {
            return strdup( path );
        }
    }
    
    return NULL;
}

// -----------------------------------------
//
//      Helper: Get architecture from DMF file
//
// -----------------------------------------
int GetModuleArchitecture( const char* filePath, char* outArch, size_t maxLen )
{
    Dmod_ModuleHeader_t header;
    
    // Read module header directly from file
    void* file = Dmod_FileOpen( filePath, "rb" );
    if( file == NULL )
    {
        return 0;
    }
    
    if( Dmod_FileRead( &header, sizeof(Dmod_ModuleHeader_t), 1, file ) != 1 )
    {
        Dmod_FileClose( file );
        return 0;
    }
    
    Dmod_FileClose( file );
    
    strncpy( outArch, header.Arch, maxLen );
    outArch[maxLen - 1] = '\0';
    
    return 1;
}

// -----------------------------------------
//
//      Helper: Create package with dependencies
//
// -----------------------------------------
int CreatePackageWithDependencies( const char* dmfFile, const char* dmdFile,
                                    const char* searchDir, const char* outputFile )
{
    // Get environment variables
    const char* dmfDir = getenv( "DMOD_DMF_DIR" );
    const char* dmfcDir = getenv( "DMOD_DMFC_DIR" );
    
    printf("Creating package with dependencies...\n");
    printf("  Main DMF: %s\n", dmfFile);
    if( dmdFile != NULL )
    {
        printf("  Dependencies file: %s\n", dmdFile);
    }
    printf("  Search directory: %s\n", searchDir);
    if( dmfDir != NULL )
    {
        printf("  DMOD_DMF_DIR: %s\n", dmfDir);
    }
    if( dmfcDir != NULL )
    {
        printf("  DMOD_DMFC_DIR: %s\n", dmfcDir);
    }
    printf("  Output file: %s\n", outputFile);
    printf("\n");
    
    // Get the architecture from the main DMF file
    char mainArch[64];
    if( !GetModuleArchitecture( dmfFile, mainArch, sizeof(mainArch) ) )
    {
        printf("Error: Cannot read architecture from main DMF file: %s\n", dmfFile);
        return -1;
    }
    printf("Target architecture: %s\n", mainArch);
    
    // TODO: Parse dependencies from DMD file or DMF file
    // For now, we'll just create a package with the main DMF file
    // This is a placeholder that maintains current functionality
    
    printf("\nWarning: Dependency resolution not yet fully implemented.\n");
    printf("Creating package with main module only.\n\n");
    
    // Create a temporary directory structure
    // Copy the main DMF file to a temp location and use existing ToDMPFile
    char tempDir[512];
    snprintf( tempDir, sizeof(tempDir), "/tmp/todmp_%d", (int)getpid() );
    
    // Create temp directory
    char mkdirCmd[600];
    snprintf( mkdirCmd, sizeof(mkdirCmd), "mkdir -p %s", tempDir );
    system( mkdirCmd );
    
    // Copy main DMF file to temp directory
    char cpCmd[1024];
    const char* baseName = strrchr( dmfFile, '/' );
    if( baseName == NULL )
    {
        baseName = dmfFile;
    }
    else
    {
        baseName++; // Skip the '/'
    }
    
    snprintf( cpCmd, sizeof(cpCmd), "cp %s %s/", dmfFile, tempDir );
    system( cpCmd );
    
    // Extract module name from file name (without extension)
    char moduleName[128];
    strncpy( moduleName, baseName, sizeof(moduleName) - 1 );
    moduleName[sizeof(moduleName) - 1] = '\0';
    
    // Remove extension
    char* ext = strrchr( moduleName, '.' );
    if( ext != NULL )
    {
        *ext = '\0';
    }
    
    // Extract package name from DMF file name
    char packageName[128];
    strncpy( packageName, moduleName, sizeof(packageName) - 1 );
    packageName[sizeof(packageName) - 1] = '\0';
    
    // Create the DMP package using the existing inline implementation
    // Count files
    uint32_t moduleCount = 1; // Just the main module for now
    
    // Allocate module entries
    Dmod_DmpModuleEntry_t* moduleEntries = (Dmod_DmpModuleEntry_t*)malloc( sizeof(Dmod_DmpModuleEntry_t) );
    if( moduleEntries == NULL )
    {
        printf("Error: Cannot allocate memory\n");
        system( mkdirCmd );
        return -1;
    }
    
    // Get file size
    void* file = Dmod_FileOpen( dmfFile, "rb" );
    if( file == NULL )
    {
        printf("Error: Cannot open DMF file: %s\n", dmfFile);
        free( moduleEntries );
        return -1;
    }
    
    size_t fileSize = Dmod_FileSize( file );
    Dmod_FileClose( file );
    
    // Fill module entry
    uint32_t dataOffset = sizeof(Dmod_DmpHeader_t) + moduleCount * sizeof(Dmod_DmpModuleEntry_t);
    moduleEntries[0].ModuleOffset = dataOffset;
    moduleEntries[0].FileSize = (uint32_t)fileSize;
    strncpy( moduleEntries[0].ModuleName, moduleName, DMOD_MAX_MODULE_NAME_LENGTH );
    moduleEntries[0].ModuleName[DMOD_MAX_MODULE_NAME_LENGTH - 1] = '\0';
    
    // Create DMP header
    Dmod_DmpHeader_t header;
    header.Signature = DMOD_DMP_SIGNATURE;
    header.HeaderSize = sizeof(Dmod_DmpHeader_t);
    header.HeaderVersion = DMOD_DMP_VERSION;
    strncpy( header.Name, packageName, DMOD_MAX_PACKAGE_NAME_LENGTH );
    header.Name[DMOD_MAX_PACKAGE_NAME_LENGTH - 1] = '\0';
    header.MainIndex = 0;
    header.ModuleCount = moduleCount;
    
    // Open output file
    void* outFile = Dmod_FileOpen( outputFile, "wb" );
    if( outFile == NULL )
    {
        printf("Error: Cannot create output file: %s\n", outputFile);
        free( moduleEntries );
        return -1;
    }
    
    // Write header
    if( Dmod_FileWrite( &header, sizeof(header), 1, outFile ) != 1 )
    {
        printf("Error: Cannot write header\n");
        Dmod_FileClose( outFile );
        free( moduleEntries );
        return -1;
    }
    
    // Write module entries
    if( Dmod_FileWrite( moduleEntries, sizeof(Dmod_DmpModuleEntry_t), moduleCount, outFile ) != moduleCount )
    {
        printf("Error: Cannot write module entries\n");
        Dmod_FileClose( outFile );
        free( moduleEntries );
        return -1;
    }
    
    // Write module data
    file = Dmod_FileOpen( dmfFile, "rb" );
    if( file == NULL )
    {
        printf("Error: Cannot reopen DMF file: %s\n", dmfFile);
        Dmod_FileClose( outFile );
        free( moduleEntries );
        return -1;
    }
    
    void* buffer = malloc( fileSize );
    if( buffer == NULL )
    {
        printf("Error: Cannot allocate buffer\n");
        Dmod_FileClose( file );
        Dmod_FileClose( outFile );
        free( moduleEntries );
        return -1;
    }
    
    if( Dmod_FileRead( buffer, 1, fileSize, file ) != fileSize )
    {
        printf("Error: Cannot read DMF file\n");
        free( buffer );
        Dmod_FileClose( file );
        Dmod_FileClose( outFile );
        free( moduleEntries );
        return -1;
    }
    Dmod_FileClose( file );
    
    if( Dmod_FileWrite( buffer, 1, fileSize, outFile ) != fileSize )
    {
        printf("Error: Cannot write file data\n");
        free( buffer );
        Dmod_FileClose( outFile );
        free( moduleEntries );
        return -1;
    }
    
    free( buffer );
    Dmod_FileClose( outFile );
    free( moduleEntries );
    
    printf("DMP package '%s' was successfully created at '%s'\n", packageName, outputFile);
    return 0;
}

// -----------------------------------------
//
//      Implementation of Dmod_ToDMPFile (inline version)
//      Copied and adapted from dmod_dmp_api.c
//
// -----------------------------------------
bool Dmod_ToDMPFile( const char* PackageName, const char* InputDir, const char* OutputFile, const char* MainModuleName )
{
    printf("DEBUG: Dmod_ToDMPFile called\n");
    printf("DEBUG: PackageName=%s, InputDir=%s, OutputFile=%s\n", PackageName, InputDir, OutputFile);
    
    if( PackageName == NULL || InputDir == NULL || OutputFile == NULL )
    {
        printf("Error: Cannot create DMP file - invalid parameters\n");
        return false;
    }

    printf("DEBUG: About to call DmodOpenDir\n");
    // Open the input directory
    DIR* dir_handle = (DIR*)DmodOpenDir( InputDir );
    printf("DEBUG: DmodOpenDir returned: %p\n", dir_handle);
    if( dir_handle == NULL )
    {
        printf("Error: Cannot create DMP file - cannot open input directory '%s'\n", InputDir);
        return false;
    }

    // Count the number of module files
    printf("DEBUG: Starting to count files\n");
    uint32_t moduleCount = 0;
    const char* fileName = NULL;
    int count = 0;
    while( (fileName = DmodReadDir( (void*)dir_handle )) != NULL && count < 100 )
    {
        count++;
        printf("DEBUG: Found file: %s\n", fileName);
        size_t len = strlen( fileName );
        if( len > 4 && (strcmp( fileName + len - 4, ".dmf" ) == 0 || (len > 5 && strcmp( fileName + len - 5, ".dmfc" ) == 0)) )
        {
            printf("DEBUG: Counting module: %s\n", fileName);
            moduleCount++;
        }
    }
    printf("DEBUG: Loop exited, count=%d, moduleCount=%u\n", count, moduleCount);
    DmodCloseDir( (void*)dir_handle );

    if( moduleCount == 0 )
    {
        printf("Error: Cannot create DMP file - no module files found in '%s'\n", InputDir);
        return false;
    }

    // Allocate memory for module entries
    Dmod_DmpModuleEntry_t* moduleEntries = (Dmod_DmpModuleEntry_t*)malloc( moduleCount * sizeof(Dmod_DmpModuleEntry_t) );
    if( moduleEntries == NULL )
    {
        printf("Error: Cannot create DMP file - cannot allocate memory for module entries\n");
        return false;
    }

    // Calculate total size and fill module entries
    dir = DmodOpenDir( InputDir );
    if( dir == NULL )
    {
        printf("Error: Cannot create DMP file - cannot reopen input directory '%s'\n", InputDir);
        free( moduleEntries );
        return false;
    }

    uint32_t currentIndex = 0;
    uint32_t mainIndex = 0;
    size_t dataOffset = sizeof(Dmod_DmpHeader_t) + moduleCount * sizeof(Dmod_DmpModuleEntry_t);
    size_t totalSize = dataOffset;

    while( (fileName = DmodReadDir( dir )) != NULL )
    {
        size_t len = strlen( fileName );
        bool isDmf = (len > 4 && strcmp( fileName + len - 4, ".dmf" ) == 0);
        bool isDmfc = (len > 5 && strcmp( fileName + len - 5, ".dmfc" ) == 0);
        
        if( !isDmf && !isDmfc )
        {
            continue;
        }

        // Build full path
        char filePath[1024];
        snprintf( filePath, sizeof(filePath), "%s/%s", InputDir, fileName );

        // Open the file to get its size
        void* file = Dmod_FileOpen( filePath, "rb" );
        if( file == NULL )
        {
            printf("Error: Cannot create DMP file - cannot open module file '%s'\n", filePath);
            DmodCloseDir( dir );
            free( moduleEntries );
            return false;
        }

        size_t fileSize = Dmod_FileSize( file );
        Dmod_FileClose( file );

        // Extract module name from file name (without extension)
        char moduleName[DMOD_MAX_MODULE_NAME_LENGTH];
        size_t nameLen = isDmf ? len - 4 : len - 5;
        if( nameLen >= DMOD_MAX_MODULE_NAME_LENGTH )
        {
            nameLen = DMOD_MAX_MODULE_NAME_LENGTH - 1;
        }
        memcpy( moduleName, fileName, nameLen );
        moduleName[nameLen] = '\0';

        // Fill module entry
        moduleEntries[currentIndex].ModuleOffset = (uint32_t)dataOffset;
        moduleEntries[currentIndex].FileSize = (uint32_t)fileSize;
        strncpy( moduleEntries[currentIndex].ModuleName, moduleName, DMOD_MAX_MODULE_NAME_LENGTH );
        moduleEntries[currentIndex].ModuleName[DMOD_MAX_MODULE_NAME_LENGTH - 1] = '\0';

        // Check if this is the main module
        if( MainModuleName != NULL && strcmp( moduleName, MainModuleName ) == 0 )
        {
            mainIndex = currentIndex;
        }

        dataOffset += fileSize;
        totalSize += fileSize;
        currentIndex++;
    }
    DmodCloseDir( dir );

    // Print list of modules being added
    printf("Adding %u module(s) to package '%s':\n", moduleCount, PackageName);
    for( uint32_t i = 0; i < moduleCount; i++ )
    {
        printf("  [%u] %s (size: %u bytes, offset: %u)%s\n", 
            i, 
            moduleEntries[i].ModuleName, 
            moduleEntries[i].FileSize,
            moduleEntries[i].ModuleOffset,
            (i == mainIndex) ? " [MAIN]" : "");
    }

    // Create DMP header
    Dmod_DmpHeader_t header;
    header.Signature = DMOD_DMP_SIGNATURE;
    header.HeaderSize = sizeof(Dmod_DmpHeader_t);
    header.HeaderVersion = DMOD_DMP_VERSION;
    strncpy( header.Name, PackageName, DMOD_MAX_PACKAGE_NAME_LENGTH );
    header.Name[DMOD_MAX_PACKAGE_NAME_LENGTH - 1] = '\0';
    header.MainIndex = mainIndex;
    header.ModuleCount = moduleCount;

    // Open output file
    void* outFile = Dmod_FileOpen( OutputFile, "wb" );
    if( outFile == NULL )
    {
        printf("Error: Cannot create DMP file - cannot open output file '%s'\n", OutputFile);
        free( moduleEntries );
        return false;
    }

    // Write header
    if( Dmod_FileWrite( &header, sizeof(header), 1, outFile ) != 1 )
    {
        printf("Error: Cannot create DMP file - cannot write header\n");
        Dmod_FileClose( outFile );
        free( moduleEntries );
        return false;
    }

    // Write module entries
    if( Dmod_FileWrite( moduleEntries, sizeof(Dmod_DmpModuleEntry_t), moduleCount, outFile ) != moduleCount )
    {
        printf("Error: Cannot create DMP file - cannot write module entries\n");
        Dmod_FileClose( outFile );
        free( moduleEntries );
        return false;
    }

    // Write module data
    dir = DmodOpenDir( InputDir );
    if( dir == NULL )
    {
        printf("Error: Cannot create DMP file - cannot reopen input directory for writing data\n");
        Dmod_FileClose( outFile );
        free( moduleEntries );
        return false;
    }

    while( (fileName = DmodReadDir( dir )) != NULL )
    {
        size_t len = strlen( fileName );
        bool isDmf = (len > 4 && strcmp( fileName + len - 4, ".dmf" ) == 0);
        bool isDmfc = (len > 5 && strcmp( fileName + len - 5, ".dmfc" ) == 0);
        
        if( !isDmf && !isDmfc )
        {
            continue;
        }

        // Build full path
        char filePath[1024];
        snprintf( filePath, sizeof(filePath), "%s/%s", InputDir, fileName );

        // Open the file
        void* file = Dmod_FileOpen( filePath, "rb" );
        if( file == NULL )
        {
            printf("Error: Cannot create DMP file - cannot open module file '%s' for reading\n", filePath);
            DmodCloseDir( dir );
            Dmod_FileClose( outFile );
            free( moduleEntries );
            return false;
        }

        size_t fileSize = Dmod_FileSize( file );
        void* buffer = malloc( fileSize );
        if( buffer == NULL )
        {
            printf("Error: Cannot create DMP file - cannot allocate buffer for file '%s'\n", filePath);
            Dmod_FileClose( file );
            DmodCloseDir( dir );
            Dmod_FileClose( outFile );
            free( moduleEntries );
            return false;
        }

        if( Dmod_FileRead( buffer, 1, fileSize, file ) != fileSize )
        {
            printf("Error: Cannot create DMP file - cannot read file '%s'\n", filePath);
            free( buffer );
            Dmod_FileClose( file );
            DmodCloseDir( dir );
            Dmod_FileClose( outFile );
            free( moduleEntries );
            return false;
        }
        Dmod_FileClose( file );

        if( Dmod_FileWrite( buffer, 1, fileSize, outFile ) != fileSize )
        {
            printf("Error: Cannot create DMP file - cannot write file data '%s'\n", filePath);
            free( buffer );
            DmodCloseDir( dir );
            Dmod_FileClose( outFile );
            free( moduleEntries );
            return false;
        }

        free( buffer );
    }

    DmodCloseDir( dir );
    Dmod_FileClose( outFile );
    free( moduleEntries );

    return true;
}

// -----------------------------------------
//
//      Prints usage message
//
// -----------------------------------------
void PrintUsage( const char* AppName )
{
    printf("Usage: %s <package_name> <input_dir> [output_file] [module_name]\n", AppName);
    printf("       %s -d <dmf_file> [dmd_file] <input_dir> [output_file]\n", AppName);
    printf("       %s -l <package_file>\n", AppName);
    printf("\n");
    printf("Arguments:\n");
    printf("  <package_name>   - Name of the package (for the header)\n");
    printf("  <input_dir>      - Folder with modules to pack (.dmf or .dmfc files)\n");
    printf("  [output_file]    - (optional) Path to output .dmp file (default: ./package_name.dmp)\n");
    printf("  [module_name]    - (optional) Name of the main module in the package\n");
    printf("  -d <dmf_file>    - Create package with dependencies from DMF file\n");
    printf("  [dmd_file]       - (optional) Dependencies file (if not provided, read from DMF)\n");
    printf("  -l <package_file> - List contents of a DMP package\n");
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
    printf("  -l, --list <file>     List contents of a DMP package\n");
    printf("  -d <dmf_file>         Create package with dependencies\n");
    printf("\n");
    printf("Environment Variables:\n");
    printf("  DMOD_DMF_DIR          Additional search path for .dmf files\n");
    printf("  DMOD_DMFC_DIR         Additional search path for .dmfc files\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s kernel ./dmfc main-app ./out/kernel.dmp\n", AppName);
    printf("  %s mypackage ./modules\n", AppName);
    printf("  %s mypackage ./modules ./output/mypackage.dmp\n", AppName);
    printf("  %s -d myapp.dmf ./modules ./output/myapp.dmp\n", AppName);
    printf("  %s -d myapp.dmf deps.dmd ./modules ./output/myapp.dmp\n", AppName);
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
    if( strcmp( argv[1], "-d" ) == 0 )
    {
        if( argc < 4 )
        {
            printf("Error: Missing required arguments for -d option\n");
            printf("Usage: %s -d <dmf_file> [dmd_file] <input_dir> [output_file]\n", argv[0]);
            return -1;
        }
        
        const char* dmfFile = argv[2];
        const char* dmdFile = NULL;
        const char* searchDir = NULL;
        const char* outputFile = NULL;
        
        // Parse arguments - need to determine if arg 3 is dmd file or search dir
        // Check if arg 3 ends with .dmd
        if( argc >= 4 )
        {
            size_t len = strlen( argv[3] );
            if( len > 4 && strcmp( argv[3] + len - 4, ".dmd" ) == 0 )
            {
                dmdFile = argv[3];
                
                if( argc >= 5 )
                {
                    searchDir = argv[4];
                }
                if( argc >= 6 )
                {
                    outputFile = argv[5];
                }
            }
            else
            {
                searchDir = argv[3];
                if( argc >= 5 )
                {
                    outputFile = argv[4];
                }
            }
        }
        
        if( searchDir == NULL )
        {
            printf("Error: Missing search directory\n");
            printf("Usage: %s -d <dmf_file> [dmd_file] <input_dir> [output_file]\n", argv[0]);
            return -1;
        }
        
        // Default output file
        char defaultOutputFile[256];
        if( outputFile == NULL )
        {
            // Extract module name from DMF file
            const char* baseName = strrchr( dmfFile, '/' );
            if( baseName == NULL )
            {
                baseName = dmfFile;
            }
            else
            {
                baseName++;
            }
            
            // Remove extension
            char moduleName[128];
            strncpy( moduleName, baseName, sizeof(moduleName) - 1 );
            moduleName[sizeof(moduleName) - 1] = '\0';
            char* ext = strrchr( moduleName, '.' );
            if( ext != NULL )
            {
                *ext = '\0';
            }
            
            snprintf( defaultOutputFile, sizeof(defaultOutputFile), "./%s.dmp", moduleName );
            outputFile = defaultOutputFile;
        }
        
        return CreatePackageWithDependencies( dmfFile, dmdFile, searchDir, outputFile );
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
