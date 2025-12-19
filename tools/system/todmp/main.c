#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "dmod.h"
#include "dmod_dependencies.h"

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
//      Check if file is a .dmd file
//
// -----------------------------------------
bool IsDmdFile( const char* filePath )
{
    if( filePath == NULL )
    {
        return false;
    }
    
    size_t len = strlen( filePath );
    if( len < 5 )
    {
        return false;
    }
    
    return strcmp( filePath + len - 4, ".dmd" ) == 0;
}

// -----------------------------------------
//
//      Create DMP package from .dmd file
//
// -----------------------------------------
int CreateDMPFromDmd( const char* packageName, const char* dmdFilePath, 
                      const char* inputDir, const char* outputFile, 
                      const char* mainModuleName )
{
    printf("Reading .dmd file: %s\n", dmdFilePath);
    
    // Initialize dependencies parser (no download function needed since modules should already be downloaded)
    // Use empty string as default manifest since we're not downloading anything
    Dmod_DependenciesContext_t* dep_ctx = Dmod_Dependencies_Init("", NULL, NULL);
    if( dep_ctx == NULL )
    {
        printf("Error: Failed to initialize dependencies parser\n");
        return -1;
    }
    
    // Parse the .dmd file
    if( !Dmod_Dependencies_ParseFile(dep_ctx, dmdFilePath) )
    {
        printf("Error: Failed to parse .dmd file: %s\n", 
               Dmod_Dependencies_GetError(dep_ctx));
        Dmod_Dependencies_Free(dep_ctx);
        return -1;
    }
    
    size_t dep_count = Dmod_Dependencies_GetEntryCount(dep_ctx);
    if( dep_count == 0 )
    {
        printf("Error: No modules found in .dmd file\n");
        Dmod_Dependencies_Free(dep_ctx);
        return -1;
    }
    
    printf("Found %zu module(s) in .dmd file\n", dep_count);
    
    // Get the first module name (for main module if not specified)
    Dmod_DependencyEntry_t first_entry;
    const char* firstModuleName = NULL;
    if( Dmod_Dependencies_GetEntry(dep_ctx, 0, &first_entry) )
    {
        firstModuleName = first_entry.name;
    }
    
    // Verify all modules exist and create a temporary directory with only the needed modules
    char tempDir[DMOD_MAX_PATH_LENGTH];
    char tempTemplate[] = "/tmp/todmp_XXXXXX";
    
    // Create secure temporary directory using mkdtemp
    if( mkdtemp(tempTemplate) == NULL )
    {
        printf("Error: Cannot create temporary directory\n");
        Dmod_Dependencies_Free(dep_ctx);
        return -1;
    }
    
    snprintf(tempDir, sizeof(tempDir), "%s", tempTemplate);
    
    // Copy only the modules specified in .dmd file to temp directory
    for( size_t i = 0; i < dep_count; i++ )
    {
        Dmod_DependencyEntry_t dep_entry;
        if( !Dmod_Dependencies_GetEntry(dep_ctx, i, &dep_entry) )
        {
            printf("Error: Failed to get dependency entry %zu\n", i);
            Dmod_Dependencies_Free(dep_ctx);
            return -1;
        }
        
        printf("  [%zu] %s\n", i + 1, dep_entry.name);
        
        // Try both .dmf and .dmfc extensions
        char sourcePath[DMOD_MAX_PATH_LENGTH];
        char destPath[DMOD_MAX_PATH_LENGTH];
        bool found = false;
        
        // Try .dmf
        snprintf(sourcePath, sizeof(sourcePath), "%s/%s.dmf", inputDir, dep_entry.name);
        void* testFile = Dmod_FileOpen(sourcePath, "rb");
        if( testFile != NULL )
        {
            Dmod_FileClose(testFile);
            snprintf(destPath, sizeof(destPath), "%s/%s.dmf", tempDir, dep_entry.name);
            found = true;
        }
        else
        {
            // Try .dmfc
            snprintf(sourcePath, sizeof(sourcePath), "%s/%s.dmfc", inputDir, dep_entry.name);
            testFile = Dmod_FileOpen(sourcePath, "rb");
            if( testFile != NULL )
            {
                Dmod_FileClose(testFile);
                snprintf(destPath, sizeof(destPath), "%s/%s.dmfc", tempDir, dep_entry.name);
                found = true;
            }
        }
        
        if( !found )
        {
            printf("Error: Module '%s' not found in directory '%s' (tried .dmf and .dmfc)\n", 
                   dep_entry.name, inputDir);
            
            // Clean up temp directory before returning
            void* cleanupDir = Dmod_OpenDir(tempDir);
            if( cleanupDir != NULL )
            {
                const char* cleanupFile;
                while( (cleanupFile = Dmod_ReadDir(cleanupDir)) != NULL )
                {
                    if( strcmp(cleanupFile, ".") == 0 || strcmp(cleanupFile, "..") == 0 ) continue;
                    char cleanupPath[DMOD_MAX_PATH_LENGTH];
                    snprintf(cleanupPath, sizeof(cleanupPath), "%s/%s", tempDir, cleanupFile);
                    unlink(cleanupPath);
                }
                Dmod_CloseDir(cleanupDir);
            }
            rmdir(tempDir);
            
            Dmod_Dependencies_Free(dep_ctx);
            return -1;
        }
        
        // Copy the file using a buffer
        void* srcFile = Dmod_FileOpen(sourcePath, "rb");
        if( srcFile == NULL )
        {
            printf("Error: Cannot open source file '%s'\n", sourcePath);
            Dmod_Dependencies_Free(dep_ctx);
            return -1;
        }
        
        void* dstFile = Dmod_FileOpen(destPath, "wb");
        if( dstFile == NULL )
        {
            printf("Error: Cannot create destination file '%s'\n", destPath);
            Dmod_FileClose(srcFile);
            Dmod_Dependencies_Free(dep_ctx);
            return -1;
        }
        
        // Copy in 64KB chunks for efficiency
        #define COPY_BUFFER_SIZE (64 * 1024)
        void* buffer = Dmod_Malloc(COPY_BUFFER_SIZE);
        if( buffer == NULL )
        {
            printf("Error: Cannot allocate copy buffer\n");
            Dmod_FileClose(srcFile);
            Dmod_FileClose(dstFile);
            Dmod_Dependencies_Free(dep_ctx);
            return -1;
        }
        
        size_t bytesRead;
        bool copySuccess = true;
        while( (bytesRead = Dmod_FileRead(buffer, 1, COPY_BUFFER_SIZE, srcFile)) > 0 )
        {
            if( Dmod_FileWrite(buffer, 1, bytesRead, dstFile) != bytesRead )
            {
                printf("Error: Cannot write to destination file '%s'\n", destPath);
                copySuccess = false;
                break;
            }
        }
        
        Dmod_Free(buffer);
        Dmod_FileClose(srcFile);
        Dmod_FileClose(dstFile);
        
        if( !copySuccess )
        {
            Dmod_Dependencies_Free(dep_ctx);
            return -1;
        }
    }
    
    Dmod_Dependencies_Free(dep_ctx);
    
    // Use first module as main if not specified
    const char* effectiveMainModule = mainModuleName;
    if( effectiveMainModule == NULL && firstModuleName != NULL )
    {
        effectiveMainModule = firstModuleName;
        printf("Using first module '%s' as main module\n", effectiveMainModule);
    }
    
    // Create the DMP package from temp directory
    printf("\nCreating DMP package...\n");
    printf("  Package name: %s\n", packageName);
    printf("  Input directory: %s\n", tempDir);
    printf("  Output file: %s\n", outputFile);
    if( effectiveMainModule != NULL )
    {
        printf("  Main module: %s\n", effectiveMainModule);
    }
    
    bool result = Dmod_ToDMPFile(packageName, tempDir, outputFile, effectiveMainModule);
    
    // Clean up temporary directory
    void* dir = Dmod_OpenDir(tempDir);
    if( dir != NULL )
    {
        const char* fileName;
        while( (fileName = Dmod_ReadDir(dir)) != NULL )
        {
            // Skip . and .. entries
            if( strcmp(fileName, ".") == 0 || strcmp(fileName, "..") == 0 )
            {
                continue;
            }
            
            char filePath[DMOD_MAX_PATH_LENGTH];
            snprintf(filePath, sizeof(filePath), "%s/%s", tempDir, fileName);
            unlink(filePath);
        }
        Dmod_CloseDir(dir);
    }
    rmdir(tempDir);
    
    if( !result )
    {
        printf("Error: Failed to create DMP package\n");
        return -1;
    }
    
    printf("\nDMP package '%s' was successfully created at '%s'\n", packageName, outputFile);
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
    printf("       %s <package_name> <dmd_file> <input_dir> [output_file] [module_name]\n", AppName);
    printf("       %s -l <package_file>\n", AppName);
    printf("\n");
    printf("Arguments:\n");
    printf("  <package_name>   - Name of the package (for the header)\n");
    printf("  <input_dir>      - Folder with modules to pack (.dmf or .dmfc files)\n");
    printf("  <dmd_file>       - .dmd file specifying which modules to pack\n");
    printf("  [output_file]    - (optional) Path to output .dmp file (default: ./package_name.dmp)\n");
    printf("  [module_name]    - (optional) Name of the main module in the package\n");
    printf("                     (defaults to first module in .dmd file if provided)\n");
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
    printf("\n");
    printf("Examples:\n");
    printf("  %s kernel ./dmfc main-app ./out/kernel.dmp\n", AppName);
    printf("  %s mypackage ./modules\n", AppName);
    printf("  %s mypackage ./modules ./output/mypackage.dmp\n", AppName);
    printf("  %s myapp deps.dmd ./modules ./myapp.dmp\n", AppName);
    printf("  %s myapp deps.dmd ./modules ./myapp.dmp custom-main\n", AppName);
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

    if( argc < 3 )
    {
        printf("Error: Missing required arguments\n");
        PrintUsage( argv[0] );
        return -1;
    }

    if( argc > 6 )
    {
        printf("Error: Too many arguments\n");
        PrintUsage( argv[0] );
        return -1;
    }

    const char* packageName = argv[1];
    const char* secondArg = argv[2];
    const char* inputDir = NULL;
    const char* dmdFilePath = NULL;
    const char* outputFile = NULL;
    const char* mainModuleName = NULL;
    
    // Check if second argument is a .dmd file
    if( IsDmdFile(secondArg) )
    {
        // Format: todmp <package_name> <dmd_file> <input_dir> [output_file] [module_name]
        dmdFilePath = secondArg;
        
        if( argc < 4 )
        {
            printf("Error: Missing input directory\n");
            PrintUsage( argv[0] );
            return -1;
        }
        
        inputDir = argv[3];
        
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
        
        // Optional main module name
        if( argc >= 6 )
        {
            mainModuleName = argv[5];
        }
        
        return CreateDMPFromDmd( packageName, dmdFilePath, inputDir, outputFile, mainModuleName );
    }
    else
    {
        // Format: todmp <package_name> <input_dir> [output_file] [module_name]
        inputDir = secondArg;
        
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
}
