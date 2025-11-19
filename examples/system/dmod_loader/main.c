#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "dmod.h"

// -----------------------------------------
//
//      Check if string is a file path or module name
//
// -----------------------------------------
bool IsFilePath( const char* str )
{
    if( str == NULL )
    {
        return false;
    }
    
    // First, check if file exists - if yes, it's a file path
    if( Dmod_FileAvailable( str ) )
    {
        return true;
    }
    
    // If file doesn't exist, check if the name contains only valid module name characters
    // Module names can only contain: a-z, A-Z, 0-9, and underscore
    for( const char* p = str; *p != '\0'; p++ )
    {
        char c = *p;
        if( !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') )
        {
            // Contains invalid character for module name, treat as file path
            return true;
        }
    }
    
    // Valid module name - not a file path
    return false;
}

// -----------------------------------------
//
//      Prints usage message
//
// -----------------------------------------
void PrintUsage( const char* AppName )
{
    printf("Usage: %s <path/to/file.dmf | module_name> [--module <module_name>] [--args <arguments>]\n", AppName);
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
    printf("Usage: %s <path/to/file.dmf | module_name> [--module <module_name>] [--args <arguments>]\n", AppName);
    printf("Options:\n");
    printf("  -h, --help                Print this help message\n");
    printf("  -v, --version             Print version information\n");
    printf("  --module <module_name>    Specify which module to load from a DMP package\n");
    printf("  --args <arguments>        Arguments to pass to the application module\n\n");
    printf("Module Types:\n");
    printf("  Application    Runs the module's main function\n");
    printf("  Library        Enables the module, then disables it\n\n");
    printf("Loading Modes:\n");
    printf("  File Path      If file exists or contains invalid module name characters\n");
    printf("  Module Name    Valid name (a-Z, 0-9, _) that doesn't exist as file\n\n");
    printf("Examples:\n");
    printf("  %s my-app.dmf                                  # Load from file\n", AppName);
    printf("  %s difs                                        # Load module by name\n", AppName);
    printf("  %s my-package.dmp --module my_module          # Load specific module from package\n", AppName);
    printf("  %s my-app.dmf --args \"arg1 arg2\"              # Load file with arguments\n", AppName);
    printf("  %s my_module --args \"--verbose\"               # Load module by name with arguments\n", AppName);
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
    const char* pathOrName = argv[1];
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
    
    // Determine if pathOrName is a file path or module name
    bool isPath = IsFilePath( pathOrName );
    
    if( !isPath )
    {
        // It's a module name, use Dmod_LoadModuleByName
        printf("Loading module by name: %s\n", pathOrName);
        if( !Dmod_LoadModuleByName( pathOrName ) )
        {
            printf("Cannot load module by name: %s\n", pathOrName);
            return -1;
        }
        
        // Store the module name for later operations
        loadedModuleName = pathOrName;
    }
    // Check if the file is a DMP package
    else if( Dmod_IsDMPFile( pathOrName ) )
    {
        // If it's a package and no module name specified, load the main module
        if( moduleName == NULL )
        {
            printf("Loading DMP package: %s (main module)\n", pathOrName);
            context = Dmod_LoadFile( pathOrName );
        }
        else
        {
            // Load the specified module from the package
            printf("Loading module '%s' from DMP package: %s\n", moduleName, pathOrName);
            
            // First, add the package to the system
            uint32_t packageIndex = UINT32_MAX;
            if( !Dmod_AddPackageFile( pathOrName, &packageIndex ) )
            {
                printf("Cannot add DMP package: %s\n", pathOrName);
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
        }
    }
    else
    {
        // For regular DMF files, ignore --module parameter
        if( moduleName != NULL )
        {
            printf("Warning: --module parameter is only valid for DMP packages, ignoring\n");
        }
        context = Dmod_LoadFile( pathOrName );
    }
    
    // Handle module loaded by name
    if( loadedModuleName != NULL )
    {
        // Try to run as application module first
        int result = Dmod_RunModule( loadedModuleName, appArgc, appArgv );
        
        // If it's not an application module, try as library
        if( result == -EINVAL )
        {
            // For library modules: enable, then disable
            printf("Module is a library, enabling...\n");
            if( !Dmod_EnableModule( loadedModuleName, false, NULL ) )
            {
                printf("Cannot enable library module: %s\n", loadedModuleName);
                Dmod_UnloadModule( loadedModuleName, false );
                return -1;
            }
            printf("Library module enabled successfully\n");
            
            printf("Disabling library module...\n");
            if( !Dmod_DisableModule( loadedModuleName, false ) )
            {
                printf("Cannot disable library module: %s\n", loadedModuleName);
                Dmod_UnloadModule( loadedModuleName, false );
                return -1;
            }
            printf("Library module disabled successfully\n");
            Dmod_UnloadModule( loadedModuleName, false );
            return 0;
        }
        
        // Unload the module after running
        Dmod_UnloadModule( loadedModuleName, false );
        return result;
    }
    
    // Handle module loaded from file/package
    if( context == NULL )
    {
        printf("Cannot load module: %s\n", pathOrName);
        return -1;
    }
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
            printf("Cannot enable library module: %s\n", pathOrName);
            Dmod_Unload( context, false );
            return -1;
        }
        printf("Library module enabled successfully\n");
        
        printf("Disabling library module...\n");
        if( !Dmod_Disable( context, false ) )
        {
            printf("Cannot disable library module: %s\n", pathOrName);
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

