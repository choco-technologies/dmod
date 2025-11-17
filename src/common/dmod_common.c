#include "dmod.h"
#include <string.h>

//==============================================================================
//                              LOCAL FUNCTIONS PROTOTYPES
//==============================================================================
static const char* ApiSignature_GetName( const char* Signature );
static const char* ApiSignature_GetVersion( const char* Signature );
static const char* ApiSignature_GetModuleVersion( const char* Signature );
static const char* ApiSignature_GetModule( const char* Signature );
static bool ApiSignature_AreNamesEqual( const char* Signature1, const char* Signature2 );
static bool ApiSignature_AreModulesEqual( const char* Signature1, const char* Signature2 );
static bool ApiSignature_AreVersionsEqual( const char* Signature1, const char* Signature2 );

//==============================================================================
//                              FUNCTION IMPLEMENTATIONS
//==============================================================================

/**
 * @brief Get number of entries in the API
 * 
 * @param Api API to get number of entries from
 * 
 * @return Number of entries in the API
 */
size_t Dmod_Api_GetNumberOfEntries( Dmod_Api_t* Api )
{
    if( Api == NULL )
    {
        return 0;
    }
    
    if( Api->SectionSize == 0 )
    {
        return 0;
    }
    size_t elementSize = 0;

    if( Api->ApiType == Dmod_ApiType_Input )
    {
        elementSize = sizeof(Api->InputSection->Entries[0]);
    }
    else if( Api->ApiType == Dmod_ApiType_Output )
    {
        elementSize = sizeof(Api->OutputSection->Entries[0]);
    }
    else 
    {
        DMOD_LOG_ERROR("Cannot get number of entries - invalid API type\n");
        return 0;
    }
    return Api->SectionSize / elementSize;
}

/**
 * @brief Check if API signature is valid
 * 
 * @param Signature API signature
 * 
 * @return true if signature is valid, false otherwise
 */
bool Dmod_ApiSignature_IsValid( const char* Signature )
{
    if( Signature == NULL )
    {
        return false;
    }

    if( 
        strncmp( Signature, DMOD_SIGNATURE_PREFIX, sizeof( DMOD_SIGNATURE_PREFIX ) - 1 ) != 0 
     && strncmp( Signature, DMOD_IRQ_SIGNATURE_PREFIX, sizeof( DMOD_IRQ_SIGNATURE_PREFIX ) - 1 ) != 0
     && strncmp( Signature, DMOD_MAL_SIGNATURE_PREFIX, sizeof( DMOD_MAL_SIGNATURE_PREFIX ) - 1 ) != 0
     && strncmp( Signature, DMOD_DIF_SIGNATURE_PREFIX, sizeof( DMOD_DIF_SIGNATURE_PREFIX ) - 1 ) != 0
        )
    {
        return false;
    }

    return true;
}

/**
 * @brief Check if module name is given in the API signature
 * 
 * @param Signature API signature
 * 
 * @return true if module name is given, false otherwise
 */
bool Dmod_ApiSignature_IsModuleNameGiven( const char* Signature )
{
    if( Dmod_ApiSignature_IsValid( Signature ) == false )
    {
        return false;
    }

    const char* moduleName = ApiSignature_GetModule( Signature );
    if( moduleName == NULL || moduleName[0] == '\0' || moduleName[0] == ':' )
    {
        return false;
    }

    return true;
}

/**
 * @brief Check if module in the API signature equals to the given module name
 * 
 * @param Signature API signature
 * @param ModuleName Module name to compare
 * 
 * @return true if signature is a module, false otherwise
 */
bool Dmod_ApiSignature_IsModule( const char* Signature, const char* ModuleName  )
{
    if( Dmod_ApiSignature_IsValid( Signature ) == false )
    {
        return false;
    }

    if(
        strncmp( Signature, DMOD_SIGNATURE_PREFIX, sizeof( DMOD_SIGNATURE_PREFIX ) - 1 ) != 0 
     && strncmp( Signature, DMOD_MAL_SIGNATURE_PREFIX, sizeof( DMOD_MAL_SIGNATURE_PREFIX ) - 1 ) != 0
     && strncmp( Signature, DMOD_DIF_SIGNATURE_PREFIX, sizeof( DMOD_DIF_SIGNATURE_PREFIX ) - 1 ) != 0
        )
    {
        return false;
    }

    const char* module = ApiSignature_GetModule( Signature );
    if( module == NULL )
    {
        return false;
    }
    size_t length = strlen( ModuleName ); 
    for( size_t i = 0; i < length; i++ )
    {
        if( module[i] != ModuleName[i] )
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Get API signature name
 * 
 * @param Signature API signature
 * 
 * @return Name of the API signature
 */
const char*  Dmod_ApiSignature_GetName( const char* Signature )
{
    if( Dmod_ApiSignature_IsValid( Signature ) == false )
    {
        return NULL;
    }

    return ApiSignature_GetName( Signature );
}

/**
 * @brief Get API signature version
 * 
 * @param Signature API signature
 * 
 * @return Version of the API signature
 */
const char*  Dmod_ApiSignature_GetVersion( const char* Signature )
{
    if( Dmod_ApiSignature_IsValid( Signature ) == false )
    {
        return NULL;
    }

    return ApiSignature_GetVersion( Signature );
}

/**
 * @brief Get module version from the API signature
 * 
 * @param Signature API signature
 * 
 * @return Module version
 */
const char*  Dmod_ApiSignature_GetModuleVersion( const char* Signature )
{
    if( Dmod_ApiSignature_IsValid( Signature ) == false )
    {
        return NULL;
    }

    return ApiSignature_GetModuleVersion( Signature );
}

/**
 * @brief Get module name from the API signature
 * 
 * @param Signature API signature
 * 
 * @return Module name
 */
const char* Dmod_ApiSignature_GetModule( const char* Signature )
{
    if( Dmod_ApiSignature_IsValid( Signature ) == false )
    {
        return NULL;
    }

    return ApiSignature_GetModule( Signature );
}

/**
 * @brief Reads module name from the API signature
 * 
 * @param Signature API signature
 * @param ModuleName Buffer to store module name
 * @param MaxLength Maximum length of the module name
 * 
 * @return true if module name was read successfully, false otherwise
 */
bool Dmod_ApiSignature_ReadModuleName( const char* Signature, char* ModuleName, size_t MaxLength )
{
    if(MaxLength == 0)
    {
        return false;
    }
    if( !Dmod_ApiSignature_IsValid( Signature ) )
    {
        return false;
    }

    const char* module = ApiSignature_GetModule( Signature );
    if( module == NULL )
    {
        return false;
    }

    while( *module != '\0' && *module != ':' )
    {
        if( MaxLength == 0 )
        {
            return false;
        }
        *ModuleName = *module;
        ModuleName++;
        module++;
        MaxLength--;
    }
    if(MaxLength > 0)
    {
        *ModuleName = '\0';
    }

    return true;
}

/**
 * @brief Reads version from the API signature
 * 
 * @param Signature API signature
 * @param Version Buffer to store version
 * @param MaxLength Maximum length of the version
 * 
 * @return true if version was read successfully, false otherwise
 */
bool Dmod_ApiSignature_ReadVersion( const char* Signature, char* Version, size_t MaxLength )
{
    if(MaxLength == 0)
    {
        return false;
    }
    if( !Dmod_ApiSignature_IsValid( Signature ) )
    {
        return false;
    }

    const char* version = ApiSignature_GetVersion( Signature );
    if( version == NULL )
    {
        return false;
    }

    for(size_t i = 0; i < MaxLength - 1; i++)
    {
        if( version[i] == '\0' || version[i] == '/' )
        {
            Version[i] = '\0';
            return true;
        }
        Version[i] = version[i];
    }

    Version[MaxLength - 1] = '\0';
    return true;
}

/**
 * @brief Reads module version from the API signature
 * 
 * @param Signature API signature
 * @param ModuleVersion Buffer to store module version
 * @param MaxLength Maximum length of the module version
 * 
 * @return true if module version was read successfully, false otherwise
 */
bool Dmod_ApiSignature_ReadModuleVersion( const char* Signature, char* ModuleVersion, size_t MaxLength )
{
    if(MaxLength == 0)
    {
        return false;
    }
    if( !Dmod_ApiSignature_IsValid( Signature ) )
    {
        return false;
    }

    const char* moduleVersion = ApiSignature_GetModuleVersion( Signature );
    if( moduleVersion == NULL )
    {
        return false;
    }

    for(size_t i = 0; i < MaxLength - 1; i++)
    {
        if( moduleVersion[i] == '\0' )
        {
            ModuleVersion[i] = '\0';
            return true;
        }
        ModuleVersion[i] = moduleVersion[i];
    }

    ModuleVersion[MaxLength - 1] = '\0';
    return true;
}

/**
 * @brief Check if API signatures are equal
 * 
 * @param Signature1 First signature
 * @param Signature2 Second signature
 * 
 * @return true if signatures are equal, false otherwise
 */
bool Dmod_ApiSignature_AreEqual( const char* Signature1, const char* Signature2 )
{
    if( !Dmod_ApiSignature_IsValid( Signature1 ) || !Dmod_ApiSignature_IsValid( Signature2 ) )
    {
        return false;
    }

    return ApiSignature_AreNamesEqual( Signature1, Signature2 )
        && ApiSignature_AreModulesEqual( Signature1, Signature2 )
        && ApiSignature_AreVersionsEqual( Signature1, Signature2 );
}

/**
 * @brief Check if API signature is MAL
 * 
 * @param Signature API signature
 * 
 * @return true if signature is MAL, false otherwise
 */
bool Dmod_ApiSignature_IsMal( const char* Signature )
{
    return strncmp( Signature, DMOD_MAL_SIGNATURE_PREFIX, sizeof(DMOD_MAL_SIGNATURE_PREFIX) - 1 ) == 0;
}

//==============================================================================
//                              LOCAL FUNCTIONS IMPLEMENTATIONS
//==============================================================================

/**
 * @brief Get API signature name
 * 
 * @param Signature API signature
 * 
 * @return Name of the API signature
 */
static const char* ApiSignature_GetName( const char* Signature )
{
    const char* name = Signature + sizeof( DMOD_SIGNATURE_PREFIX ) - 1;
    return name;
}

/**
 * @brief Get API signature version
 * 
 * @param Signature API signature
 * 
 * @return Version of the API signature
 */
static const char* ApiSignature_GetVersion( const char* Signature )
{
    const char* version = strchr( Signature, ':' );
    if( version == NULL )
    {
        return NULL;
    }

    return version + 1;
}

/**
 * @brief Get module version from the API signature
 * 
 * @param Signature API signature
 * 
 * @return Module version
 */
static const char* ApiSignature_GetModuleVersion( const char* Signature )
{
    const char* version = ApiSignature_GetVersion( Signature );
    if( version == NULL )
    {
        return NULL;
    }

    const char* moduleVersion = strchr( version, '/' );
    if( moduleVersion == NULL )
    {
        return NULL;
    }

    return moduleVersion + 1;
}

/**
 * @brief Get module name from the API signature
 * 
 * @param Signature API signature
 * 
 * @return Module name
 */
static const char* ApiSignature_GetModule( const char* Signature )
{
    const char* module = strchr( Signature, '@' );
    if( module == NULL )
    {
        return NULL;
    }

    return module + 1;
}

/**
 * @brief Check if names are equal
 * 
 * @param Signature1 First signature
 * @param Signature2 Second signature
 * 
 * @return true if names are equal, false otherwise
 */
static bool ApiSignature_AreNamesEqual( const char* Signature1, const char* Signature2 )
{
    const char* name1 = ApiSignature_GetName( Signature1 );
    const char* name2 = ApiSignature_GetName( Signature2 );

    while( *name1 != '\0' && *name2 != '\0' && *name1 != ':' && *name2 != ':' )
    {
        if( *name1 != *name2 )
        {
            return false;
        }
        name1++;
        name2++;
    }

    return *name1 == *name2;
}

/**
 * @brief Check if modules are equal
 * 
 * @param Signature1 First signature
 * @param Signature2 Second signature
 * 
 * @return true if modules are equal, false otherwise
 */
static bool ApiSignature_AreModulesEqual( const char* Signature1, const char* Signature2 )
{
    const char* module1 = ApiSignature_GetModule( Signature1 );
    const char* module2 = ApiSignature_GetModule( Signature2 );

    while( *module1 != '\0' && *module2 != '\0' && *module1 != '@' && *module2 != '@' )
    {
        if( *module1 != *module2 )
        {
            return false;
        }
        module1++;
        module2++;
    }

    return *module1 == *module2;
}

/**
 * @brief Check if versions are equal
 * 
 * @param Signature1 First signature
 * @param Signature2 Second signature
 * 
 * @return true if versions are equal, false otherwise
 */
static bool ApiSignature_AreVersionsEqual( const char* Signature1, const char* Signature2 )
{
    const char* version1 = ApiSignature_GetVersion( Signature1 );
    const char* version2 = ApiSignature_GetVersion( Signature2 );

    while( *version1 != '\0' && *version2 != '\0' && *version1 != '.' && *version2 != '.' )
    {
        if( *version1 != *version2 )
        {
            DMOD_LOG_ERROR("Version mismatch: %s != %s\n", version1, version2);
            return false;
        }
        version1++;
        version2++;
    }

    return *version1 == *version2;
}