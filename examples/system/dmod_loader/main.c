#include <stdio.h>
#include <string.h>
#include "dmod.h"

// -----------------------------------------
//
//      Prints usage message
//
// -----------------------------------------
void PrintUsage( const char* AppName )
{
    printf("Usage: %s path/to/file.dmf [--module <module_name>] [--args <arguments>]\n", AppName);
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
    printf("Usage: %s path/to/file.dmf [--module <module_name>] [--args <arguments>]\n", AppName);
    printf("Options:\n");
    printf("  -h, --help                Print this help message\n");
    printf("  -v, --version             Print version information\n");
    printf("  --module <module_name>    Specify which module to load from a DMP package\n");
    printf("  --args <arguments>        Arguments to pass to the application module\n\n");
    printf("Module Types:\n");
    printf("  Application    Runs the module's main function\n");
    printf("  Library        Enables the module, then disables it\n\n");
    printf("Examples:\n");
    printf("  %s my-app.dmf\n", AppName);
    printf("  %s my-package.dmp --module my_module\n", AppName);
    printf("  %s my-app.dmf --args \"arg1 arg2\"\n", AppName);
    printf("  %s my-package.dmp --module my_module --args \"--verbose\"\n", AppName);
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
    
    if( context == NULL )
    {
        printf("Cannot load module: %s\n", dmfPath);
        return -1;
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

