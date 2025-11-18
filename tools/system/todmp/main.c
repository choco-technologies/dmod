#include "dmod.h"
#include "dmod_dependencies.h"
#include <string.h>
#include <time.h>

// -----------------------------------------
//
//      Lists contents of a DMP package
//
// -----------------------------------------
int ListDMPPackage( const char* packageFile )
{
    Dmod_Printf("Reading DMP package: %s\n", packageFile);
    
    // Open the file
    void* file = Dmod_FileOpen( packageFile, "rb" );
    if( file == NULL )
    {
        DMOD_LOG_ERROR("Cannot open file '%s'\n", packageFile);
        return -1;
    }
    
    // Read header
    Dmod_DmpHeader_t header;
    if( Dmod_FileRead( &header, sizeof(header), 1, file ) != 1 )
    {
        DMOD_LOG_ERROR("Cannot read DMP header\n");
        Dmod_FileClose( file );
        return -1;
    }
    
    // Verify signature
    if( header.Signature != DMOD_DMP_SIGNATURE )
    {
        DMOD_LOG_ERROR("Invalid DMP signature (0x%08X)\n", header.Signature);
        Dmod_FileClose( file );
        return -1;
    }
    
    // Print header info
    Dmod_Printf("\nPackage Information:\n");
    Dmod_Printf("  Name: %s\n", header.Name);
    Dmod_Printf("  Version: 0x%04X\n", header.HeaderVersion);
    Dmod_Printf("  Module Count: %u\n", header.ModuleCount);
    Dmod_Printf("  Main Module Index: %u\n", header.MainIndex);
    Dmod_Printf("  Header Size: %u bytes\n", header.HeaderSize);
    
    // Read module entries
    if( header.ModuleCount == 0 )
    {
        Dmod_Printf("\nNo modules in package.\n");
        Dmod_FileClose( file );
        return 0;
    }
    
    Dmod_DmpModuleEntry_t* entries = (Dmod_DmpModuleEntry_t*)Dmod_Malloc( header.ModuleCount * sizeof(Dmod_DmpModuleEntry_t) );
    if( entries == NULL )
    {
        DMOD_LOG_ERROR("Cannot allocate memory for module entries\n");
        Dmod_FileClose( file );
        return -1;
    }
    
    if( Dmod_FileRead( entries, sizeof(Dmod_DmpModuleEntry_t), header.ModuleCount, file ) != header.ModuleCount )
    {
        DMOD_LOG_ERROR("Cannot read module entries\n");
        Dmod_Free( entries );
        Dmod_FileClose( file );
        return -1;
    }
    
    // Print module list
    Dmod_Printf("\nModules:\n");
    for( uint32_t i = 0; i < header.ModuleCount; i++ )
    {
        Dmod_Printf("  [%u] %s\n", i, entries[i].ModuleName);
        Dmod_Printf("      Offset: %u bytes\n", entries[i].ModuleOffset);
        Dmod_Printf("      Size: %u bytes\n", entries[i].FileSize);
        if( i == header.MainIndex )
        {
            Dmod_Printf("      [MAIN MODULE]\n");
        }
    }
    
    Dmod_Free( entries );
    Dmod_FileClose( file );
    
    return 0;
}

// -----------------------------------------
//
//      Helper: Check if file exists in a directory
//
// -----------------------------------------
static bool FileExistsInDir( const char* dir, const char* moduleName, char* outPath, size_t pathSize, const char* targetArch )
{
    if( dir == NULL || moduleName == NULL )
    {
        return false;
    }
    
    // Try .dmf extension first
    Dmod_SnPrintf( outPath, pathSize, "%s/%s.dmf", dir, moduleName );
    if( Dmod_FileAvailable( outPath ) )
    {
        // Verify architecture if needed
        if( targetArch != NULL )
        {
            char fileArch[DMOD_MAX_ARCH_NAME_LENGTH];
            if( Dmod_GetFileArchitecture( outPath, fileArch, sizeof(fileArch) ) )
            {
                if( strcmp( fileArch, targetArch ) == 0 )
                {
                    return true;
                }
                else
                {
                    DMOD_LOG_WARN("Module '%s.dmf' has different architecture ('%s' vs '%s'), skipping\n", moduleName, fileArch, targetArch);
                    // Don't return false - try .dmfc extension
                }
            }
            else
            {
                DMOD_LOG_WARN("Cannot read architecture from '%s', skipping\n", outPath);
                // Don't return false - try .dmfc extension
            }
        }
        else
        {
            return true;
        }
    }
    
    // Try .dmfc extension
    Dmod_SnPrintf( outPath, pathSize, "%s/%s.dmfc", dir, moduleName );
    if( Dmod_FileAvailable( outPath ) )
    {
        // Verify architecture if needed
        if( targetArch != NULL )
        {
            char fileArch[DMOD_MAX_ARCH_NAME_LENGTH];
            if( Dmod_GetFileArchitecture( outPath, fileArch, sizeof(fileArch) ) )
            {
                if( strcmp( fileArch, targetArch ) == 0 )
                {
                    return true;
                }
                else
                {
                    DMOD_LOG_WARN("Module '%s.dmfc' has different architecture ('%s' vs '%s'), skipping\n", moduleName, fileArch, targetArch);
                    return false;
                }
            }
            else
            {
                DMOD_LOG_WARN("Cannot read architecture from '%s', skipping\n", outPath);
                return false;
            }
        }
        else
        {
            return true;
        }
    }
    
    return false;
}

// -----------------------------------------
//
//      Create DMP package with dependencies
//
// -----------------------------------------
int CreatePackageWithDependencies( const char* packageName, const char* mainDmfPath, const char* dmdPath, const char* outputFile, const char* dmfDir, const char* dmfcDir )
{
    if( packageName == NULL || mainDmfPath == NULL || outputFile == NULL )
    {
        DMOD_LOG_ERROR("Invalid parameters\n");
        return -1;
    }
    
    Dmod_Printf("Creating DMP package with dependencies...\n");
    Dmod_Printf("  Package name: %s\n", packageName);
    Dmod_Printf("  Main module: %s\n", mainDmfPath);
    if( dmdPath != NULL )
    {
        Dmod_Printf("  Dependencies file: %s\n", dmdPath);
    }
    Dmod_Printf("  Output file: %s\n", outputFile);
    
    // Initialize Dmod system
    if( !Dmod_Initialize() )
    {
        DMOD_LOG_ERROR("Failed to initialize Dmod system\n");
        return -1;
    }
    
    // Enable cross-platform mode
    Dmod_SetCrossplatformMode( true );
    
    // Get target architecture from main module
    char targetArch[DMOD_MAX_ARCH_NAME_LENGTH];
    if( !Dmod_GetFileArchitecture( mainDmfPath, targetArch, sizeof(targetArch) ) )
    {
        DMOD_LOG_ERROR("Cannot get architecture from main module '%s'\n", mainDmfPath);
        Dmod_Deinitialize();
        return -1;
    }
    Dmod_Printf("  Target architecture: %s\n", targetArch);
    
    // Read dependencies
    Dmod_RequiredModule_t requiredModules[DMOD_MAX_REQUIRED_MODULES];
    memset( requiredModules, 0, sizeof(requiredModules) );
    
    // If DMD file is provided, read from it using dmod_dependencies library
    if( dmdPath != NULL )
    {
        // Initialize dependencies context
        Dmod_DependenciesContext_t* depCtx = Dmod_Dependencies_Init( NULL, NULL, NULL );
        if( depCtx == NULL )
        {
            DMOD_LOG_ERROR("Cannot initialize dependencies parser\n");
            Dmod_Deinitialize();
            return -1;
        }
        
        // Parse DMD file
        if( !Dmod_Dependencies_ParseFile( depCtx, dmdPath ) )
        {
            const char* error = Dmod_Dependencies_GetError( depCtx );
            DMOD_LOG_ERROR("Cannot parse DMD file '%s': %s\n", dmdPath, error ? error : "unknown error");
            Dmod_Dependencies_Free( depCtx );
            Dmod_Deinitialize();
            return -1;
        }
        
        // Get entries from dependencies
        size_t entryCount = Dmod_Dependencies_GetEntryCount( depCtx );
        int moduleIndex = 0;
        
        for( size_t i = 0; i < entryCount && moduleIndex < DMOD_MAX_REQUIRED_MODULES; i++ )
        {
            Dmod_DependencyEntry_t entry;
            if( Dmod_Dependencies_GetEntry( depCtx, i, &entry ) )
            {
                strncpy( requiredModules[moduleIndex].Name, entry.name, DMOD_MAX_MODULE_NAME_LENGTH - 1 );
                requiredModules[moduleIndex].Name[DMOD_MAX_MODULE_NAME_LENGTH - 1] = '\0';
                
                if( entry.version[0] != '\0' )
                {
                    strncpy( requiredModules[moduleIndex].Version, entry.version, DMOD_MAX_VERSION_LENGTH - 1 );
                    requiredModules[moduleIndex].Version[DMOD_MAX_VERSION_LENGTH - 1] = '\0';
                }
                
                requiredModules[moduleIndex].SystemModule = false;
                moduleIndex++;
            }
        }
        
        Dmod_Printf("  Read %d dependencies from DMD file\n", moduleIndex);
        Dmod_Dependencies_Free( depCtx );
    }
    else
    {
        // Read dependencies from DMF file
        if( !Dmod_ReadRequiredModules( mainDmfPath, requiredModules, DMOD_MAX_REQUIRED_MODULES ) )
        {
            DMOD_LOG_WARN("Cannot read dependencies from DMF file, continuing without dependencies\n");
        }
    }
    
    // Get search paths - prefer provided arguments, fall back to environment variables
    const char* searchDmfDir = dmfDir;
    const char* searchDmfcDir = dmfcDir;
    
    if( searchDmfDir == NULL )
    {
        searchDmfDir = Dmod_GetEnv( "DMOD_DMF_DIR" );
    }
    if( searchDmfcDir == NULL )
    {
        searchDmfcDir = Dmod_GetEnv( "DMOD_DMFC_DIR" );
    }
    
    if( searchDmfDir != NULL )
    {
        Dmod_Printf("  DMF search directory: %s\n", searchDmfDir);
    }
    if( searchDmfcDir != NULL )
    {
        Dmod_Printf("  DMFC search directory: %s\n", searchDmfcDir);
    }
    
    // Collect all module files to include
    char modulePaths[DMOD_MAX_REQUIRED_MODULES + 1][DMOD_MAX_PATH_LENGTH];
    int moduleCount = 0;
    
    // Add main module first
    strncpy( modulePaths[moduleCount++], mainDmfPath, DMOD_MAX_PATH_LENGTH - 1 );
    modulePaths[moduleCount - 1][DMOD_MAX_PATH_LENGTH - 1] = '\0';
    
    // Search for required modules
    for( int i = 0; i < DMOD_MAX_REQUIRED_MODULES; i++ )
    {
        if( requiredModules[i].Name[0] == '\0' )
        {
            break;
        }
        
        // Skip system modules
        if( requiredModules[i].SystemModule )
        {
            Dmod_Printf("  Skipping system module: %s\n", requiredModules[i].Name);
            continue;
        }
        
        bool found = false;
        char modulePath[DMOD_MAX_PATH_LENGTH];
        
        // Try DMF_DIR
        if( !found && searchDmfDir != NULL )
        {
            if( FileExistsInDir( searchDmfDir, requiredModules[i].Name, modulePath, sizeof(modulePath), targetArch ) )
            {
                found = true;
                Dmod_Printf("  Found dependency '%s' in DMF directory\n", requiredModules[i].Name);
            }
        }
        
        // Try DMFC_DIR
        if( !found && searchDmfcDir != NULL )
        {
            if( FileExistsInDir( searchDmfcDir, requiredModules[i].Name, modulePath, sizeof(modulePath), targetArch ) )
            {
                found = true;
                Dmod_Printf("  Found dependency '%s' in DMFC directory\n", requiredModules[i].Name);
            }
        }
        
        // Try current directory
        if( !found )
        {
            if( FileExistsInDir( ".", requiredModules[i].Name, modulePath, sizeof(modulePath), targetArch ) )
            {
                found = true;
                Dmod_Printf("  Found dependency '%s' in current directory\n", requiredModules[i].Name);
            }
        }
        
        if( found && moduleCount < DMOD_MAX_REQUIRED_MODULES + 1 )
        {
            strncpy( modulePaths[moduleCount++], modulePath, DMOD_MAX_PATH_LENGTH - 1 );
            modulePaths[moduleCount - 1][DMOD_MAX_PATH_LENGTH - 1] = '\0';
        }
        else if( !found )
        {
            DMOD_LOG_WARN("Dependency '%s' not found, skipping\n", requiredModules[i].Name);
        }
    }
    
    Dmod_Printf("\nTotal modules to pack: %d\n", moduleCount);
    
    // Create temporary directory for all modules
    char tempDir[DMOD_MAX_PATH_LENGTH];
    Dmod_SnPrintf( tempDir, sizeof(tempDir), "/tmp/todmp_%ld", (long)time(NULL) );
    
    if( Dmod_MakeDir( tempDir, 0755 ) != 0 )
    {
        DMOD_LOG_ERROR("Cannot create temporary directory '%s'\n", tempDir);
        Dmod_Deinitialize();
        return -1;
    }
    
    // Copy all files to temp directory
    for( int i = 0; i < moduleCount; i++ )
    {
        // Extract filename from path
        const char* lastSlash = strrchr( modulePaths[i], '/' );
        const char* fileName = lastSlash ? lastSlash + 1 : modulePaths[i];
        
        char destPath[DMOD_MAX_PATH_LENGTH];
        Dmod_SnPrintf( destPath, sizeof(destPath), "%s/%s", tempDir, fileName );
        
        // Copy file
        void* srcFile = Dmod_FileOpen( modulePaths[i], "rb" );
        if( srcFile == NULL )
        {
            DMOD_LOG_ERROR("Cannot open source file '%s'\n", modulePaths[i]);
            Dmod_Deinitialize();
            return -1;
        }
        
        void* dstFile = Dmod_FileOpen( destPath, "wb" );
        if( dstFile == NULL )
        {
            DMOD_LOG_ERROR("Cannot create destination file '%s'\n", destPath);
            Dmod_FileClose( srcFile );
            Dmod_Deinitialize();
            return -1;
        }
        
        // Copy data
        char buffer[4096];
        size_t bytesRead;
        while( (bytesRead = Dmod_FileRead( buffer, 1, sizeof(buffer), srcFile )) > 0 )
        {
            if( Dmod_FileWrite( buffer, 1, bytesRead, dstFile ) != bytesRead )
            {
                DMOD_LOG_ERROR("Error writing to destination file '%s'\n", destPath);
                Dmod_FileClose( srcFile );
                Dmod_FileClose( dstFile );
                Dmod_Deinitialize();
                return -1;
            }
        }
        
        Dmod_FileClose( srcFile );
        Dmod_FileClose( dstFile );
    }
    
    // Extract main module name for the package
    const char* lastSlash = strrchr( mainDmfPath, '/' );
    const char* mainFileName = lastSlash ? lastSlash + 1 : mainDmfPath;
    
    // Remove extension to get module name
    char mainModuleName[DMOD_MAX_MODULE_NAME_LENGTH];
    strncpy( mainModuleName, mainFileName, sizeof(mainModuleName) - 1 );
    mainModuleName[sizeof(mainModuleName) - 1] = '\0';
    char* dot = strchr( mainModuleName, '.' );
    if( dot != NULL )
    {
        *dot = '\0';
    }
    
    // Deinitialize Dmod system before creating DMP package
    // This is important to release any loaded modules and clean up state
    Dmod_Deinitialize();
    
    // Create DMP package from temp directory
    bool success = Dmod_ToDMPFile( packageName, tempDir, outputFile, mainModuleName );
    
    // Cleanup: Remove temporary directory and files
    // Note: This is a simplified cleanup, in production you'd want to recursively remove files
    void* dir = Dmod_OpenDir( tempDir );
    if( dir != NULL )
    {
        const char* fileName;
        while( (fileName = Dmod_ReadDir( dir )) != NULL )
        {
            char filePath[DMOD_MAX_PATH_LENGTH];
            Dmod_SnPrintf( filePath, sizeof(filePath), "%s/%s", tempDir, fileName );
            // Remove file (using system call as Dmod doesn't have remove function)
        }
        Dmod_CloseDir( dir );
    }
    
    if( !success )
    {
        DMOD_LOG_ERROR("Failed to create DMP package\n");
        return -1;
    }
    
    Dmod_Printf("\nDMP package '%s' was successfully created at '%s'\n", packageName, outputFile);
    return 0;
}

// -----------------------------------------
//
//      Prints usage message
//
// -----------------------------------------
void PrintUsage( const char* AppName )
{
    Dmod_Printf("Usage:\n");
    Dmod_Printf("  %s <package_name> <input_dir> [-o output_file] [-m module_name]\n", AppName);
    Dmod_Printf("  %s -d <package_name> <main_dmf> [OPTIONS]\n", AppName);
    Dmod_Printf("  %s -l <package_file>\n", AppName);
    Dmod_Printf("\n");
    Dmod_Printf("Modes:\n");
    Dmod_Printf("  (default)         Create package from all files in directory\n");
    Dmod_Printf("  -d, --deps        Create package with dependency resolution\n");
    Dmod_Printf("  -l, --list        List contents of a DMP package\n");
    Dmod_Printf("\n");
    Dmod_Printf("Options:\n");
    Dmod_Printf("  -o <file>         Output file path (default: ./<package_name>.dmp)\n");
    Dmod_Printf("  -m <name>         Main module name (for directory mode)\n");
    Dmod_Printf("  --dmd <file>      DMD dependencies file (for -d mode)\n");
    Dmod_Printf("  --dmf-dir <dir>   Directory to search for .dmf files\n");
    Dmod_Printf("  --dmfc-dir <dir>  Directory to search for .dmfc files\n");
    Dmod_Printf("  -h, --help        Show this help message\n");
    Dmod_Printf("  -v, --version     Show version information\n");
}

// -----------------------------------------
//
//      Prints help message
//
// -----------------------------------------
void PrintHelp( const char* AppName )
{
    Dmod_Printf("-- DMP Package Creator ver. " DMOD_VERSION_STRING " --\n\n");
    Dmod_Printf("This application allows you to create a DMP package from multiple modules.\n");
    Dmod_Printf("DMP packages can contain multiple DMF or DMFC modules and be loaded together.\n\n");
    PrintUsage( AppName );
    Dmod_Printf("\n");
    Dmod_Printf("Examples:\n");
    Dmod_Printf("\n");
    Dmod_Printf("  Directory mode (pack all files from directory):\n");
    Dmod_Printf("    %s mypackage ./modules\n", AppName);
    Dmod_Printf("    %s mypackage ./modules -o ./out/mypackage.dmp\n", AppName);
    Dmod_Printf("    %s kernel ./dmfc -m main-app -o ./out/kernel.dmp\n", AppName);
    Dmod_Printf("\n");
    Dmod_Printf("  Dependency mode (resolve and include dependencies):\n");
    Dmod_Printf("    %s -d myapp main.dmf --dmf-dir ./modules\n", AppName);
    Dmod_Printf("    %s -d myapp main.dmf --dmd deps.dmd --dmf-dir ./build/dmf -o myapp.dmp\n", AppName);
    Dmod_Printf("    %s -d myapp main.dmf --dmf-dir ./dmf --dmfc-dir ./dmfc\n", AppName);
    Dmod_Printf("\n");
    Dmod_Printf("  List mode:\n");
    Dmod_Printf("    %s -l ./mypackage.dmp\n", AppName);
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

    // Check for help and version first
    for( int i = 1; i < argc; i++ )
    {
        if( strcmp( argv[i], "-h" ) == 0 || strcmp( argv[i], "--help" ) == 0 )
        {
            PrintHelp( argv[0] );
            return 0;
        }
        if( strcmp( argv[i], "-v" ) == 0 || strcmp( argv[i], "--version" ) == 0 )
        {
            Dmod_Printf("Dynamic Module Loader ver. " DMOD_VERSION_STRING "\n");
            return 0;
        }
    }

    // Handle list option
    if( strcmp( argv[1], "-l" ) == 0 || strcmp( argv[1], "--list" ) == 0 )
    {
        if( argc < 3 )
        {
            DMOD_LOG_ERROR("Missing package file argument\n");
            Dmod_Printf("Usage: %s -l <package_file>\n", argv[0]);
            return -1;
        }
        return ListDMPPackage( argv[2] );
    }

    // Handle dependencies option
    if( strcmp( argv[1], "-d" ) == 0 || strcmp( argv[1], "--deps" ) == 0 )
    {
        if( argc < 4 )
        {
            DMOD_LOG_ERROR("Missing required arguments for dependencies mode\n");
            Dmod_Printf("Usage: %s -d <package_name> <main_dmf> [OPTIONS]\n", argv[0]);
            return -1;
        }

        const char* packageName = argv[2];
        const char* mainDmfPath = argv[3];
        const char* dmdPath = NULL;
        const char* outputFile = NULL;
        const char* dmfDir = NULL;
        const char* dmfcDir = NULL;

        // Parse optional arguments
        for( int i = 4; i < argc; i++ )
        {
            if( strcmp( argv[i], "--dmd" ) == 0 && i + 1 < argc )
            {
                dmdPath = argv[++i];
            }
            else if( strcmp( argv[i], "-o" ) == 0 && i + 1 < argc )
            {
                outputFile = argv[++i];
            }
            else if( strcmp( argv[i], "--dmf-dir" ) == 0 && i + 1 < argc )
            {
                dmfDir = argv[++i];
            }
            else if( strcmp( argv[i], "--dmfc-dir" ) == 0 && i + 1 < argc )
            {
                dmfcDir = argv[++i];
            }
            else
            {
                DMOD_LOG_ERROR("Unknown option or missing argument: %s\n", argv[i]);
                PrintUsage( argv[0] );
                return -1;
            }
        }

        // Default output file if not provided
        char defaultOutputFile[256];
        if( outputFile == NULL )
        {
            Dmod_SnPrintf( defaultOutputFile, sizeof(defaultOutputFile), "./%s.dmp", packageName );
            outputFile = defaultOutputFile;
        }

        return CreatePackageWithDependencies( packageName, mainDmfPath, dmdPath, outputFile, dmfDir, dmfcDir );
    }

    // Default mode: create package from directory
    if( argc < 3 )
    {
        DMOD_LOG_ERROR("Missing required arguments\n");
        PrintUsage( argv[0] );
        return -1;
    }

    const char* packageName = argv[1];
    const char* inputDir = argv[2];
    const char* outputFile = NULL;
    const char* mainModuleName = NULL;

    // Parse optional arguments
    for( int i = 3; i < argc; i++ )
    {
        if( strcmp( argv[i], "-o" ) == 0 && i + 1 < argc )
        {
            outputFile = argv[++i];
        }
        else if( strcmp( argv[i], "-m" ) == 0 && i + 1 < argc )
        {
            mainModuleName = argv[++i];
        }
        else
        {
            DMOD_LOG_ERROR("Unknown option or missing argument: %s\n", argv[i]);
            PrintUsage( argv[0] );
            return -1;
        }
    }

    // Default output file: ./package_name.dmp
    char defaultOutputFile[256];
    if( outputFile == NULL )
    {
        Dmod_SnPrintf( defaultOutputFile, sizeof(defaultOutputFile), "./%s.dmp", packageName );
        outputFile = defaultOutputFile;
    }

    Dmod_Printf("Creating DMP package...\n");
    Dmod_Printf("  Package name: %s\n", packageName);
    Dmod_Printf("  Input directory: %s\n", inputDir);
    Dmod_Printf("  Output file: %s\n", outputFile);
    if( mainModuleName != NULL )
    {
        Dmod_Printf("  Main module: %s\n", mainModuleName);
    }

    if( !Dmod_ToDMPFile( packageName, inputDir, outputFile, mainModuleName ) )
    {
        DMOD_LOG_ERROR("Failed to create DMP package\n");
        return -1;
    }

    Dmod_Printf("DMP package '%s' was successfully created at '%s'\n", packageName, outputFile);

    return 0;
}
