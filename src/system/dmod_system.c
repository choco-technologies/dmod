#define DMOD_PRIVATE
#define DMOD_ENABLE_REGISTRATION
#include "dmod.h"
#include "private/dmod_vars.h"
#include "private/dmod_ctx.h"
#include "private/dmod_hlp.h"
#include "private/dmod_ldr.h"
#include "private/dmod_mgr.h"
#include "private/dmod_rmod.h"
#include "dmod_system.h"
#include <stdbool.h>
#include <string.h>
#include <errno.h>

//==============================================================================
//                              LOCAL FUNCTION PROTOTYPES
//==============================================================================

static bool ReadFile( const char* ModuleName, void* Data, size_t Size, void* File );
static bool IsAllApiConnected( Dmod_Context_t* Context );
static bool PrepareModulePath( const char* RepoDir, const char* ModuleName, bool Compressed, char* Path, size_t MaxLength );

//==============================================================================
//                              FUNCTION IMPLEMENTATIONS
//==============================================================================

/**
 * @brief Load module
 * 
 * @param Path Path to the module
 * 
 * @return Pointer to the context
 */
Dmod_Context_t* Dmod_LoadFile( const char* Path )
{
    if( Path == NULL )
    {
        DMOD_LOG_ERROR("Cannot load module - invalid path\n");
        return NULL;
    }

    Dmod_Event_ModuleLoadingInProgress( Path, 10 );

    // Open file
    void* file = Dmod_FileOpen( Path, "rb" );
    if( file == NULL )
    {
        DMOD_LOG_ERROR("Cannot load module - cannot open file\n");
        return NULL;
    }

    size_t fileSize = Dmod_FileSize( file );
    if( fileSize == 0 )
    {
        DMOD_LOG_ERROR("Cannot load module - file is empty\n");
        Dmod_FileClose( file );
        return NULL;
    }
    Dmod_Event_ModuleLoadingInProgress( Path, 15 );
    void* buffer = Dmod_AlignedMalloc( fileSize, DMOD_STACK_ALIGNMENT );
    if( buffer == NULL )
    {
        DMOD_LOG_ERROR("Cannot load module - cannot allocate memory\n");
        Dmod_FileClose( file );
        return NULL;
    }

    Dmod_Event_ModuleLoadingInProgress( Path, 20 );

    if(!ReadFile( Path, buffer, fileSize, file ))
    {
        Dmod_FileClose( file );
        return NULL;
    }

    void* dmfData = buffer;
    size_t dmfSize = fileSize;
    if( Dmod_IsDMFC(buffer, fileSize) )
    {
        DMOD_LOG_INFO("Module is compressed - decompressing\n");
        if( !Dmod_FromDMFC(buffer, fileSize, &dmfData, &dmfSize) )
        {
            DMOD_LOG_ERROR("Cannot load module - failed to convert from DMFC\n");
            Dmod_Free( buffer );
            Dmod_FileClose( file );
            return NULL;
        }
        Dmod_Free( buffer );
    }

    Dmod_Event_ModuleLoadingInProgress( Path, 78 );
    Dmod_Context_t* context = Dmod_Context_New( dmfData, dmfSize );
    Dmod_FileClose( file );
    if( context == NULL )
    {
        return NULL;
    }
    if( !Dmod_Ldr_Load( context ) || !Dmod_Context_Add( context ) )
    {
        Dmod_Context_Delete( context );
        return NULL;
    }

    Dmod_PrintAllApis( context );
    Dmod_Event_ModuleLoadingInProgress( Path, 100 );

    DMOD_LOG_INFO("Module loaded: %s\n", Dmod_Context_GetModuleName( context ));

    return context;
}

/**
 * @brief Load module
 * 
 * @param Data Data of the module (aligned to 16 bytes)
 * @param Size Size of the data
 * 
 * @return Pointer to the context
 */
Dmod_Context_t* Dmod_Load( const void* Data, size_t Size )
{
    if( Data == NULL || Size == 0 )
    {
        DMOD_LOG_ERROR("Cannot load module - invalid data\n");
        return NULL;
    }

    Dmod_Event_ModuleLoadingInProgress( "Unknown", 10 );

    void* dmfData = NULL;
    size_t dmfSize = Size;
    if( Dmod_IsDMFC(Data, Size) )
    {
        if( !Dmod_FromDMFC(Data, Size, &dmfData, &dmfSize) )
        {
            DMOD_LOG_ERROR("Cannot load module - failed to convert from DMFC\n");
            return NULL;
        }
    }

    Dmod_Context_t* context = Dmod_Context_New( dmfData, dmfSize );
    if( context == NULL )
    {
        return NULL;
    }

    memcpy( context->Data, Data, Size );

    Dmod_Event_ModuleLoadingInProgress( Dmod_Context_GetModuleName(context), 50 );

    if(!Dmod_Ldr_Load(context) || !Dmod_Context_Add(context))
    {
        Dmod_Context_Delete( context );
        return NULL;
    }
    
    Dmod_PrintAllApis( context );
    Dmod_Event_ModuleLoadingInProgress( Dmod_Context_GetModuleName(context), 100 );

    DMOD_LOG_INFO("Module loaded: %s\n", Dmod_Context_GetModuleName( context ));
    Dmod_Event_ModuleLoaded( context );

    return context;
}

/**
 * @brief Load module by name
 * 
 * @param ModuleName Name of the module
 * 
 * @return True if module was loaded
 */
bool Dmod_LoadModuleByName(const char* ModuleName)
{
    if( ModuleName == NULL )
    {
        DMOD_LOG_ERROR("Cannot load module by name - invalid name\n");
        return false;
    }

    if(Dmod_Mgr_IsLoaded(ModuleName))
    {
        DMOD_LOG_INFO("Module %s is already loaded\n", ModuleName);
        return true;
    }

    char path[DMOD_MAX_PATH_LENGTH + 1] = {0};
    const char* repoPaths = Dmod_GetEnv("DMOD_REPO_PATHS");
    char* repoEnv = NULL;
    size_t repoEnvSize = 0;
    if(repoPaths != NULL)
    {
        repoEnvSize = strlen(repoPaths);
        if(repoEnvSize > 0)
        {
            repoEnv = Dmod_Malloc(repoEnvSize + 1);
            strcpy(repoEnv, repoPaths);
        }
    }
    const char* repoDir = repoEnv != NULL ? strtok(repoEnv, DMOD_ARRAY_SEP) : NULL;
    if( repoDir == NULL )
    {
        DMOD_LOG_VERBOSE("DMOD_REPO_PATH variable is not available. Searching for module '%s' in default repository\n", ModuleName);
        repoDir = Dmod_GetRepoDir();
        repoPaths = NULL;
    }
    do 
    {
        if( repoDir != NULL )
        {
            DMOD_LOG_VERBOSE("Searching for module '%s' in '%s'\n", ModuleName, repoDir);
            if( 
                (
                    PrepareModulePath(repoDir, ModuleName, false, path, sizeof(path))
                 && Dmod_FileAvailable(path)
                 && Dmod_LoadFile(path) != NULL 
                    ) ||
                (
                    PrepareModulePath(repoDir, ModuleName, true, path, sizeof(path))
                 && Dmod_FileAvailable(path)
                 && Dmod_LoadFile(path) != NULL
                    )
                )
            {   
                DMOD_LOG_INFO("Using module '%s' from '%s'\n", ModuleName, path);
                if( repoEnv != NULL )
                {
                    Dmod_Free( repoEnv );
                }
                return true;
            }
        }
        repoDir = NULL;
        if( repoPaths != NULL )
        {
            repoDir = strtok(NULL, DMOD_ARRAY_SEP);
            if( repoDir == NULL )
            {
                repoDir = Dmod_GetRepoDir();
            }
        }
    } while( repoDir != NULL );
    Dmod_Free( repoEnv );
    DMOD_LOG_ERROR("Cannot load module by name - module not found: %s\n", ModuleName);
    return false;
}

/**
 * @brief Unload module
 * 
 * @param Context Context to unload
 */
bool Dmod_Unload( Dmod_Context_t* Context, bool Force )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot unload module - invalid context\n");
        return false;
    }

    Dmod_EnterCritical();
    if(Context->Enabled && !Force)
    {
        DMOD_LOG_ERROR("Module %s cannot be unloaded - it has to be disabled first\n", Dmod_Context_GetModuleName( Context ));
        Dmod_ExitCritical();
        return false;
    }
    if(Context->Running && !Force)
    {
        DMOD_LOG_ERROR("Module %s cannot be unloaded - it has to be stopped first\n", Dmod_Context_GetModuleName( Context ));
        Dmod_ExitCritical();
        return false;
    }

    Dmod_Event_ModuleUnloaded( Context );

    if( !Dmod_Context_Remove( Context ) )
    {
        DMOD_LOG_WARN("Unloading module %s failed - not found\n", Dmod_Context_GetModuleName( Context ));
    }
    Context->Signature = 0;
    Dmod_ExitCritical();
    Dmod_Context_Delete( Context );    

    return true;
}

/**
 * @brief Get stack size
 * 
 * @param Context Context to get stack size from
 * 
 * @return Stack size
 */
uint64_t Dmod_GetStackSize( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot get stack size - invalid context\n");
        return 0;
    }

    return Context->Header->RequiredStackSize;
}

/**
 * @brief Get module type
 * 
 * @param Context Context to get module type from
 * 
 * @return Module type
 */
Dmod_ModuleType_t Dmod_GetModuleType( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot get module type - invalid context\n");
        return Dmod_ModuleType_Unknown;
    }
    if( Context->Header == NULL )
    {
        DMOD_LOG_ERROR("Cannot get module type - header not set\n");
        return Dmod_ModuleType_Unknown;
    }

    return Context->Header->ModuleType;
}

/**
 * @brief Enable module
 * 
 * @param Context Context to enable
 * @param Force If true, the module will be enabled even if it is already enabled or if not all required modules are enabled
 * @param Config Configuration to pass to the module
 * 
 * @return true on success, false on error
 */
bool Dmod_Enable( Dmod_Context_t* Context, bool Force, const Dmod_Config_t* Config )
{
    Dmod_ModuleType_t moduleType = Dmod_GetModuleType(Context);
    if( moduleType != Dmod_ModuleType_Library )
    {
        DMOD_LOG_ERROR("Cannot enable module - invalid module type: %d\n", moduleType);
        return false;
    }

    if(Dmod_Mutex_Lock(Context->Mutex) != 0)
    {
        DMOD_LOG_ERROR("Cannot enable module - cannot lock mutex\n");
        return false;
    }

    if( Context->Enabled && !Force )
    {
        DMOD_LOG_WARN("Module already enabled\n");
        Dmod_Mutex_Unlock(Context->Mutex);
        return true;
    }

    if(!Dmod_RMod_LoadRequiredModules(Context))
    {
        DMOD_LOG_ERROR("Cannot run module - cannot load required modules\n");
        Dmod_Mutex_Unlock(Context->Mutex);
        return -ENOEXEC;
    }

    if(!Dmod_RMod_EnableRequiredModules(Context))
    {
        if(Context->Header->ManualLoad)
        {
            DMOD_LOG_WARN("Running module without all required modules enabled\n");
        }
        else 
        {
            DMOD_LOG_ERROR("Cannot run module - not all required modules are enabled\n");
            Dmod_Mutex_Unlock(Context->Mutex);
            return -ENOEXEC;
        }
    }

    if( !Dmod_RMod_AreRequiredModulesEnabled( Context ) )
    {
        if( !Force )
        {
            DMOD_LOG_ERROR("Cannot enable module - not all required modules are enabled\n");
            Dmod_Mutex_Unlock(Context->Mutex);
            return false;
        }
        else 
        {
            DMOD_LOG_WARN("Not all required modules are enabled for %s\n", Dmod_Context_GetModuleName( Context ));
        }
    }


    if( !Dmod_ConnectAllApis( Context ) )
    {
        DMOD_LOG_ERROR("Cannot enable module - cannot connect APIs\n");
        Dmod_Mutex_Unlock(Context->Mutex);
        return false;
    }

    Dmod_Preinit( Context );

    if(!IsAllApiConnected(Context))
    {
        if(Context->Header->ManualLoad)
        {
            DMOD_LOG_WARN("Running module without all APIs connected\n");
        }
        else 
        {
            DMOD_LOG_ERROR("Cannot run module - not all APIs are connected\n");
            Dmod_DisconnectOutputApis(Context);
            Dmod_Mutex_Unlock(Context->Mutex);
            return -ENOEXEC;
        }
    }

    int result = Dmod_Init( Context, Config );
    if( result != 0 )
    {
        DMOD_LOG_ERROR("Cannot enable module - init failed: %d\n", result);
        Dmod_DisconnectAllApis( Context );
        Dmod_Mutex_Unlock(Context->Mutex);
        return false;
    }

    DMOD_LOG_INFO("Module enabled: %s\n", Dmod_Context_GetModuleName( Context ));
    Context->Enabled = true;

    Dmod_Event_ModuleEnabled( Context );

    Dmod_Mutex_Unlock(Context->Mutex);
    return true;
}

/**
 * @brief Disable module
 * 
 * @param Context Context to disable
 * @param Force If true, the module will be disabled even if it is already disabled
 * 
 * @return true on success, false on error
 */
bool Dmod_Disable( Dmod_Context_t* Context, bool Force )
{
    Dmod_ModuleType_t moduleType = Dmod_GetModuleType(Context);
    if( moduleType != Dmod_ModuleType_Library )
    {
        DMOD_LOG_ERROR("Cannot disable module - invalid module type: %d\n", moduleType);
        return false;
    }

    if(Dmod_Mutex_Lock(Context->Mutex) != 0)
    {
        DMOD_LOG_ERROR("Cannot disable module - cannot lock mutex\n");
        return false;
    }

    if( Context->UsageCounter > 0 && !Force )
    {
        DMOD_LOG_ERROR("Cannot disable module %s - it is used\n", Dmod_Context_GetModuleName( Context ));
        Dmod_Mutex_Unlock(Context->Mutex);
        return false;
    }

    if( !Context->Enabled && !Force )
    {
        DMOD_LOG_WARN("Module already disabled\n");
        Dmod_Mutex_Unlock(Context->Mutex);
        return true;
    }
    Dmod_Context_t* dependentModule = Dmod_RMod_FindDependentModule( Context, true );
    if( dependentModule != NULL )
    {
        if( !Force )
        {
            DMOD_LOG_ERROR("Cannot disable module '%s' - required by module '%s'\n", Dmod_Context_GetModuleName(Context), Dmod_Context_GetModuleName( dependentModule ));
            Dmod_Mutex_Unlock(Context->Mutex);
            return false;
        }
        else 
        {
            DMOD_LOG_WARN("'%s' is required by: %s\n", Dmod_Context_GetModuleName( Context ), Dmod_Context_GetModuleName( dependentModule ));
        }
    }

    Context->Enabled = false;

    int result = Dmod_Deinit( Context );
    if( result != 0 )
    {
        DMOD_LOG_ERROR("Cannot disable module - deinit failed: %d\n", result);
    }

    if( !Dmod_DisconnectAllApis( Context ) )
    {
        DMOD_LOG_ERROR("Cannot disable module - cannot disconnect APIs\n");
        Dmod_Mutex_Unlock(Context->Mutex);
        return false;
    }

    DMOD_LOG_INFO("Module disabled: %s\n", Dmod_Context_GetModuleName( Context ));
    Dmod_Event_ModuleDisabled( Context );

    Dmod_Mutex_Unlock(Context->Mutex);
    return true;
}

/**
 * @brief Check if module is enabled
 * 
 * @param Context Context to check
 * 
 * @return true if module is enabled, false otherwise
 */
bool Dmod_IsEnabled( Dmod_Context_t* Context )
{
    return Dmod_Context_IsValid(Context) && Context->Enabled;
}

/**
 * @brief Run application
 * 
 * @param Context Context to run
 * @param argc Number of arguments
 * @param argv Arguments
 * 
 * @return Return value of the main function
 */
int Dmod_Run( Dmod_Context_t* Context, int argc, char *argv[] )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot run module - invalid context\n");
        return -EINVAL;
    }

    if( Dmod_GetModuleType( Context ) != Dmod_ModuleType_Application )
    {
        DMOD_LOG_ERROR("Cannot run module - invalid module type\n");
        return -EINVAL;
    }

    if( Dmod_Mutex_Lock(Context->Mutex) != 0 )
    {
        DMOD_LOG_ERROR("Cannot run module - cannot lock mutex\n");
        return -EINVAL;
    }

    if( Dmod_IsRunning( Context ) )
    {
        DMOD_LOG_ERROR("Module %s already running\n", Dmod_Context_GetModuleName( Context ));
        Dmod_Mutex_Unlock(Context->Mutex);
        return -EEXIST;
    }

    if(!Dmod_RMod_LoadRequiredModules(Context))
    {
        DMOD_LOG_ERROR("Cannot run module - cannot load required modules\n");
        Dmod_Mutex_Unlock(Context->Mutex);
        return -ENOEXEC;
    }

    if(!Dmod_RMod_EnableRequiredModules(Context))
    {
        if(Context->Header->ManualLoad)
        {
            DMOD_LOG_WARN("Running module without all required modules enabled\n");
        }
        else 
        {
            DMOD_LOG_ERROR("Cannot run module - not all required modules are enabled\n");
            Dmod_Mutex_Unlock(Context->Mutex);
            return -ENOEXEC;
        }
    }

    if(!Dmod_ConnectOutputApis(Context))
    {
        DMOD_LOG_ERROR("Cannot run module - cannot connect output APIs\n");
        Dmod_Mutex_Unlock(Context->Mutex);
        return -ENOEXEC;
    }

    Context->Running = true;
    Dmod_Event_ModuleRunning( Context );
    Dmod_Preinit( Context );

    if(!IsAllApiConnected(Context))
    {
        if(Context->Header->ManualLoad)
        {
            DMOD_LOG_WARN("Running module without all APIs connected\n");
        }
        else 
        {
            DMOD_LOG_ERROR("Cannot run module - not all APIs are connected\n");
            Dmod_DisconnectOutputApis(Context);
            Dmod_Mutex_Unlock(Context->Mutex);
            return -ENOEXEC;
        }
    }
    int result = Dmod_Init( Context, NULL );
    if( result != 0 )
    {
        DMOD_LOG_ERROR("Cannot run module - init failed: %d\n", result);
        Dmod_DisconnectOutputApis(Context);
        Dmod_Mutex_Unlock(Context->Mutex);
        return -ENOEXEC;
    }

    result = Dmod_Main( Context, argc, argv );
    Dmod_Event_ModuleStopped( Context );
    Context->Running = false;

    result = Dmod_Deinit( Context );
    if( result != 0 )
    {
        DMOD_LOG_ERROR("Cannot run module - deinit failed: %d\n", result);
    }

    Dmod_DisconnectOutputApis(Context);

    Dmod_Mutex_Unlock(Context->Mutex);
    return result;
}

/**
 * @brief Check if module is running
 * 
 * @param Context Context to check
 * 
 * @return true if module is running, false otherwise
 */
bool Dmod_IsRunning( Dmod_Context_t* Context )
{
    return Dmod_Context_IsValid(Context) && Context->Running;
}

/**
 * @brief Returns license of the module
 * 
 * @param Context Context to get license from
 * 
 * @return Pointer to the license
 */
Dmod_License_t* Dmod_GetLicense( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot get license - invalid context\n");
        return NULL;
    }

    if( Context->Header == NULL )
    {
        DMOD_LOG_ERROR("Cannot get license - header not set\n");
        return NULL;
    }

    return Context->Header->License;
}

/**
 * @brief Read module header
 * 
 * @param FilePath Path to the module file
 * @param Header Buffer to store module header
 * 
 * @return true if header was read successfully, false otherwise
 */
bool Dmod_ReadModuleHeader(const char* FilePath, Dmod_ModuleHeader_t* Header)
{
    if( FilePath == NULL || Header == NULL )
    {
        return false;
    }

    void* file = Dmod_FileOpen( FilePath, "rb" );
    if( file == NULL )
    {
        DMOD_LOG_ERROR("Cannot read module header - cannot open file\n");
        return false;
    }

    if( Dmod_FileRead( Header, sizeof(Dmod_ModuleHeader_t), 1, file ) != 1 )
    {
        DMOD_LOG_ERROR("Cannot read module header - cannot read header\n");
        Dmod_FileClose( file );
        return false;
    }

    Dmod_FileClose( file );

    return true;
}

/**
 * @brief Get context by module name
 * 
 * @param ModuleName Name of the module
 * 
 * @return Pointer to the context
 */
void Dmod_BeginUsage( const char* ModuleName )
{
    if(Dmod_Mgr_IsSystemModule(ModuleName))
    {
        return;
    }

    Dmod_Context_t* context = Dmod_Context_Get( ModuleName );
    if( context == NULL )
    {
        DMOD_LOG_ERROR("Cannot begin usage - module not found: %s\n", ModuleName);
        return;
    }

    if( Dmod_Mutex_Lock(context->Mutex) != 0 )
    {
        DMOD_LOG_ERROR("Cannot begin usage - cannot lock mutex\n");
        return;
    }

    context->UsageCounter++;
    Dmod_Mutex_Unlock(context->Mutex);
}

/**
 * @brief End usage of the module
 * 
 * @param ModuleName Name of the module
 */
void Dmod_EndUsage( const char* ModuleName )
{
    if(Dmod_Mgr_IsSystemModule(ModuleName))
    {
        return;
    }

    Dmod_Context_t* context = Dmod_Context_Get( ModuleName );
    if( context == NULL )
    {
        DMOD_LOG_ERROR("Cannot end usage - module not found: %s\n", ModuleName);
        return;
    }

    if( Dmod_Mutex_Lock(context->Mutex) != 0 )
    {
        DMOD_LOG_ERROR("Cannot end usage - cannot lock mutex\n");
        return;
    }

    if( context->UsageCounter > 0 )
    {
        context->UsageCounter--;
    }
    else 
    {
        DMOD_LOG_WARN("Usage counter is already 0\n");
    }

    Dmod_Mutex_Unlock(context->Mutex);
}

/**
 * @brief Check if module is used
 * 
 * @param ModuleName Name of the module
 * 
 * @return true if module is used, false otherwise
 */
bool Dmod_IsModuleUsed( const char* ModuleName )
{
    if(Dmod_Mgr_IsSystemModule(ModuleName))
    {
        return true;
    }

    Dmod_Context_t* context = Dmod_Context_Get( ModuleName );
    if( context == NULL )
    {
        DMOD_LOG_ERROR("Cannot check if module is used - module not found: %s\n", ModuleName);
        return false;
    }

    if( Dmod_Mutex_Lock(context->Mutex) != 0 )
    {
        DMOD_LOG_ERROR("Cannot check if module is used - cannot lock mutex\n");
        return false;
    }

    bool result = context->UsageCounter > 0;

    Dmod_Mutex_Unlock(context->Mutex);
    return result;
}

/**
 * @brief Check if module is loaded
 * 
 * @param ModuleName Name of the module
 * 
 * @return true if module is loaded, false otherwise
 */
bool Dmod_IsModuleLoaded(const char* ModuleName)
{
    return Dmod_Mgr_IsLoaded(ModuleName);
}

/**
 * @brief Check if module is enabled
 * 
 * @param ModuleName Name of the module
 * 
 * @return true if module is enabled, false otherwise
 */
bool Dmod_IsApplicationModuleFile(const char* FilePath)
{
    Dmod_ModuleHeader_t header;
    if( !Dmod_ReadModuleHeader( FilePath, &header ) )
    {
        DMOD_LOG_ERROR("Cannot check if module file is application - cannot read header\n");
        return false;
    }

    return header.ModuleType == Dmod_ModuleType_Application;
}

/**
 * @brief Check if module file is loaded
 */
bool Dmod_IsModuleFileLoaded(const char* FilePath)
{
    Dmod_ModuleHeader_t header;
    if( !Dmod_ReadModuleHeader( FilePath, &header ) )
    {
        DMOD_LOG_ERROR("Cannot check if module file is loaded - cannot read header\n");
        return false;
    }

    return Dmod_Mgr_IsLoaded( header.Name );
}

/**
 * @brief Check if module is enabled
 * 
 * @param ModuleName Name of the module
 * 
 * @return true if module is enabled, false otherwise
 */
bool Dmod_IsModuleEnabled(const char* ModuleName)
{
    return Dmod_Mgr_IsEnabled(ModuleName);
}

/**
 * @brief Check if module is required
 * 
 * @param ModuleName Name of the module
 * @param RequiredModuleName Name of the required module
 * 
 * @return true if module is required, false otherwise
 */
bool Dmod_IsModuleRequired(const char* ModuleName, const char* RequiredModuleName)
{
    if( ModuleName == NULL || RequiredModuleName == NULL )
    {
        return false;
    }

    Dmod_Context_t* context = Dmod_Context_Get( ModuleName );
    if( context == NULL )
    {
        return false;
    }

    return Dmod_RMod_IsModuleRequired( context, RequiredModuleName );
}

/**
 * @brief Get module version
 * 
 * @param ModuleName Name of the module
 * 
 * @return Module type
 */
const char* Dmod_GetModuleVersion(const char* ModuleName)
{
    if( Dmod_Mgr_IsSystemModule( ModuleName ) )
    {
        return DMOD_SYSTEM_VERSION_STRING;
    }

    Dmod_Context_t* context = Dmod_Context_Get( ModuleName );
    if( context == NULL )
    {
        return 0;
    }

    return context->Header->Version;
}

/**
 * @brief loads module
 * 
 * @param ModuleName Name of the module
 * 
 * @return true if module was loaded successfully, false otherwise
 */
bool Dmod_LoadModule(const char* FilePath)
{
    if( FilePath == NULL )
    {
        DMOD_LOG_ERROR("Cannot load module - invalid module name\n");
        return false;
    }

    Dmod_Context_t* context = Dmod_LoadFile( FilePath );
    if( context == NULL )
    {
        DMOD_LOG_ERROR("Cannot load module: %s\n", FilePath);
        return false;
    }

    return true;
}

/**
 * @brief Unload module
 * 
 * @param ModuleName Name of the module
 * 
 * @return true if module was unloaded successfully, false otherwise
 */
bool Dmod_UnloadModule(const char* ModuleName, bool Force)
{
    if( ModuleName == NULL )
    {
        DMOD_LOG_ERROR("Cannot unload module - invalid module name\n");
        return false;
    }

    Dmod_Context_t* context = Dmod_Context_Get( ModuleName );
    if( context == NULL )
    {
        DMOD_LOG_ERROR("Cannot unload module - module not found: %s\n", ModuleName);
        return false;
    }

    return Dmod_Unload( context, Force );
}

/**
 * @brief Enable module
 * 
 * @param ModuleName Name of the module
 * @param Force If true, the module will be enabled even if it is already enabled or if not all required modules are enabled
 * @param Config Configuration to pass to the module
 * 
 * @return true if module was enabled successfully, false otherwise
 */
bool Dmod_EnableModule(const char* ModuleName, bool Force, const Dmod_Config_t* Config)
{
    if( ModuleName == NULL )
    {
        DMOD_LOG_ERROR("Cannot enable module - invalid module name\n");
        return false;
    }

    Dmod_Context_t* context = Dmod_Context_Get( ModuleName );
    if( context == NULL )
    {
        DMOD_LOG_ERROR("Cannot enable module - module not found: %s\n", ModuleName);
        return false;
    }

    return Dmod_Enable( context, Force, Config );
}

/**
 * @brief Disable module
 * 
 * @param ModuleName Name of the module
 * @param Force If true, the module will be disabled even if it is already disabled
 * 
 * @return true if module was disabled successfully, false otherwise
 */
bool Dmod_DisableModule(const char* ModuleName, bool Force)
{
    if( ModuleName == NULL )
    {
        DMOD_LOG_ERROR("Cannot disable module - invalid module name\n");
        return false;
    }

    Dmod_Context_t* context = Dmod_Context_Get( ModuleName );
    if( context == NULL )
    {
        DMOD_LOG_ERROR("Cannot disable module - module not found: %s\n", ModuleName);
        return false;
    }

    return Dmod_Disable( context, Force );
}

/**
 * @brief Run application
 * 
 * @param ModuleName Name of the module
 * @param argc Number of arguments
 * @param argv Arguments
 * 
 * @return Return value of the main function
 */
int Dmod_RunModule(const char* ModuleName, int argc, char *argv[])
{
    if( ModuleName == NULL )
    {
        DMOD_LOG_ERROR("Cannot run module - invalid module name\n");
        return -EINVAL;
    }

    Dmod_Context_t* context = Dmod_Context_Get( ModuleName );
    if( context == NULL )
    {
        DMOD_LOG_ERROR("Cannot run module - module not found: %s\n", ModuleName);
        return -EINVAL;
    }

    return Dmod_Run( context, argc, argv );
}

/**
 * @brief Check if function is connected
 * 
 * @param FunctionPointer Function pointer to check
 * 
 * @return true if function is connected, false otherwise
 */
bool Dmod_IsFunctionConnected( void* FunctionPointer )
{
    return FunctionPointer != NULL && !Dmod_ApiSignature_IsValid( FunctionPointer );
}

//==============================================================================
//                              LOCAL FUNCTIONS IMPLEMENTATIONS
//==============================================================================

/**
 * @brief Read file
 * 
 * @param ModuleName Name of the module (just for event logging)
 * @param Data Destination for data to read
 * @param Size Size of the buffer 
 * @param File File to read
 * 
 * @return True if file was read successfully, false otherwise
 */
static bool ReadFile( const char* ModuleName, void* Data, size_t Size, void* File )
{
    if( Data == NULL || File == NULL )
    {
        return false;
    }

    // Seek to the beginning of the file
    if( Dmod_FileSeek( File, 0, DMOD_SEEK_SET ) != 0 )
    {
        DMOD_LOG_ERROR("Cannot read file - cannot seek to the beginning of the file\n");
        return false;
    }

    size_t segmentSize = 1000;
    size_t segments = Size / segmentSize;
    size_t remainder = Size % segmentSize;
    size_t progress = 20;
    size_t progressRange = 55;

    for(size_t i = 0; i < segments; i++)
    {
        size_t read = Dmod_FileRead( Data + i * segmentSize, 1, segmentSize, File );
        if( read != segmentSize )
        {
            DMOD_LOG_ERROR("Cannot read file - not all data read: %d\n", read);
            return false;
        }
        Dmod_Event_ModuleLoadingInProgress( ModuleName, progress + ((progressRange * (i+1))/segments) );
    }

    size_t read = Dmod_FileRead( Data + segments * segmentSize, 1, remainder, File );
    if( read != remainder )
    {
        DMOD_LOG_ERROR("Cannot read file - not all data read: %d\n", read);
        return false;
    }
    Dmod_Event_ModuleLoadingInProgress( ModuleName, progress + progressRange );

    return true;
}

/**
 * @brief checks if all output API is connected
 * 
 * @param Context Context to check
 * 
 * @return True if all output API is connected, false otherwise
 */
static bool IsAllApiConnected( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    size_t numberOfOuptuts = Dmod_Api_GetNumberOfEntries( &Context->Outputs );
    for(size_t outputIndex = 0; outputIndex < numberOfOuptuts; outputIndex++)
    {
        const char* apiSignature = Context->Outputs.OutputSection->Entries[outputIndex];
        if(Dmod_ApiSignature_IsValid(apiSignature))
        {
            DMOD_LOG_VERBOSE("API '%s' is not connected\n", apiSignature);
            return false;
        }
    }

    return true;
}

/**
 * @brief Prepare module path
 * 
 * @param RepoDir Repository directory
 * @param ModuleName Name of the module
 * @param Compressed If true, the module is compressed
 * @param Path Destination for the path
 * @param MaxLength Maximum length of the path
 * 
 * @return True if path was prepared successfully, false otherwise
 */
static bool PrepareModulePath( const char* RepoDir, const char* ModuleName, bool Compressed, char* Path, size_t MaxLength )
{
    if( RepoDir == NULL || ModuleName == NULL || Path == NULL )
    {
        return false;
    }

    memset(Path, 0, MaxLength);
    strncpy(Path, RepoDir, MaxLength);
    strncat(Path, "/", MaxLength - strlen(Path));
    strncat(Path, ModuleName, MaxLength - strlen(Path));
    if( Compressed )
    {
        strncat(Path, ".dmfc", MaxLength - strlen(Path));
    }
    else 
    {
        strncat(Path, ".dmf", MaxLength - strlen(Path));
    }

    return true;
}
