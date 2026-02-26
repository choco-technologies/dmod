#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dmod.h"

// Maximum number of version requirements
#define MAX_VERSION_REQUIREMENTS 64

// Maximum number of per-module manifest mappings
#define MAX_MODULE_MANIFESTS 64

// Maximum number of local manifest module entries
#define MAX_LOCAL_MANIFEST_MODULES 128

// Structure to hold a version requirement
typedef struct {
    char moduleName[64];
    char version[32];
} VersionRequirement_t;

// Structure to hold a per-module manifest mapping
typedef struct {
    char moduleName[64];
    char manifestPath[512];
} ModuleManifest_t;

// -----------------------------------------
//
//      Load version requirements from file
//
// -----------------------------------------
int LoadVersionRequirements( const char* filePath, VersionRequirement_t* requirements, int maxRequirements )
{
    FILE* file = fopen( filePath, "r" );
    if( file == NULL )
    {
        DMOD_LOG_WARN("Cannot open version requirements file: %s\n", filePath);
        return 0;
    }
    
    int count = 0;
    char line[256];
    
    while( fgets( line, sizeof(line), file ) != NULL && count < maxRequirements )
    {
        // Remove newline
        size_t len = strlen(line);
        if( len > 0 && line[len-1] == '\n' )
        {
            line[len-1] = '\0';
        }
        
        // Skip empty lines and comments
        if( line[0] == '\0' || line[0] == '#' )
        {
            continue;
        }
        
        // Parse module@version format
        char* atSign = strchr( line, '@' );
        if( atSign != NULL )
        {
            // Module has version
            size_t nameLen = atSign - line;
            if( nameLen >= sizeof(requirements[count].moduleName) )
            {
                nameLen = sizeof(requirements[count].moduleName) - 1;
            }
            
            strncpy( requirements[count].moduleName, line, nameLen );
            requirements[count].moduleName[nameLen] = '\0';
            
            strncpy( requirements[count].version, atSign + 1, sizeof(requirements[count].version) - 1 );
            requirements[count].version[sizeof(requirements[count].version) - 1] = '\0';
        }
        else
        {
            // Module without version
            strncpy( requirements[count].moduleName, line, sizeof(requirements[count].moduleName) - 1 );
            requirements[count].moduleName[sizeof(requirements[count].moduleName) - 1] = '\0';
            requirements[count].version[0] = '\0';
        }
        
        count++;
    }
    
    fclose( file );
    
    Dmod_Printf("Loaded %d version requirements from %s\n", count, filePath);
    
    return count;
}

// -----------------------------------------
//
//      Find version requirement for a module
//
// -----------------------------------------
const char* FindVersionRequirement( const VersionRequirement_t* requirements, int requirementCount, const char* moduleName )
{
    for( int i = 0; i < requirementCount; i++ )
    {
        if( strcmp( requirements[i].moduleName, moduleName ) == 0 )
        {
            return requirements[i].version[0] != '\0' ? requirements[i].version : NULL;
        }
    }
    return NULL;
}

// -----------------------------------------
//
//      Find manifest path for a module
//
// -----------------------------------------
const char* FindModuleManifest( const ModuleManifest_t* manifests, int manifestCount, const char* moduleName )
{
    for( int i = 0; i < manifestCount; i++ )
    {
        if( strcmp( manifests[i].moduleName, moduleName ) == 0 )
        {
            return manifests[i].manifestPath;
        }
    }
    return NULL;
}

// -----------------------------------------
//
//      Load module names from a local manifest (.dmm) file
//
// -----------------------------------------
int LoadLocalManifestModules( const char* filePath, char moduleNames[][64], int maxModules )
{
    FILE* file = fopen( filePath, "r" );
    if( file == NULL )
    {
        DMOD_LOG_WARN("Cannot open local manifest file: %s\n", filePath);
        return 0;
    }

    int count = 0;
    char line[1024];

    while( fgets( line, sizeof(line), file ) != NULL && count < maxModules )
    {
        // Remove newline
        size_t len = strlen(line);
        if( len > 0 && line[len-1] == '\n' )
        {
            line[len-1] = '\0';
            len--;
        }

        // Skip empty lines, comments and directives
        if( len == 0 || line[0] == '#' || line[0] == '$' )
        {
            continue;
        }

        // Extract module name: everything before '@' or first space
        size_t nameLen = 0;
        while( nameLen < len && line[nameLen] != '@' && line[nameLen] != ' ' && line[nameLen] != '\t' )
        {
            nameLen++;
        }

        if( nameLen == 0 || nameLen >= 64 )
        {
            continue;
        }

        strncpy( moduleNames[count], line, nameLen );
        moduleNames[count][nameLen] = '\0';
        count++;
    }

    fclose( file );

    Dmod_Printf("Loaded %d module names from local manifest %s\n", count, filePath);

    return count;
}

// -----------------------------------------
//
//      Check if a module name is in the local manifest list
//
// -----------------------------------------
bool IsModuleInLocalManifest( char moduleNames[][64], int count, const char* moduleName )
{
    for( int i = 0; i < count; i++ )
    {
        if( strcmp( moduleNames[i], moduleName ) == 0 )
        {
            return true;
        }
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
    printf("Usage: %s path/to/file.dmf [output.dmd] [-r version_requirements.txt] [--from manifest_or_mapping ...] [--local-manifest manifest.dmm]\n", AppName);
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
    printf("  -r <file>             Version requirements file from dmod_link_modules\n");
    printf("  --from <manifest>     Global manifest file/URL added as $from at the top of the .dmd\n");
    printf("  --from <mod>=<manifest>  Per-module manifest: adds inline $from for the named module\n");
    printf("                        Option can be repeated to set manifests for multiple modules\n");
    printf("  --local-manifest <manifest>  Local manifest (.dmm) file; generates a companion\n");
    printf("                        <module>-local.dmd listing only dependencies present in\n");
    printf("                        the manifest, each with an inline $from <manifest> directive\n");
    printf("\nArguments:\n");
    printf("  path/to/file.dmf      Path to the DMF module file\n");
    printf("  [output.dmd]          (optional) Output .dmd file path (default: module_name.dmd)\n");
    printf("\nDescription:\n");
    printf("  The tool loads a module in crossplatform mode, reads its dependencies,\n");
    printf("  and creates a .dmd file listing all non-system required modules.\n");
    printf("  System modules are automatically filtered out.\n");
    printf("\n");
    printf("  If a version requirements file is provided with -r, the tool will merge\n");
    printf("  version information from that file with the dependencies found in the DMF.\n");
    printf("\n");
    printf("  If --from is provided without '=', a global $from directive is written at\n");
    printf("  the top of the .dmd file so all modules are fetched from that manifest.\n");
    printf("  If --from is provided with 'module=manifest', the manifest is added inline\n");
    printf("  only for that specific module using the $from syntax.\n");
    printf("\n");
    printf("  If --local-manifest is provided, a companion <module>-local.dmd file is\n");
    printf("  generated alongside the regular .dmd. This file lists only the dependencies\n");
    printf("  that are present in the local manifest, each with an inline $from <manifest>\n");
    printf("  directive so they can be resolved from the local build.\n");
    printf("\nExamples:\n");
    printf("  %s myapp.dmf                                        # Creates myapp.dmd\n", AppName);
    printf("  %s myapp.dmf dependencies.dmd                       # Creates dependencies.dmd\n", AppName);
    printf("  %s myapp.dmf -r versions.txt                        # Creates myapp.dmd with versions from versions.txt\n", AppName);
    printf("  %s myapp.dmf --from build/manifest.dmm              # Global manifest for all modules\n", AppName);
    printf("  %s myapp.dmf --from dmgpio=build/manifest.dmm \\\n", AppName);
    printf("             --from dmclk=build/manifest2.dmm         # Per-module manifests\n");
    printf("  %s myapp.dmf --local-manifest build/packages/manifest-local.dmm\n", AppName);
    printf("                                                       # Creates myapp.dmd and myapp-local.dmd\n");
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

    // Parse arguments
    const char* dmfPath = NULL;
    const char* outputPath = NULL;
    const char* versionReqsPath = NULL;
    const char* globalManifest = NULL;
    const char* localManifestPath = NULL;
    ModuleManifest_t moduleManifests[MAX_MODULE_MANIFESTS];
    int moduleManifestCount = 0;
    
    // First argument is always the DMF path
    dmfPath = argv[1];
    
    // Parse remaining arguments
    for( int i = 2; i < argc; i++ )
    {
        if( strcmp( argv[i], "-r" ) == 0 )
        {
            // Next argument is the version requirements file
            if( i + 1 < argc )
            {
                versionReqsPath = argv[i + 1];
                i++; // Skip next argument
            }
            else
            {
                printf("Error: -r option requires a file path\n");
                PrintUsage( argv[0] );
                return -1;
            }
        }
        else if( strcmp( argv[i], "--from" ) == 0 )
        {
            if( i + 1 < argc )
            {
                const char* fromArg = argv[i + 1];
                i++; // Skip next argument
                // Check if this is a per-module mapping: module=manifest
                const char* eqSign = strchr( fromArg, '=' );
                if( eqSign != NULL )
                {
                    // Per-module manifest mapping
                    if( moduleManifestCount >= MAX_MODULE_MANIFESTS )
                    {
                        printf("Error: Too many --from module mappings (max %d)\n", MAX_MODULE_MANIFESTS);
                        return -1;
                    }
                    size_t nameLen = (size_t)(eqSign - fromArg);
                    if( nameLen >= sizeof(moduleManifests[moduleManifestCount].moduleName) )
                    {
                        nameLen = sizeof(moduleManifests[moduleManifestCount].moduleName) - 1;
                    }
                    strncpy( moduleManifests[moduleManifestCount].moduleName, fromArg, nameLen );
                    moduleManifests[moduleManifestCount].moduleName[nameLen] = '\0';
                    strncpy( moduleManifests[moduleManifestCount].manifestPath, eqSign + 1,
                             sizeof(moduleManifests[moduleManifestCount].manifestPath) - 1 );
                    moduleManifests[moduleManifestCount].manifestPath[sizeof(moduleManifests[moduleManifestCount].manifestPath) - 1] = '\0';
                    moduleManifestCount++;
                }
                else
                {
                    // Global manifest
                    globalManifest = fromArg;
                }
            }
            else
            {
                printf("Error: --from option requires a manifest path or module=manifest mapping\n");
                PrintUsage( argv[0] );
                return -1;
            }
        }
        else if( strcmp( argv[i], "--local-manifest" ) == 0 )
        {
            if( i + 1 < argc )
            {
                localManifestPath = argv[i + 1];
                i++; // Skip next argument
            }
            else
            {
                printf("Error: --local-manifest option requires a manifest file path\n");
                PrintUsage( argv[0] );
                return -1;
            }
        }
        else if( outputPath == NULL )
        {
            // This is the output path
            outputPath = argv[i];
        }
        else
        {
            printf("Error: Too many arguments\n");
            PrintUsage( argv[0] );
            return -1;
        }
    }
    
    char defaultOutputPath[256];

    // Determine output path
    if( outputPath == NULL )
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

    // Compute the local dmd output path: replace trailing ".dmd" with "-local.dmd"
    char localDmdOutputPath[512];
    if( localManifestPath != NULL )
    {
        size_t outLen = strlen( outputPath );
        if( outLen >= 4 && strcmp( outputPath + outLen - 4, ".dmd" ) == 0 )
        {
            if( outLen - 4 >= sizeof(localDmdOutputPath) - 10 )
            {
                printf("Error: Output path too long for local .dmd generation\n");
                return -1;
            }
            strncpy( localDmdOutputPath, outputPath, outLen - 4 );
            localDmdOutputPath[outLen - 4] = '\0';
            strcat( localDmdOutputPath, "-local.dmd" );
        }
        else
        {
            if( outLen >= sizeof(localDmdOutputPath) - 10 )
            {
                printf("Error: Output path too long for local .dmd generation\n");
                return -1;
            }
            strncpy( localDmdOutputPath, outputPath, outLen );
            localDmdOutputPath[outLen] = '\0';
            strcat( localDmdOutputPath, "-local.dmd" );
        }
    }

    printf("Reading module: %s\n", dmfPath);
    
    // Load version requirements if provided
    VersionRequirement_t versionRequirements[MAX_VERSION_REQUIREMENTS];
    int versionRequirementCount = 0;
    
    if( versionReqsPath != NULL )
    {
        printf("Loading version requirements from: %s\n", versionReqsPath);
        versionRequirementCount = LoadVersionRequirements( versionReqsPath, versionRequirements, MAX_VERSION_REQUIREMENTS );
    }

    // Load local manifest module names if provided
    char localManifestModules[MAX_LOCAL_MANIFEST_MODULES][64];
    int localManifestModuleCount = 0;

    if( localManifestPath != NULL )
    {
        printf("Loading local manifest from: %s\n", localManifestPath);
        localManifestModuleCount = LoadLocalManifestModules( localManifestPath, localManifestModules, MAX_LOCAL_MANIFEST_MODULES );
    }

    // Initialize Dmod system
    if (!Dmod_Initialize(0, 0))
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
    Dmod_Printf("Module name: %s\n", moduleName ? moduleName : "<unknown>");

    // Open output file
    void* outputFile = Dmod_FileOpen( outputPath, "w" );
    if( outputFile == NULL )
    {
        DMOD_LOG_ERROR("Cannot create output file: %s\n", outputPath);
        Dmod_Unload( context, false );
        Dmod_Deinitialize();
        return -1;
    }

    // Write header comment to the .dmd file
    Dmod_FPrintf( outputFile, "# DMOD Dependencies File\n" );
    Dmod_FPrintf( outputFile, "# Generated from module: %s\n", moduleName ? moduleName : "<unknown>" );
    Dmod_FPrintf( outputFile, "# Source file: %s\n", dmfPath );
    Dmod_FPrintf( outputFile, "#\n");
    Dmod_FPrintf( outputFile, "# This file lists all non-system modules required by the module.\n" );
    Dmod_FPrintf( outputFile, "# Use with dmf-get: dmf-get -d %s\n", outputPath );
    Dmod_FPrintf( outputFile, "\n" );

    // Write global $from directive if provided
    if( globalManifest != NULL )
    {
        Dmod_FPrintf( outputFile, "$from %s\n\n", globalManifest );
        Dmod_Printf("Using global manifest: %s\n", globalManifest);
    }

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
                reqModule = Dmod_GetNextRequiredModule( context, reqModule );
                continue;
            }
            
            // Check if there's a version requirement for this module
            const char* requiredVersion = FindVersionRequirement( versionRequirements, versionRequirementCount, reqModule->Name );
            
            // Determine which version to use:
            // 1. If both DMF and requirements file specify a version, use the requirements file version (developer's explicit choice)
            // 2. If only DMF has version, use soft range constraint (>=version<(major+1).0) since the version is auto-discovered
            // 3. If only requirements file has version, use requirements file version
            // 4. If neither has version, write module without version
            
            const char* versionToUse = NULL;
            bool hasVersionInDmf = (reqModule->Version[0] != '\0');
            char softVersionBuf[64];
            
            if( requiredVersion != NULL )
            {
                // Requirements file has version, use it (takes precedence)
                versionToUse = requiredVersion;
                if( hasVersionInDmf && strcmp( reqModule->Version, requiredVersion ) != 0 )
                {
                    DMOD_LOG_INFO("Overriding DMF version '%s' with requirement version '%s' for module: %s\n", 
                                  reqModule->Version, requiredVersion, reqModule->Name);
                }
            }
            else if( hasVersionInDmf )
            {
                // Only DMF has version (auto-discovered, not user-specified)
                // Use a soft range constraint >=version<(major+1).0 so that
                // newer patch/minor versions are accepted but a different major
                // version (which dmod treats as ABI-incompatible) is rejected.
                // Versions that already carry an operator are used as-is.
                const char* v = reqModule->Version;
                if( v[0] == '>' || v[0] == '<' || v[0] == '=' )
                {
                    // Already has a constraint operator, use as-is
                    versionToUse = v;
                }
                else
                {
                    // Plain version number - convert to soft range constraint:
                    // >=version<(major+1).0 so that only the same major version
                    // series is accepted (dmod refuses cross-major-version APIs)
                    long major = strtol( v, NULL, 10 );
                    snprintf( softVersionBuf, sizeof(softVersionBuf), ">=%s<%ld.0", v, major + 1 );
                    versionToUse = softVersionBuf;
                }
            }
            
            // Check if there's a per-module manifest for this module
            const char* moduleManifest = FindModuleManifest( moduleManifests, moduleManifestCount, reqModule->Name );

            // Write module to .dmd file
            if( versionToUse != NULL )
            {
                if( moduleManifest != NULL )
                {
                    Dmod_FPrintf( outputFile, "%s@%s $from %s\n", reqModule->Name, versionToUse, moduleManifest );
                    Dmod_Printf("  + %s@%s $from %s\n", reqModule->Name, versionToUse, moduleManifest);
                }
                else
                {
                    Dmod_FPrintf( outputFile, "%s@%s\n", reqModule->Name, versionToUse );
                    Dmod_Printf("  + %s@%s\n", reqModule->Name, versionToUse);
                }
            }
            else
            {
                if( moduleManifest != NULL )
                {
                    Dmod_FPrintf( outputFile, "%s $from %s\n", reqModule->Name, moduleManifest );
                    Dmod_Printf("  + %s $from %s\n", reqModule->Name, moduleManifest);
                }
                else
                {
                    Dmod_FPrintf( outputFile, "%s\n", reqModule->Name );
                    Dmod_Printf("  + %s\n", reqModule->Name);
                }
            }
            moduleCount++;
        }
        
        reqModule = Dmod_GetNextRequiredModule( context, reqModule );
    }

    // Close the output file
    Dmod_FileClose( outputFile );

    // Generate the local .dmd file if a local manifest was provided
    if( localManifestPath != NULL )
    {
        Dmod_Printf("\nGenerating local dependencies file: %s\n", localDmdOutputPath);

        void* localOutputFile = Dmod_FileOpen( localDmdOutputPath, "w" );
        if( localOutputFile == NULL )
        {
            DMOD_LOG_ERROR("Cannot create local output file: %s\n", localDmdOutputPath);
            Dmod_Unload( context, false );
            Dmod_Deinitialize();
            return -1;
        }

        // Write header comment to the local .dmd file
        Dmod_FPrintf( localOutputFile, "# DMOD Dependencies File\n" );
        Dmod_FPrintf( localOutputFile, "# Generated from module: %s\n", moduleName ? moduleName : "<unknown>" );
        Dmod_FPrintf( localOutputFile, "# Source file: %s\n", dmfPath );
        Dmod_FPrintf( localOutputFile, "#\n" );
        Dmod_FPrintf( localOutputFile, "# This file lists all non-system modules required by the module.\n" );
        Dmod_FPrintf( localOutputFile, "# Use with dmf-get: dmf-get -d %s\n", localDmdOutputPath );
        Dmod_FPrintf( localOutputFile, "\n" );

        // Iterate through required modules again for the local .dmd
        int localModuleCount = 0;
        reqModule = Dmod_GetNextRequiredModule( context, NULL );

        while( reqModule != NULL )
        {
            if( reqModule->Name[0] != '\0' )
            {
                if( reqModule->SystemModule )
                {
                    reqModule = Dmod_GetNextRequiredModule( context, reqModule );
                    continue;
                }

                // Determine version (same logic as for the regular .dmd)
                const char* requiredVersion = FindVersionRequirement( versionRequirements, versionRequirementCount, reqModule->Name );
                const char* versionToUse = NULL;
                bool hasVersionInDmf = (reqModule->Version[0] != '\0');
                char softVersionBuf[64];

                if( requiredVersion != NULL )
                {
                    versionToUse = requiredVersion;
                }
                else if( hasVersionInDmf )
                {
                    const char* v = reqModule->Version;
                    if( v[0] == '>' || v[0] == '<' || v[0] == '=' )
                    {
                        versionToUse = v;
                    }
                    else
                    {
                        long major = strtol( v, NULL, 10 );
                        snprintf( softVersionBuf, sizeof(softVersionBuf), ">=%s<%ld.0", v, major + 1 );
                        versionToUse = softVersionBuf;
                    }
                }

                // Use local manifest as $from source if this module is available locally
                bool isLocal = IsModuleInLocalManifest( localManifestModules, localManifestModuleCount, reqModule->Name );

                // Only include modules present in the local manifest (with $from directive)
                if( isLocal )
                {
                    if( versionToUse != NULL )
                    {
                        Dmod_FPrintf( localOutputFile, "%s@%s $from %s\n", reqModule->Name, versionToUse, localManifestPath );
                        Dmod_Printf("  + %s@%s $from %s\n", reqModule->Name, versionToUse, localManifestPath);
                    }
                    else
                    {
                        Dmod_FPrintf( localOutputFile, "%s $from %s\n", reqModule->Name, localManifestPath );
                        Dmod_Printf("  + %s $from %s\n", reqModule->Name, localManifestPath);
                    }
                    localModuleCount++;
                }
            }

            reqModule = Dmod_GetNextRequiredModule( context, reqModule );
        }

        Dmod_FileClose( localOutputFile );
        Dmod_Printf("  Local output file: %s (%d entries)\n", localDmdOutputPath, localModuleCount);
    }

    // Clean up
    Dmod_Unload( context, false );
    Dmod_Deinitialize();

    // Print summary
    Dmod_Printf("\nSummary:\n");
    Dmod_Printf("  Non-system modules: %d\n", moduleCount);
    Dmod_Printf("  System modules (skipped): %d\n", systemModuleCount);
    if( versionRequirementCount > 0 )
    {
        Dmod_Printf("  Version requirements applied: %d\n", versionRequirementCount);
    }
    if( globalManifest != NULL )
    {
        Dmod_Printf("  Global manifest: %s\n", globalManifest);
    }
    if( moduleManifestCount > 0 )
    {
        Dmod_Printf("  Per-module manifest mappings: %d\n", moduleManifestCount);
    }
    Dmod_Printf("  Output file: %s\n", outputPath);
    if( localManifestPath != NULL )
    {
        Dmod_Printf("  Local manifest: %s\n", localManifestPath);
        Dmod_Printf("  Local output file: %s\n", localDmdOutputPath);
    }
    
    if( moduleCount == 0 && systemModuleCount == 0 )
    {
        DMOD_LOG_INFO("No dependencies found in this module.\n");
    }
    else if( moduleCount == 0 )
    {
        DMOD_LOG_INFO("Module only has system dependencies (all were filtered out).\n");
    }
    
    Dmod_Printf("\nSuccess! .dmd file created successfully.\n");

    return 0;
}
