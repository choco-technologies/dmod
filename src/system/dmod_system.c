#define DMOD_PRIVATE
#define DMOD_ENABLE_REGISTRATION
#include "dmod.h"
#include "private/dmod_vars.h"
#include "private/dmod_ctx.h"
#include "private/dmod_hlp.h"
#include "private/dmod_ldr.h"
#include "private/dmod_mgr.h"
#include "private/dmod_rmod.h"
#include "private/dmod_pck.h"
#include "dmod_system.h"
#include <stdbool.h>
#include <string.h>
#include <errno.h>

//==============================================================================
//                              LOCAL FUNCTION PROTOTYPES
//==============================================================================

static bool ReadFile( const char* ModuleName, void* Data, size_t Size, void* File, long FilePos );
static bool IsAllApiConnected( Dmod_Context_t* Context );
static bool PrepareModulePath( const char* RepoDir, const char* ModuleName, bool Compressed, char* Path, size_t MaxLength );
static bool CheckModuleArchitecture( const char* FilePath, const char* ExpectedArch );

//==============================================================================
//                              FUNCTION IMPLEMENTATIONS
//==============================================================================

/**
 * @brief Initialize DMOD system
 * 
 * @return True if initialization was successful, false otherwise
 */
bool Dmod_Initialize(void)
{
    DMOD_LOG_INFO("== dmod ver. " DMOD_VERSION_STRING " ==\n");
    
    if(Dmod_BuiltinInputApi.SectionSize == 0)
    {
        Dmod_BuiltinInputApi.SectionSize = (size_t)((void*)&__dmod_inputs_end - (void*)&__dmod_inputs_start);
        Dmod_BuiltinInputApi.ApiType = Dmod_ApiType_Input;
        Dmod_BuiltinInputApi.InputSection = (Dmod_InputsSection_t*)&__dmod_inputs_start;
    }
    if(Dmod_BuiltinOutputApi.SectionSize == 0)
    {
        Dmod_BuiltinOutputApi.SectionSize = (size_t)((void*)&__dmod_outputs_end - (void*)&__dmod_outputs_start);
        Dmod_BuiltinOutputApi.ApiType = Dmod_ApiType_Output;
        Dmod_BuiltinOutputApi.OutputSection = (Dmod_OutputsSection_t*)&__dmod_outputs_start;
    }
    return Dmod_BuiltinInputApi.InputSection != NULL && Dmod_BuiltinOutputApi.OutputSection != NULL;
}

/**
 * @brief Deinitialize DMOD system
 * 
 * @return True if deinitialization was successful, false otherwise
 */
bool Dmod_Deinitialize(void)
{
    return true;
}

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

    if( Dmod_IsDMPFile(Path) )
    {
        uint32_t nIndex = UINT32_MAX;
        DMOD_LOG_INFO("Module is DMP package - loading from package\n");
        if( !Dmod_AddPackageFile( Path, &nIndex ) )
        {
            DMOD_LOG_ERROR("Cannot load module - failed to add DMP package\n");
            return NULL;
        }

        Dmod_PackageSlot_t* slot = Dmod_Pck_GetSlotByIndex( nIndex );
        if( !Dmod_Pck_IsValidSlot( slot ) )
        {
            DMOD_LOG_ERROR("Cannot load module - failed to get DMP package slot\n");
            return NULL;
        }

        const char* mainModuleName = Dmod_Pck_GetMainModuleName( slot );
        if( mainModuleName == NULL )
        {
            DMOD_LOG_ERROR("Cannot load module - failed to get main module name from DMP package\n");
            return NULL;    
        }

        return Dmod_LoadFromPackage( Dmod_Pck_GetPackageName( slot ), mainModuleName );
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

    if(!ReadFile( Path, buffer, fileSize, file, 0 ))
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

    if( Dmod_IsDMP(Data, Size) )
    {
        uint32_t nIndex = UINT32_MAX;
        DMOD_LOG_INFO("Module is DMP package - loading from package\n");
        if( !Dmod_AddPackageBuffer( Data, Size, &nIndex ) )
        {
            DMOD_LOG_ERROR("Cannot load module - failed to add DMP package\n");
            return NULL;
        }

        Dmod_PackageSlot_t* slot = Dmod_Pck_GetSlotByIndex( nIndex );
        if( !Dmod_Pck_IsValidSlot( slot ) )
        {
            DMOD_LOG_ERROR("Cannot load module - failed to get DMP package slot\n");
            return NULL;
        }

        const char* mainModuleName = Dmod_Pck_GetMainModuleName( slot );
        if( mainModuleName == NULL )
        {
            DMOD_LOG_ERROR("Cannot load module - failed to get main module name from DMP package\n");
            return NULL;    
        }

        return Dmod_LoadFromPackage( Dmod_Pck_GetPackageName( slot ), mainModuleName );
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
 * @brief Load module from package
 * 
 * The function loads a module from a package by its name.
 * 
 * @param PackageName Name of the package
 * @param ModuleName Name of the module
 * 
 * @return Pointer to the context
 */
Dmod_Context_t* Dmod_LoadFromPackage( const char* PackageName, const char* ModuleName )
{
    Dmod_Context_t* context = NULL;
    if( PackageName == NULL || ModuleName == NULL )
    {
        DMOD_LOG_ERROR("Cannot load module from package - invalid package or module name\n");
        return NULL;
    }

    Dmod_PackageSlot_t* slot = Dmod_Pck_FindSlotByName( PackageName, NULL );
    if( slot == NULL || !Dmod_Pck_IsValidSlot( slot ) )
    {
        DMOD_LOG_ERROR("Cannot load module from package - package not found or invalid: %s\n", PackageName);
        return NULL;
    }

    Dmod_DmpModuleEntry_t* moduleEntry = Dmod_Pck_FindModuleEntry( slot, ModuleName );
    if( moduleEntry == NULL )
    {
        DMOD_LOG_ERROR("Cannot load module from package - module not found: %s\n", ModuleName);
        return NULL;
    }

    if( slot->PackageBuffer != NULL )
    {
        const void* moduleData = (const uint8_t*)slot->PackageBuffer + moduleEntry->ModuleOffset;
        context = Dmod_Load( moduleData, moduleEntry->FileSize );
        if( context != NULL )
        {
            context->PackageName = PackageName;
        }
        return context;
    }

    if( slot->FilePath == NULL || slot->DmpHeader == NULL )
    {
        DMOD_LOG_ERROR("Cannot load module from package - invalid package slot\n");
        return NULL;
    }

    void* file = Dmod_FileOpen( slot->FilePath, "rb" );
    if( file == NULL )
    {
        DMOD_LOG_ERROR("Cannot load module from package - cannot open package file: %s\n", slot->FilePath);
        return NULL;
    }

    Dmod_Event_ModuleLoadingInProgress( slot->FilePath, 15 );
    void* buffer = Dmod_AlignedMalloc( moduleEntry->FileSize, DMOD_STACK_ALIGNMENT );
    if( buffer == NULL )
    {
        DMOD_LOG_ERROR("Cannot load module - cannot allocate memory\n");
        Dmod_FileClose( file );
        return NULL;
    }

    Dmod_Event_ModuleLoadingInProgress( slot->FilePath, 20 );

    if(!ReadFile( slot->FilePath, buffer, moduleEntry->FileSize, file, moduleEntry->ModuleOffset ))
    {
        Dmod_FileClose( file );
        return NULL;
    }

    void* dmfData = buffer;
    size_t dmfSize = moduleEntry->FileSize;
    if( Dmod_IsDMFC(buffer, moduleEntry->FileSize) )
    {
        DMOD_LOG_INFO("Module is compressed - decompressing\n");
        if( !Dmod_FromDMFC(buffer, moduleEntry->FileSize, &dmfData, &dmfSize) )
        {
            DMOD_LOG_ERROR("Cannot load module - failed to convert from DMFC\n");
            Dmod_Free( buffer );
            Dmod_FileClose( file );
            return NULL;
        }
        Dmod_Free( buffer );
    }

    Dmod_Event_ModuleLoadingInProgress( slot->FilePath, 78 );
    context = Dmod_Context_New( dmfData, dmfSize );
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
    context->PackageName = PackageName;

    Dmod_PrintAllApis( context );
    Dmod_Event_ModuleLoadingInProgress( slot->FilePath, 100 );

    DMOD_LOG_INFO("Module loaded: %s\n", Dmod_Context_GetModuleName( context ));

    return context;
}

/**
 * @brief Find module file in repositories
 * 
 * @param ModuleName Name of the module
 * @param ArchName Name of the architecture
 * @param outFilePath Output file path
 * @param MaxLength Maximum length of the output file path
 * 
 * @return True if module file was found
 */
bool Dmod_FindModuleFile(const char* ModuleName, const char* ArchName, char* outFilePath, size_t MaxLength)
{
    if( ModuleName == NULL || outFilePath == NULL || MaxLength == 0 )
    {
        DMOD_LOG_ERROR("Cannot find module file - invalid parameters\n");
        return false;
    }
    if(ArchName == NULL)
    {
        ArchName = DMOD_ARCH;
    }

    Dmod_SearchNode_t* searchNode = Dmod_Hlp_PrepareModulesSearchNodes();
    Dmod_SearchNode_t* currentNode = searchNode;
    while( currentNode != NULL )
    {
        const char* repoDir = currentNode->Path;
        DMOD_LOG_VERBOSE("Searching for module '%s' in '%s'\n", ModuleName, repoDir);

        if( 
            (
                PrepareModulePath(repoDir, ModuleName, false, outFilePath, MaxLength)
             && Dmod_FileAvailable(outFilePath)
             && CheckModuleArchitecture(outFilePath, ArchName)
                ) ||
            (
                PrepareModulePath(repoDir, ModuleName, true, outFilePath, MaxLength)
             && Dmod_FileAvailable(outFilePath)
             && CheckModuleArchitecture(outFilePath, ArchName
                )
            )
            )
        {   
            DMOD_LOG_INFO("Found module '%s' in '%s'\n", ModuleName, outFilePath);
            Dmod_Hlp_FreeSearchPathList( searchNode );
            return true;
        }
        else 
        {
            DMOD_LOG_VERBOSE("Module '%s' not found in '%s'\n", ModuleName, repoDir);
        }
        currentNode = currentNode->Prev;
    }
    Dmod_Hlp_FreeSearchPathList( searchNode );
    return false;
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

    Dmod_SearchNode_t* searchNode = Dmod_Hlp_PrepareModulesSearchNodes();
    Dmod_SearchNode_t* currentNode = searchNode;
    while( currentNode != NULL )
    {
        const char* repoDir = currentNode->Path;
        char filePath[DMOD_MAX_FILE_PATH_LENGTH];
        DMOD_LOG_VERBOSE("Searching for module '%s' in '%s'\n", ModuleName, repoDir);

        if( 
            (
                PrepareModulePath(repoDir, ModuleName, false, filePath, sizeof(filePath))
             && Dmod_FileAvailable(filePath)
             && Dmod_LoadFile( filePath ) != NULL
                ) ||
            (
                PrepareModulePath(repoDir, ModuleName, true, filePath, sizeof(filePath))
             && Dmod_FileAvailable(filePath)
             && Dmod_LoadFile( filePath ) != NULL
                )
            )
        {   
            DMOD_LOG_INFO("Loaded module '%s' from file '%s'\n", ModuleName, filePath);
            Dmod_Hlp_FreeSearchPathList( searchNode );
            return true;
        }
        else 
        {
            DMOD_LOG_VERBOSE("Module '%s' not found in '%s'\n", ModuleName, repoDir);
        }
        currentNode = currentNode->Prev;
    }
    Dmod_PackageSlot_t* slot = NULL;
    Dmod_DmpModuleEntry_t* moduleEntry = Dmod_Pck_FindModuleEntryInPackages( ModuleName, &slot );
    if( moduleEntry != NULL && slot != NULL )
    {
        DMOD_LOG_INFO("Using module '%s' from package '%s'\n", ModuleName, Dmod_Pck_GetPackageName( slot ));
        Dmod_Context_t* context = Dmod_LoadFromPackage( Dmod_Pck_GetPackageName( slot ), ModuleName );
        if( context != NULL )
        {
            return true;
        }
    }
    DMOD_LOG_ERROR("Cannot load module by name - module not found: %s\n", ModuleName);
    return false;
}

/**
 * @brief Load module by name from package
 * 
 * @param ModuleName Name of the module
 * @param PackageName Name of the package
 * 
 * @return True if module was loaded
 */
bool Dmod_LoadModuleFromPackage(const char* ModuleName, const char* PackageName)
{
    if( ModuleName == NULL || PackageName == NULL )
    {
        DMOD_LOG_ERROR("Cannot load module by name from package - invalid name\n");
        return false;
    }

    if(Dmod_Mgr_IsLoaded(ModuleName))
    {
        DMOD_LOG_INFO("Module %s is already loaded\n", ModuleName);
        return true;
    }

    Dmod_Context_t* context = Dmod_LoadFromPackage( PackageName, ModuleName );
    if( context == NULL )
    {
        DMOD_LOG_ERROR("Cannot load module by name from package - module not found: %s in package %s\n", ModuleName, PackageName);
        return false;
    }

    DMOD_LOG_INFO("Using module '%s' from package '%s'\n", ModuleName, PackageName);
    return true;
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
 * @brief Set crossplatform mode
 * 
 * @param Enable If true, crossplatform mode is enabled
 */
void Dmod_SetCrossplatformMode ( bool Enable )
{
    DMOD_LOG_INFO("Setting crossplatform mode to: %s\n", Enable ? "ENABLED" : "DISABLED");
    Dmod_SystemCrossplatformMode = Enable;
    if(Enable)
    {
        DMOD_LOG_WARN("Crossplatform mode ENABLED - module compatibility checks are DISABLED\n");
    }
}

/**
 * @brief Check if crossplatform mode is enabled
 * 
 * @return true if crossplatform mode is enabled, false otherwise
 */
bool Dmod_IsCrossplatformMode ( void )
{
    return Dmod_SystemCrossplatformMode;
}

/**
 * @brief Get next required module
 * 
 * @param Context Context to get required modules from
 * @param Last Last required module returned, or NULL to get the first one
 * 
 * @return Pointer to the next required module, or NULL if there are no more
 */
const Dmod_RequiredModule_t* Dmod_GetNextRequiredModule( Dmod_Context_t* Context, const Dmod_RequiredModule_t* Last )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot get next required module - invalid context\n");
        return NULL;
    }

    if( Last == NULL )
    {
        return &Context->RequiredModules[0];
    }

    size_t index = Last - &Context->RequiredModules[0];
    if( index + 1 >= DMOD_MAX_REQUIRED_MODULES )
    {
        return NULL;
    }

    const Dmod_RequiredModule_t* next = &Context->RequiredModules[index + 1];
    if( next->Name[0] == '\0' )
    {
        return NULL;
    }

    return &Context->RequiredModules[index + 1];
}

/**
 * @brief Read required modules from module file
 * 
 * @param Path Path to the module file
 * @param outRequiredModules Output array of required modules
 * @param MaxModules Maximum number of modules to read
 * 
 * @return true on success, false on error
 */
bool Dmod_ReadRequiredModules( const char* Path, Dmod_RequiredModule_t* outRequiredModules, size_t MaxModules )
{
    if( Path == NULL || outRequiredModules == NULL || MaxModules == 0 )
    {
        DMOD_LOG_ERROR("Cannot read required modules - invalid parameters\n");
        return false;
    }

    Dmod_Context_t* context = Dmod_LoadFile( Path );
    if( context == NULL )
    {
        DMOD_LOG_ERROR("Cannot read required modules - cannot load module file: %s\n", Path);
        return false;
    }

    size_t count = 0;
    const Dmod_RequiredModule_t* reqModule = Dmod_GetNextRequiredModule( context, NULL );
    while( reqModule != NULL && count < MaxModules )
    {
        memcpy( &outRequiredModules[count], reqModule, sizeof(Dmod_RequiredModule_t) );
        count++;
        reqModule = Dmod_GetNextRequiredModule( context, reqModule );
    }

    Dmod_Unload( context, true );

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
 * @brief Get module name
 * 
 * @param Context Context to get module name from
 * 
 * @return Module name (from signature)
 */
const char* Dmod_GetName( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot get module name - invalid context\n");
        return "Invalid";
    }

    if( Context->Header == NULL )
    {
        DMOD_LOG_ERROR("Cannot get module name - header not set\n");
        return "Unknown";
    }

    return Context->Header->Name;
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
 * @brief Get architecture name from a DMF/DMFC file
 * 
 * @param FilePath Path to the DMF or DMFC file
 * @param outArch Output buffer to store the architecture name
 * @param MaxLength Maximum length of the output buffer
 * 
 * @return true on success, false on error
 */
bool Dmod_GetFileArchitecture( const char* FilePath, char* outArch, size_t MaxLength )
{
    if( FilePath == NULL || outArch == NULL || MaxLength == 0 )
    {
        return false;
    }

    // Initialize output buffer
    memset( outArch, 0, MaxLength );

    // Load the module file (handles both DMF and DMFC)
    Dmod_Context_t* context = Dmod_LoadFile( FilePath );
    if( context == NULL )
    {
        return false;
    }

    // Read architecture from the loaded module header
    if( context->Header != NULL )
    {
        strncpy( outArch, context->Header->Arch, MaxLength - 1 );
        outArch[MaxLength - 1] = '\0';
    }

    // Unload the module
    Dmod_Unload( context, true );

    return outArch[0] != '\0';
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
 * @param FilePos Position in the file to start reading from
 * 
 * @return True if file was read successfully, false otherwise
 */
static bool ReadFile( const char* ModuleName, void* Data, size_t Size, void* File, long FilePos  )
{
    if( Data == NULL || File == NULL )
    {
        return false;
    }

    // Seek to the beginning of the file
    if( Dmod_FileSeek( File, FilePos, DMOD_SEEK_SET ) != 0 )
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

/**
 * @brief Check module architecture
 * 
 * @param FilePath Path to the module file
 * @param ExpectedArch Expected architecture
 * 
 * @return True if module architecture matches expected architecture, false otherwise
 */
static bool CheckModuleArchitecture( const char* FilePath, const char* ExpectedArch )
{
    char fileArch[DMOD_MAX_ARCH_NAME_LENGTH];
    Dmod_EnterCritical();
    bool crossplatform = Dmod_IsCrossplatformMode();
    Dmod_SetCrossplatformMode( strcmp( ExpectedArch, DMOD_ARCH ) == 0 );
    bool archRead = Dmod_GetFileArchitecture( FilePath, fileArch, sizeof(fileArch) );
    Dmod_SetCrossplatformMode( crossplatform );
    Dmod_ExitCritical();
    if( !archRead )
    {
        DMOD_LOG_ERROR("Cannot check module architecture - cannot get file architecture: %s\n", FilePath);
        return false;
    }
    if( strcmp( fileArch, ExpectedArch ) != 0 )
    {
        DMOD_LOG_ERROR("Module architecture mismatch - expected: %s, got: %s\n", ExpectedArch, fileArch);
        return false;
    }

    return true;
}

/**
 * @brief Internal structure for modules iterator
 */
typedef struct 
{
    size_t                  ContextIndex;           //!< Current index in Dmod_Contexts array
    Dmod_SearchNode_t*      SearchNodeHead;         //!< Head of search path list
    Dmod_SearchNode_t*      CurrentSearchNode;      //!< Current search node being processed
    void*                   CurrentDir;             //!< Current directory handle
    size_t                  PackageIndex;           //!< Current package index
    size_t                  PackageModuleIndex;     //!< Current module index within package
    bool                    LoadedPhaseComplete;    //!< True when loaded modules phase is complete
    bool                    SearchPathsPhaseComplete; //!< True when search paths phase is complete
    bool                    PackagesPhaseComplete;  //!< True when packages phase is complete
    Dmod_ModuleInfo_t       CurrentModule;          //!< Buffer for current module info
    char                    SeenModules[DMOD_MAX_MODULES][DMOD_MAX_MODULE_NAME_LENGTH]; //!< Track seen modules
    size_t                  SeenModulesCount;       //!< Number of seen modules
} Dmod_ModulesIteratorInternal_t;

/**
 * @brief Helper function to check if module was already seen
 * 
 * @param Iterator Iterator state
 * @param ModuleName Name of the module to check
 * 
 * @return True if module was already seen, false otherwise
 */
static bool IsModuleSeen( Dmod_ModulesIteratorInternal_t* Iterator, const char* ModuleName )
{
    for( size_t i = 0; i < Iterator->SeenModulesCount; i++ )
    {
        if( strcmp( Iterator->SeenModules[i], ModuleName ) == 0 )
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief Helper function to mark module as seen
 * 
 * @param Iterator Iterator state
 * @param ModuleName Name of the module to mark as seen
 */
static void MarkModuleSeen( Dmod_ModulesIteratorInternal_t* Iterator, const char* ModuleName )
{
    if( Iterator->SeenModulesCount < DMOD_MAX_MODULES )
    {
        strncpy( Iterator->SeenModules[Iterator->SeenModulesCount], ModuleName, DMOD_MAX_MODULE_NAME_LENGTH - 1 );
        Iterator->SeenModules[Iterator->SeenModulesCount][DMOD_MAX_MODULE_NAME_LENGTH - 1] = '\0';
        Iterator->SeenModulesCount++;
    }
}

/**
 * @brief Open modules iterator
 * 
 * Creates an iterator for listing all loaded and available modules.
 * Use Dmod_ReadModule to get the next module and Dmod_CloseModules to free resources.
 * 
 * @return Modules iterator handle, or NULL on error
 */
Dmod_ModulesIterator_t Dmod_OpenModules( void )
{
    Dmod_ModulesIteratorInternal_t* iterator = (Dmod_ModulesIteratorInternal_t*)Dmod_Malloc( sizeof(Dmod_ModulesIteratorInternal_t) );
    if( iterator == NULL )
    {
        DMOD_LOG_ERROR("Cannot open modules iterator - out of memory\n");
        return NULL;
    }

    // Initialize iterator state
    memset( iterator, 0, sizeof(Dmod_ModulesIteratorInternal_t) );
    iterator->ContextIndex = 0;
    iterator->SearchNodeHead = Dmod_Hlp_PrepareModulesSearchNodes();
    iterator->CurrentSearchNode = iterator->SearchNodeHead;
    iterator->CurrentDir = NULL;
    iterator->PackageIndex = 0;
    iterator->PackageModuleIndex = 0;
    iterator->LoadedPhaseComplete = false;
    iterator->SearchPathsPhaseComplete = false;
    iterator->PackagesPhaseComplete = false;
    iterator->SeenModulesCount = 0;

    return (Dmod_ModulesIterator_t)iterator;
}

/**
 * @brief Read next module from iterator
 * 
 * Returns the next module in the iteration. Returns NULL when no more modules are available.
 * The returned pointer is valid until the next call to Dmod_ReadModule or Dmod_CloseModules.
 * 
 * @param Iterator Modules iterator handle
 * 
 * @return Pointer to module info, or NULL if no more modules
 */
const Dmod_ModuleInfo_t* Dmod_ReadModule( Dmod_ModulesIterator_t Iterator )
{
    if( Iterator == NULL )
    {
        DMOD_LOG_ERROR("Cannot read module - invalid iterator\n");
        return NULL;
    }

    Dmod_ModulesIteratorInternal_t* iter = (Dmod_ModulesIteratorInternal_t*)Iterator;

    // Phase 1: Iterate through loaded modules
    if( !iter->LoadedPhaseComplete )
    {
        while( iter->ContextIndex < DMOD_MAX_MODULES )
        {
            if( Dmod_Contexts[iter->ContextIndex] != NULL && Dmod_Contexts[iter->ContextIndex]->Header != NULL )
            {
                const char* moduleName = Dmod_Contexts[iter->ContextIndex]->Header->Name;
                
                if( !IsModuleSeen( iter, moduleName ) )
                {
                    // Prepare module info
                    Dmod_ModuleState_t state = Dmod_ModuleState_Loaded;
                    if( Dmod_Contexts[iter->ContextIndex]->Running )
                    {
                        state = Dmod_ModuleState_Running;
                    }
                    else if( Dmod_Contexts[iter->ContextIndex]->Enabled )
                    {
                        state = Dmod_ModuleState_Enabled;
                    }

                    strncpy( iter->CurrentModule.ModuleName, moduleName, DMOD_MAX_MODULE_NAME_LENGTH - 1 );
                    iter->CurrentModule.ModuleName[DMOD_MAX_MODULE_NAME_LENGTH - 1] = '\0';
                    
                    strncpy( iter->CurrentModule.Version, Dmod_Contexts[iter->ContextIndex]->Header->Version, DMOD_MAX_VERSION_LENGTH - 1 );
                    iter->CurrentModule.Version[DMOD_MAX_VERSION_LENGTH - 1] = '\0';
                    
                    iter->CurrentModule.State = state;

                    MarkModuleSeen( iter, moduleName );
                    iter->ContextIndex++;
                    return &iter->CurrentModule;
                }
            }
            iter->ContextIndex++;
        }
        iter->LoadedPhaseComplete = true;
    }

    // Phase 2: Scan available modules in search paths
    if( !iter->SearchPathsPhaseComplete )
    {
        while( iter->CurrentSearchNode != NULL )
        {
            if( iter->CurrentDir == NULL )
            {
                iter->CurrentDir = Dmod_OpenDir( iter->CurrentSearchNode->Path );
            }

            if( iter->CurrentDir != NULL )
            {
                const char* fileName;
                while( (fileName = Dmod_ReadDir( iter->CurrentDir )) != NULL )
                {
                    // Check if it's a .dmf or .dmfc file
                    size_t len = strlen( fileName );
                    bool isDmf = (len > 4 && strcmp( &fileName[len - 4], ".dmf" ) == 0);
                    bool isDmfc = (len > 5 && strcmp( &fileName[len - 5], ".dmfc" ) == 0);
                    
                    if( isDmf || isDmfc )
                    {
                        // Extract module name (remove extension)
                        char moduleName[DMOD_MAX_MODULE_NAME_LENGTH];
                        size_t nameLen = isDmf ? len - 4 : len - 5;
                        if( nameLen >= DMOD_MAX_MODULE_NAME_LENGTH )
                        {
                            nameLen = DMOD_MAX_MODULE_NAME_LENGTH - 1;
                        }
                        strncpy( moduleName, fileName, nameLen );
                        moduleName[nameLen] = '\0';

                        if( !IsModuleSeen( iter, moduleName ) )
                        {
                            // Read module header to get version
                            char filePath[DMOD_MAX_FILE_PATH_LENGTH];
                            Dmod_SnPrintf( filePath, sizeof(filePath), "%s/%s", iter->CurrentSearchNode->Path, fileName );
                            
                            Dmod_ModuleHeader_t header;
                            if( Dmod_ReadModuleHeader( filePath, &header ) )
                            {
                                // Check architecture match
                                if( strcmp( header.Arch, DMOD_ARCH ) == 0 )
                                {
                                    strncpy( iter->CurrentModule.ModuleName, header.Name, DMOD_MAX_MODULE_NAME_LENGTH - 1 );
                                    iter->CurrentModule.ModuleName[DMOD_MAX_MODULE_NAME_LENGTH - 1] = '\0';
                                    
                                    strncpy( iter->CurrentModule.Version, header.Version, DMOD_MAX_VERSION_LENGTH - 1 );
                                    iter->CurrentModule.Version[DMOD_MAX_VERSION_LENGTH - 1] = '\0';
                                    
                                    iter->CurrentModule.State = Dmod_ModuleState_Available;

                                    MarkModuleSeen( iter, header.Name );
                                    return &iter->CurrentModule;
                                }
                            }
                        }
                    }
                }
            }

            // Close current directory and move to next search node
            if( iter->CurrentDir != NULL )
            {
                Dmod_CloseDir( iter->CurrentDir );
                iter->CurrentDir = NULL;
            }
            iter->CurrentSearchNode = iter->CurrentSearchNode->Prev;
        }
        iter->SearchPathsPhaseComplete = true;
    }

    // Phase 3: Add available modules from packages
    if( !iter->PackagesPhaseComplete )
    {
        while( iter->PackageIndex < DMOD_MAX_NUMBER_OF_PACKAGES )
        {
            if( Dmod_Pck_IsSlotUsed( &Dmod_Packages[iter->PackageIndex] ) && 
                Dmod_Pck_IsValidSlot( &Dmod_Packages[iter->PackageIndex] ) )
            {
                Dmod_PackageSlot_t* slot = &Dmod_Packages[iter->PackageIndex];
                
                while( iter->PackageModuleIndex < slot->DmpHeader->ModuleCount )
                {
                    const char* moduleName = slot->ModuleEntries[iter->PackageModuleIndex].ModuleName;
                    
                    if( !IsModuleSeen( iter, moduleName ) )
                    {
                        strncpy( iter->CurrentModule.ModuleName, moduleName, DMOD_MAX_MODULE_NAME_LENGTH - 1 );
                        iter->CurrentModule.ModuleName[DMOD_MAX_MODULE_NAME_LENGTH - 1] = '\0';
                        
                        iter->CurrentModule.Version[0] = '\0'; // No version info available for package modules
                        iter->CurrentModule.State = Dmod_ModuleState_Available;

                        MarkModuleSeen( iter, moduleName );
                        iter->PackageModuleIndex++;
                        return &iter->CurrentModule;
                    }
                    iter->PackageModuleIndex++;
                }
                iter->PackageModuleIndex = 0;
            }
            iter->PackageIndex++;
        }
        iter->PackagesPhaseComplete = true;
    }

    // No more modules
    return NULL;
}

/**
 * @brief Close modules iterator and free resources
 * 
 * @param Iterator Modules iterator handle
 */
void Dmod_CloseModules( Dmod_ModulesIterator_t Iterator )
{
    if( Iterator == NULL )
    {
        return;
    }

    Dmod_ModulesIteratorInternal_t* iter = (Dmod_ModulesIteratorInternal_t*)Iterator;

    // Close any open directory
    if( iter->CurrentDir != NULL )
    {
        Dmod_CloseDir( iter->CurrentDir );
    }

    // Free search path list
    if( iter->SearchNodeHead != NULL )
    {
        Dmod_Hlp_FreeSearchPathList( iter->SearchNodeHead );
    }

    // Free iterator
    Dmod_Free( iter );
}

