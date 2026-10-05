#define DMOD_PRIVATE
#include <string.h>
#include "dmod.h"
#include "private/dmod_vars.h"
#include "private/dmod_ctx.h"
#include "private/dmod_mgr.h"

/**
 * @brief Names of the system modules - the modules the built-in input APIs belong to
 *
 * Packed one after another, each terminated with '\0', the list itself with an
 * empty name. Built once (see Dmod_Mgr_BuildSystemModuleList), so that
 * Dmod_Mgr_IsSystemModule() does not have to check the signature of every
 * built-in API on each call.
 */
static char* SystemModuleNames = NULL;

/**
 * @brief Read the module name of a built-in API signature that names a module
 *
 * @param Signature API signature
 * @param outModuleName Buffer for the module name (DMOD_MAX_MODULE_NAME_LENGTH bytes)
 *
 * @return True if the signature is one that Dmod_ApiSignature_IsModule() accepts
 */
static bool ReadSystemModuleName( const char* Signature, char* outModuleName )
{
    if(
        strncmp( Signature, DMOD_SIGNATURE_PREFIX, sizeof( DMOD_SIGNATURE_PREFIX ) - 1 ) != 0
     && strncmp( Signature, DMOD_BUILTIN_SIGNATURE_PREFIX, sizeof( DMOD_BUILTIN_SIGNATURE_PREFIX ) - 1 ) != 0
     && strncmp( Signature, DMOD_MAL_SIGNATURE_PREFIX, sizeof( DMOD_MAL_SIGNATURE_PREFIX ) - 1 ) != 0
     && strncmp( Signature, DMOD_DIF_SIGNATURE_PREFIX, sizeof( DMOD_DIF_SIGNATURE_PREFIX ) - 1 ) != 0
        )
    {
        return false;
    }

    memset( outModuleName, 0, DMOD_MAX_MODULE_NAME_LENGTH );
    return Dmod_ApiSignature_ReadModuleName( Signature, outModuleName, DMOD_MAX_MODULE_NAME_LENGTH - 1 );
}

/**
 * @brief Find a system module name in the packed list
 *
 * @param Names Packed list of names (see SystemModuleNames)
 * @param ModuleName Name to look for
 * @param Length Number of characters of ModuleName to compare
 *
 * @return True if a name in the list starts with the first Length characters of ModuleName
 */
static bool FindSystemModuleName( const char* Names, const char* ModuleName, size_t Length )
{
    for( const char* name = Names; *name != '\0'; name += strlen( name ) + 1 )
    {
        if( strncmp( name, ModuleName, Length ) == 0 )
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief Add a module name to the packed list of names, unless it is already there
 *
 * @param Names Packed list of names (NULL for an empty one)
 * @param Size Size of the list in bytes, updated on return
 * @param ModuleName Name to add
 *
 * @return The list (possibly reallocated), or NULL on allocation failure
 */
static char* AddSystemModuleName( char* Names, size_t* Size, const char* ModuleName )
{
    size_t length = strlen( ModuleName );
    if( Names != NULL && FindSystemModuleName( Names, ModuleName, length + 1 ) )
    {
        return Names;
    }

    size_t used = ( Names == NULL ) ? 0 : *Size - 1;
    char* names = Dmod_Realloc( Names, used + length + 2 );
    if( names == NULL )
    {
        Dmod_Free( Names );
        return NULL;
    }

    memcpy( &names[used], ModuleName, length + 1 );
    names[used + length + 1] = '\0';
    *Size = used + length + 2;
    return names;
}

/**
 * @brief Build the list of the system module names, if it is not built yet
 *
 * @return True if the list is available
 */
bool Dmod_Mgr_BuildSystemModuleList( void )
{
    if( SystemModuleNames != NULL )
    {
        return true;
    }

    Dmod_BuiltinInputApi.SectionSize = (size_t)((void*)&__dmod_inputs_end - (void*)&__dmod_inputs_start);
    size_t numberOfEntries = Dmod_Api_GetNumberOfEntries( &Dmod_BuiltinInputApi );
    char* names = NULL;
    size_t size = 0;
    char moduleName[DMOD_MAX_MODULE_NAME_LENGTH];
    for(size_t i = 0; i < numberOfEntries; i++)
    {
        const char* signature = Dmod_BuiltinInputApi.InputSection->Entries[i].Signature;
        if( !Dmod_ApiSignature_IsValid( signature ) || !ReadSystemModuleName( signature, moduleName ) || moduleName[0] == '\0' )
        {
            continue;
        }
        names = AddSystemModuleName( names, &size, moduleName );
        if( names == NULL )
        {
            DMOD_LOG_ERROR("Cannot allocate memory for the list of system modules\n");
            return false;
        }
    }

    SystemModuleNames = names;
    return SystemModuleNames != NULL;
}

/**
 * @brief Free the list of the system module names
 */
void Dmod_Mgr_FreeSystemModuleList( void )
{
    Dmod_Free( SystemModuleNames );
    SystemModuleNames = NULL;
}

/**
 * @brief Check if the given module is a system module by checking every built-in API
 *
 * Fallback for when the list of the system module names cannot be built.
 *
 * @param ModuleName Name of the module to check
 *
 * @return True if module is a system module, false otherwise
 */
static bool IsSystemModuleInApis( const char* ModuleName )
{
    Dmod_BuiltinInputApi.SectionSize = (size_t)((void*)&__dmod_inputs_end - (void*)&__dmod_inputs_start);
    size_t numberOfEntries = Dmod_Api_GetNumberOfEntries( &Dmod_BuiltinInputApi );
    for(size_t i = 0; i < numberOfEntries; i++)
    {
        if( Dmod_ApiSignature_IsModule(Dmod_BuiltinInputApi.InputSection->Entries[i].Signature, ModuleName) )
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief checks if the given module is a system module
 * 
 * @param ModuleName Name of the module to check
 * 
 * @return True if module is a system module, false otherwise
 */
bool Dmod_Mgr_IsSystemModule( const char* ModuleName )
{
    if(ModuleName == NULL || ModuleName[0] == 0)
    {
        DMOD_LOG_ERROR("Cannot check if module is system module - invalid module name (empty or NULL)\n");
        return false;
    }
    if( !Dmod_Mgr_BuildSystemModuleList() )
    {
        return IsSystemModuleInApis( ModuleName );
    }
    return FindSystemModuleName( SystemModuleNames, ModuleName, strlen( ModuleName ) );
}

/**
 * @brief Prints all system modules to the log
 */
void Dmod_Mgr_PrintSystemModules( void )
{
    Dmod_BuiltinInputApi.SectionSize = (size_t)((void*)&__dmod_inputs_end - (void*)&__dmod_inputs_start);
    size_t numberOfEntries = Dmod_Api_GetNumberOfEntries( &Dmod_BuiltinInputApi );
    for(size_t i = 0; i < numberOfEntries; i++)
    {
        const char* signature = Dmod_BuiltinInputApi.InputSection->Entries[i].Signature;
        DMOD_LOG_VERBOSE("System module: %s\n", signature);
    }
}

/**
 * @brief Checks if the given module is loaded
 * 
 * @param ModuleName Name of the module to check
 * 
 * @return True if module is loaded, false otherwise
 */
bool Dmod_Mgr_IsLoaded( const char* ModuleName )
{
    /* Loaded modules first: a lookup in the context table is cheap, while
     * Dmod_Mgr_IsSystemModule() checks the signature of every built-in API. */
    if(Dmod_Context_Get( ModuleName ) != NULL)
    {
        return true;
    }

    return Dmod_Mgr_IsSystemModule( ModuleName );
}

/**
 * @brief Checks if the given module is enabled
 * 
 * @param ModuleName Name of the module to check
 * 
 * @return True if module is enabled, false otherwise
 */
bool Dmod_Mgr_IsEnabled( const char* ModuleName )
{
    /* See Dmod_Mgr_IsLoaded() for why loaded modules are checked first */
    Dmod_Context_t* context = Dmod_Context_Get( ModuleName );
    if(context != NULL && Dmod_IsEnabled( context ))
    {
        return true;
    }

    return Dmod_Mgr_IsSystemModule( ModuleName );
}
