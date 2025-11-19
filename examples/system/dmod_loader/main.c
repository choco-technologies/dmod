#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// For strdup on some systems
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "dmod.h"

// -----------------------------------------
//
//      Check if string looks like a file path
//
// -----------------------------------------
int IsFilePath( const char* Input )
{
    if( Input == NULL )
    {
        return 0;
    }
    
    // Check if contains path separators
    if( strchr( Input, '/' ) != NULL || strchr( Input, '\\' ) != NULL )
    {
        return 1;
    }
    
    // Check if ends with .dmf or .dmfc or .dmp extension
    size_t len = strlen( Input );
    if( len > 4 )
    {
        if( strcmp( Input + len - 4, ".dmf" ) == 0 || strcmp( Input + len - 4, ".dmp" ) == 0 )
        {
            return 1;
        }
    }
    if( len > 5 )
    {
        if( strcmp( Input + len - 5, ".dmfc" ) == 0 )
        {
            return 1;
        }
    }
    
    return 0;
}

// -----------------------------------------
//
//      Try to load module from directory
//
// -----------------------------------------
Dmod_Context_t* TryLoadModuleFromDir( const char* DirPath, const char* ModuleName )
{
    if( DirPath == NULL || ModuleName == NULL )
    {
        return NULL;
    }
    
    char path[512];
    Dmod_Context_t* context = NULL;
    
    // Try uncompressed version (.dmf)
    snprintf( path, sizeof(path), "%s/%s.dmf", DirPath, ModuleName );
    context = Dmod_LoadFile( path );
    if( context != NULL )
    {
        printf("Loaded module '%s' from '%s'\n", ModuleName, path);
        return context;
    }
    
    // Try compressed version (.dmfc)
    snprintf( path, sizeof(path), "%s/%s.dmfc", DirPath, ModuleName );
    context = Dmod_LoadFile( path );
    if( context != NULL )
    {
        printf("Loaded module '%s' from '%s'\n", ModuleName, path);
        return context;
    }
    
    return NULL;
}

// -----------------------------------------
//
//      Load module by name - searches in known paths
//
// -----------------------------------------
Dmod_Context_t* LoadModuleByName( const char* ModuleName )
{
    if( ModuleName == NULL )
    {
        return NULL;
    }
    
    printf("Searching for module '%s'...\n", ModuleName);
    
    Dmod_Context_t* context = NULL;
    
    // 1. First try current directory
    printf("  Checking current directory\n");
    context = TryLoadModuleFromDir( ".", ModuleName );
    if( context != NULL )
    {
        return context;
    }
    
    // 2. Use Dmod_LoadModuleByName which searches:
    //    - DMOD_REPO_PATHS (includes DMOD_DMF_DIR and DMOD_DMFC_DIR)
    //    - Default repository directory
    //    - Precompiled packages
    if( Dmod_LoadModuleByName( ModuleName ) )
    {
        printf("Module '%s' loaded successfully via Dmod_LoadModuleByName\n", ModuleName);
        // Module was loaded but we don't have context directly
        // The caller will handle running/enabling the module by name
        return (Dmod_Context_t*)1; // Marker that module was loaded
    }
    
    printf("Error: Module '%s' not found\n", ModuleName);
    return NULL;
}

// -----------------------------------------
//
//      Prints usage message
//
// -----------------------------------------
void PrintUsage( const char* AppName )
{
    printf("Usage: %s <module_name | path/to/file.dmf> [--module <module_name>] [--args <arguments>]\n", AppName);
    printf("       You can specify either a module name or a full path to a .dmf/.dmfc/.dmp file\n");
}

// -----------------------------------------
//
//      Prints help message
//
// -----------------------------------------
void PrintHelp( const char* AppName )
{
    printf("-- Dynamic Module Loader ver. " DMOD_VERSION_STRING " --\n\n");
    printf("The DMOD is a dynamic module loader that allows to load and unload modules\n");
    printf("This is an example application that uses the DMOD system\n\n");
    printf("Usage: %s <module_name | path/to/file.dmf> [--module <module_name>] [--args <arguments>]\n", AppName);
    printf("Options:\n");
    printf("  -h, --help                Print this help message\n");
    printf("  -v, --version             Print version information\n");
    printf("  --module <module_name>    Specify which module to load from a DMP package\n");
    printf("  --args <arguments>        Arguments to pass to the application module\n\n");
    printf("Module Types:\n");
    printf("  Application    Runs the module's main function\n");
    printf("  Library        Enables the module, then disables it\n\n");
    printf("Module Search Paths (when loading by name):\n");
    printf("  1. Current directory\n");
    printf("  2. DMOD_REPO_PATHS environment variable (multiple paths)\n");
    printf("  3. Default repository directory\n");
    printf("  4. Precompiled packages\n\n");
    printf("Note: DMOD_REPO_PATHS typically includes DMOD_DMF_DIR and DMOD_DMFC_DIR\n\n");
    printf("Examples:\n");
    printf("  %s my-app.dmf                                 # Load by file path\n", AppName);
    printf("  %s my_module                                  # Load by name (searches paths)\n", AppName);
    printf("  %s my-package.dmp --module my_module          # Load from package\n", AppName);
    printf("  %s my-app.dmf --args \"arg1 arg2\"              # Load with arguments\n", AppName);
    printf("  %s my_module --args \"--verbose\"               # Load by name with arguments\n", AppName);
}

// -----------------------------------------
//
//      Main function
//
// -----------------------------------------
int main( int argc, char *argv[] )
{
    // Initialize Dmod system
    if (!Dmod_Initialize())
    {
        printf("Error: Failed to initialize Dmod system\n");
        return -1;
    }

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
    const char* dmfPath = argv[1];
    const char* moduleName = NULL;
    int appArgc = 0;
    char** appArgv = NULL;

    // Look for --module and --args flags
    int moduleIndex = -1;
    int argsIndex = -1;
    for( int i = 2; i < argc; i++ )
    {
        if( strcmp( argv[i], "--module" ) == 0 )
        {
            moduleIndex = i;
        }
        else if( strcmp( argv[i], "--args" ) == 0 )
        {
            argsIndex = i;
            // Don't break here to allow detecting all flags
        }
    }

    // Get module name if --module flag was provided
    if( moduleIndex != -1 )
    {
        if( argc <= moduleIndex + 1 )
        {
            printf("Error: --module flag requires a module name\n");
            PrintUsage( argv[0] );
            return -1;
        }
        moduleName = argv[moduleIndex + 1];
    }

    // Prepare arguments to pass to the module
    // Check if --args flag was found and there are arguments after it
    if( argsIndex != -1 && argc > argsIndex + 1 )
    {
        // Arguments start after --args flag
        appArgc = argc - argsIndex - 1;
        appArgv = &argv[argsIndex + 1];
    }
    else
    {
        // No --args flag or no arguments after it, pass empty arguments
        appArgc = 0;
        appArgv = NULL;
    }

    // Load the module or package
    Dmod_Context_t* context = NULL;
    const char* loadedModuleName = NULL;
    
    // Determine if input is a file path or module name
    if( IsFilePath( dmfPath ) )
    {
        // Input is a file path
        printf("Loading from file path: %s\n", dmfPath);
        
        // Check if the file is a DMP package
        if( Dmod_IsDMPFile( dmfPath ) )
        {
            // If it's a package and no module name specified, load the main module
            if( moduleName == NULL )
            {
                printf("Loading DMP package: %s (main module)\n", dmfPath);
                context = Dmod_LoadFile( dmfPath );
            }
            else
            {
                // Load the specified module from the package
                printf("Loading module '%s' from DMP package: %s\n", moduleName, dmfPath);
                
                // First, add the package to the system
                uint32_t packageIndex = UINT32_MAX;
                if( !Dmod_AddPackageFile( dmfPath, &packageIndex ) )
                {
                    printf("Cannot add DMP package: %s\n", dmfPath);
                    return -1;
                }
                
                // Get package name from the added package
                char packageName[DMOD_MAX_PACKAGE_NAME_LENGTH] = {0};
                size_t packageSize = 0;
                if( !Dmod_GetPackageInfo( packageIndex, packageName, sizeof(packageName), &packageSize ) )
                {
                    printf("Cannot get package info\n");
                    return -1;
                }
                
                // Load the specific module from the package
                context = Dmod_LoadFromPackage( packageName, moduleName );
                loadedModuleName = moduleName;
            }
        }
        else
        {
            // For regular DMF files, ignore --module parameter
            if( moduleName != NULL )
            {
                printf("Warning: --module parameter is only valid for DMP packages, ignoring\n");
            }
            context = Dmod_LoadFile( dmfPath );
        }
    }
    else
    {
        // Input looks like a module name - search in known paths
        printf("Input looks like a module name: %s\n", dmfPath);
        
        if( moduleName != NULL )
        {
            printf("Warning: --module parameter is ignored when loading by module name\n");
        }
        
        context = LoadModuleByName( dmfPath );
        loadedModuleName = dmfPath;
        
        // Special case: if LoadModuleByName returned a marker (not a real context)
        // it means Dmod_LoadModuleByName succeeded, so we need to get the context
        if( context == (Dmod_Context_t*)1 )
        {
            // Module was loaded via Dmod_LoadModuleByName, but we don't have context
            // We'll work with the module by name instead
            loadedModuleName = dmfPath;
            context = NULL; // Will use module name operations below
        }
    }
    
    // If we have neither context nor module name, fail
    if( context == NULL && loadedModuleName == NULL )
    {
        printf("Cannot load module: %s\n", dmfPath);
        return -1;
    }
    
    // Handle case where we loaded by name and need to use module name for operations
    if( context == NULL && loadedModuleName != NULL )
    {
        printf("Module '%s' loaded successfully\n", loadedModuleName);
        
        // Check if it's a library or application using module name
        if( Dmod_IsModuleLoaded( loadedModuleName ) )
        {
            // Try to enable and disable (library pattern)
            printf("Attempting to enable module '%s'...\n", loadedModuleName);
            if( Dmod_EnableModule( loadedModuleName, false, NULL ) )
            {
                printf("Module enabled successfully\n");
                printf("Disabling module...\n");
                if( !Dmod_DisableModule( loadedModuleName, false ) )
                {
                    printf("Warning: Cannot disable module: %s\n", loadedModuleName);
                }
                printf("Module disabled successfully\n");
                Dmod_UnloadModule( loadedModuleName, false );
                return 0;
            }
            else
            {
                // Try to run as application
                printf("Attempting to run module '%s' as application...\n", loadedModuleName);
                int result = Dmod_RunModule( loadedModuleName, appArgc, appArgv );
                Dmod_UnloadModule( loadedModuleName, false );
                return result;
            }
        }
        else
        {
            printf("Error: Module '%s' was not loaded properly\n", loadedModuleName);
            return -1;
        }
    }
    
    // We have a context, use it directly
    const Dmod_RequiredModule_t* reqModule = Dmod_GetNextRequiredModule( context, NULL );
    while( reqModule != NULL)
    {
        DMOD_LOG_INFO("Module '%s' requires module '%s' version '%s'\n", Dmod_GetName(context), reqModule->Name, reqModule->Version );
        reqModule = Dmod_GetNextRequiredModule( context, reqModule );
    }

    // Check module type and handle accordingly
    Dmod_ModuleType_t moduleType = Dmod_GetModuleType( context );
    
    if( moduleType == Dmod_ModuleType_Library )
    {
        // For library modules: enable, then disable
        printf("Module is a library, enabling...\n");
        if( !Dmod_Enable( context, false, NULL ) )
        {
            printf("Cannot enable library module: %s\n", dmfPath);
            Dmod_Unload( context, false );
            return -1;
        }
        printf("Library module enabled successfully\n");
        
        printf("Disabling library module...\n");
        if( !Dmod_Disable( context, false ) )
        {
            printf("Cannot disable library module: %s\n", dmfPath);
            Dmod_Unload( context, false );
            return -1;
        }
        printf("Library module disabled successfully\n");
        Dmod_Unload( context, false );
        return 0;
    }
    else if( moduleType == Dmod_ModuleType_Application )
    {
        // For application modules: run with parsed arguments
        int result = Dmod_Run( context, appArgc, appArgv );
        Dmod_Unload( context, false );
        return result;
    }
    else
    {
        printf("Unknown module type: %d\n", moduleType);
        Dmod_Unload( context, false );
        return -1;
    }
}

