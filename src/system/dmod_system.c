#define DMOD_PRIVATE
#include "dmod.h"
#include "private/dmod_vars.h"
#include "private/dmod_ctx.h"
#include "private/dmod_hlp.h"
#include "private/dmod_ldr.h"
#include "private/dmod_mgr.h"
#include "dmod_system.h"
#include <stdbool.h>
#include <string.h>
#include <errno.h>

//==============================================================================
//                              LOCAL FUNCTION PROTOTYPES
//==============================================================================

static bool                     ReadFile( const char* ModuleName, void* Data, size_t Size, void* File );
static bool                     Load( Dmod_Context_t* Context );
static Dmod_RequiredModule_t*   FindRequiredModule( Dmod_Context_t* Context, const char* ModuleName );
static Dmod_RequiredModule_t*   FindEmptyRequiredModule( Dmod_Context_t* Context );
static bool                     IsModuleRequired( Dmod_Context_t* Context, const char* ModuleName );
static bool                     ReadRequiredModules( Dmod_Context_t* Context );
static bool                     AddRequiredModule( Dmod_Context_t* Context, const char* ApiSignature );
static bool                     AreRequiredModulesEnabled( Dmod_Context_t* Context );
static Dmod_Context_t*          FindDependentModule( Dmod_Context_t* Context, bool OnlyEnabled );
static bool                     IsAllApiConnected( Dmod_Context_t* Context );
static bool                     LoadRequiredModules( Dmod_Context_t* Context );
static bool                     EnableRequiredModules( Dmod_Context_t* Context );

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

    Dmod_Event_ModuleLoadingInProgress( Path, 78 );
    Dmod_Context_t* context = Dmod_Context_New( buffer, fileSize );
    Dmod_FileClose( file );
    if( context == NULL )
    {
        return NULL;
    }
    if( !Load( context ) || !Dmod_Context_Add( context ) )
    {
        Dmod_Context_Delete( context );
        return NULL;
    }

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

    Dmod_Context_t* context = Dmod_Context_New( NULL, Size );
    if( context == NULL )
    {
        return NULL;
    }

    memcpy( context->Data, Data, Size );

    Dmod_Event_ModuleLoadingInProgress( Dmod_Context_GetModuleName(context), 50 );

    if(!Load(context) || !Dmod_Context_Add(context))
    {
        Dmod_Context_Delete( context );
        return NULL;
    }
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
    const char* repoPath = Dmod_GetRepoPath();
    if( repoPath == NULL )
    {
        DMOD_LOG_ERROR("Cannot load module by name - repository path is not set\n");
        return false;
    }

    strncpy(path, repoPath, DMOD_MAX_PATH_LENGTH);
    strncat(path, "/", DMOD_MAX_PATH_LENGTH - strlen(path));
    strncat(path, ModuleName, DMOD_MAX_PATH_LENGTH - strlen(path));
    strncat(path, ".dmf", DMOD_MAX_PATH_LENGTH - strlen(path));

    return Dmod_LoadFile(path);
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
 * @brief Connect API
 * 
 * @param Outputs Outputs section
 * @param Inputs Inputs section
 */
bool Dmod_ConnectApi( Dmod_Api_t* OutputsApi, Dmod_Api_t* InputsApi )
{
    if( OutputsApi == NULL || InputsApi == NULL )
    {
        DMOD_LOG_ERROR("Cannot connect API - invalid API pointers\n");
        return false;
    }
    
    size_t numberOfOutputs = Dmod_Api_GetNumberOfEntries( OutputsApi );
    size_t numberOfInputs = Dmod_Api_GetNumberOfEntries( InputsApi );

    for(size_t i = 0; i < numberOfOutputs; i++)
    {
        for(size_t j = 0; j < numberOfInputs; j++)
        {
            if( !Dmod_ApiSignature_IsValid( OutputsApi->OutputSection->Entries[i] ) )
            {
                continue;
            }
            else if( Dmod_ApiSignature_AreEqual( OutputsApi->OutputSection->Entries[i], InputsApi->InputSection->Entries[j].Signature ) )
            {
                DMOD_LOG_VERBOSE("Connected: %s 0x%08X\n", InputsApi->InputSection->Entries[j].Signature, InputsApi->InputSection->Entries[j].Function);
                OutputsApi->OutputSection->Entries[i] = InputsApi->InputSection->Entries[j].Function;
            }
        }
    }
    return true;
}

/**
 * @brief Disconnect API
 * 
 * @param Outputs Outputs section
 * @param Inputs Inputs section
 */
bool Dmod_DisconnectApi( Dmod_Api_t* OutputsApi, Dmod_Api_t* InputsApi )
{
    if( OutputsApi == NULL || InputsApi == NULL )
    {
        DMOD_LOG_ERROR("Cannot disconnect API - invalid API pointers\n");
        return false;
    }
    
    size_t numberOfOutputs = Dmod_Api_GetNumberOfEntries( OutputsApi );
    size_t numberOfInputs = Dmod_Api_GetNumberOfEntries( InputsApi );

    for(size_t i = 0; i < numberOfOutputs; i++)
    {
        for(size_t j = 0; j < numberOfInputs; j++)
        {
            if( !Dmod_ApiSignature_IsValid( OutputsApi->OutputSection->Entries[i] ) )
            {
                continue;
            }
            else if( OutputsApi->OutputSection->Entries[i] == InputsApi->InputSection->Entries[j].Function )
            {
                DMOD_LOG_VERBOSE("Disconnected: %s\n", InputsApi->InputSection->Entries[j].Signature);
                OutputsApi->OutputSection->Entries[i] = (void*)InputsApi->InputSection->Entries[j].Signature;
            }
        }
    }
    return true;
}

/**
 * @brief Connect output APIs
 * 
 * @param Context Context to connect APIs
 */
bool Dmod_ConnectOutputApis( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot connect APIs - invalid context\n");
        return false;
    }
    Dmod_EnterCritical();

    Dmod_BuiltinInputApi.SectionSize = (size_t)((void*)&__dmod_inputs_end - (void*)&__dmod_inputs_start);
    if( !Dmod_ConnectApi( &Context->Outputs, &Dmod_BuiltinInputApi ) )
    {
        DMOD_LOG_ERROR("Cannot connect system API to '%s' APIs\n", Dmod_Context_GetModuleName( Context ));
        Dmod_ExitCritical();
        return false;
    }

    bool result = true;
    for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
    {
        if( Dmod_Contexts[i] == NULL || Context == Dmod_Contexts[i] )
        {
            continue;
        }
        DMOD_LOG_INFO("Connecting API from %s to %s\n", Dmod_Context_GetModuleName( Dmod_Contexts[i] ), Dmod_Context_GetModuleName( Context ));

        if( !Dmod_ConnectApi( &Context->Outputs, &Dmod_Contexts[i]->Inputs ) )
        {
            DMOD_LOG_ERROR("Cannot connect API from '%s' to '%s'\n", Dmod_Context_GetModuleName( Dmod_Contexts[i] ), Dmod_Context_GetModuleName( Context ));
            result = false;
        }
    }

    if(!result)
    {
        Dmod_DisconnectOutputApis( Context );
    }
    
    Dmod_ExitCritical();
    return result;
}

/**
 * @brief Connect input APIs
 * 
 * @param Context Context to connect APIs
 */
bool Dmod_ConnectInputApis( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot connect APIs - invalid context\n");
        return false;
    }

    Dmod_EnterCritical();

    Dmod_BuiltinOutputApi.SectionSize = (size_t)((void*)&__dmod_outputs_end - (void*)&__dmod_outputs_start);
    if( !Dmod_ConnectApi( &Dmod_BuiltinOutputApi, &Context->Inputs ) )
    {
        DMOD_LOG_ERROR("Cannot connect API '%s' to system\n", Dmod_Context_GetModuleName( Context ));
        Dmod_ExitCritical();
        return false;
    }

    bool result = true;

    for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
    {
        if( Dmod_Contexts[i] == NULL )
        {
            continue;
        }

        DMOD_LOG_INFO("Connecting API from %s to %s\n", Dmod_Context_GetModuleName( Context ), Dmod_Context_GetModuleName( Dmod_Contexts[i] ));

        if( !Dmod_ConnectApi( &Dmod_Contexts[i]->Outputs, &Context->Inputs ) )
        {
            DMOD_LOG_ERROR("Cannot connect API from '%s' to '%s'\n", Dmod_Context_GetModuleName( Context ), Dmod_Context_GetModuleName( Dmod_Contexts[i] ));
            result = false;
        }
    }

    if(!result)
    {
        Dmod_DisconnectInputApis( Context );
    }

    Dmod_ExitCritical();

    return result;
}

/**
 * @brief Connect all APIs
 * 
 * @param Context Context to connect APIs
 */
bool Dmod_ConnectAllApis( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot connect APIs - invalid context\n");
        return false;
    }

    Dmod_EnterCritical();
    bool result = Dmod_ConnectOutputApis( Context ) && Dmod_ConnectInputApis( Context );
    Dmod_ExitCritical();

    return result;
}

/**
 * @brief Disconnect output APIs
 * 
 * @param Context Context to disconnect APIs
 */
bool Dmod_DisconnectOutputApis( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot disconnect APIs - invalid context\n");
        return false;
    }

    Dmod_EnterCritical();
    bool result = true;

    for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
    {
        if( Dmod_Contexts[i] == NULL )
        {
            continue;
        }

        if( !Dmod_DisconnectApi( &Context->Outputs, &Dmod_Contexts[i]->Inputs ) )
        {
            DMOD_LOG_ERROR("Cannot disconnect %s's API from '%s'\n", Dmod_Context_GetModuleName( Dmod_Contexts[i] ), Dmod_Context_GetModuleName( Context ));
            result = false;
        }
    }

    if( !Dmod_DisconnectApi( &Context->Outputs, &Dmod_BuiltinInputApi ) )
    {
        DMOD_LOG_ERROR("Cannot disconnect system API from '%s'\n", Dmod_Context_GetModuleName( Context ));
        result = false;
    }
    Dmod_ExitCritical();

    return result;
}

/**
 * @brief Disconnect input APIs
 * 
 * @param Context Context to disconnect APIs
 */
bool Dmod_DisconnectInputApis( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot disconnect APIs - invalid context\n");
        return false;
    }

    Dmod_EnterCritical();
    bool result = true;

    for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
    {
        if( Dmod_Contexts[i] == NULL )
        {
            continue;
        }

        if( !Dmod_DisconnectApi( &Dmod_Contexts[i]->Outputs, &Context->Inputs ) )
        {
            DMOD_LOG_ERROR("Cannot disconnect %s's API from '%s'\n", Dmod_Context_GetModuleName( Context ), Dmod_Context_GetModuleName( Dmod_Contexts[i] ));
            result = false;
        }
    }

    if( !Dmod_DisconnectApi( &Dmod_BuiltinOutputApi, &Context->Inputs ) )
    {
        DMOD_LOG_ERROR("Cannot disconnect %s's API from system\n", Dmod_Context_GetModuleName( Context ));
        result = false;
    }

    Dmod_ExitCritical();

    return result;
}

/**
 * @brief Disconnect all APIs
 * 
 * @param Context Context to disconnect APIs
 */
bool Dmod_DisconnectAllApis( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot disconnect APIs - invalid context\n");
        return false;
    }

    Dmod_EnterCritical();
    bool result = Dmod_DisconnectInputApis( Context ) && Dmod_DisconnectOutputApis( Context );
    Dmod_ExitCritical();
    if( !result )
    {
        DMOD_LOG_ERROR("Cannot disconnect APIs - failed to disconnect\n");
        return false;
    }

    return true;
}

/**
 * @brief Get function
 * 
 * @param Context Context to get function from
 * @param Signature Signature of the function
 * 
 * @return Pointer to the function
 */
void* Dmod_GetFunction( Dmod_Context_t* Context, const char* Signature )
{
    if( !Dmod_Context_IsValid( Context ) || !Dmod_ApiSignature_IsValid( Signature ) )
    {
        DMOD_LOG_ERROR("Cannot get function - invalid context or signature\n");
        return NULL;
    }

    if( Context->Inputs.InputSection == NULL )
    {
        DMOD_LOG_ERROR("Cannot get function - no output section\n");
        return NULL;
    }

    size_t numberOfEntries = Dmod_Api_GetNumberOfEntries( &Context->Inputs );
    for(size_t i = 0; i < numberOfEntries; i++)
    {
        if( Dmod_ApiSignature_AreEqual( Context->Inputs.InputSection->Entries[i].Signature, Signature ) )
        {
            return Context->Inputs.InputSection->Entries[i].Function;
        }
    }

    DMOD_LOG_ERROR("Cannot get function - function not found: %s\n", Signature);
    return NULL;
}

/**
 * @brief Preinitialize module
 * 
 * @param Context Context to preinitialize
 */
void Dmod_Preinit( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot preinit module - invalid context\n");
        return;
    }

    if( Context->Header->Preinit == NULL )
    {
        DMOD_LOG_INFO("Preinit function not set\n");
        return;
    }

    Context->Header->Preinit();
}

/**
 * @brief Initialize module
 * 
 * @param Context Context to initialize
 * @param Config Configuration
 * 
 * @return 0 on success, errno on error
 */
int Dmod_Init( Dmod_Context_t* Context, const Dmod_Config_t* Config )
{
    int result = -EINVAL;
    if( Dmod_Context_IsValid( Context ) )
    {
        if( Context->Header->Init == NULL )
        {
            DMOD_LOG_INFO("Init function not set\n");
            result = 0;
        }
        else 
        {
            result = Context->Header->Init( Config );
        }
    }
    return result;
}

/**
 * @brief Call main function
 * 
 * @param Context Context to call main function
 * @param argc Number of arguments
 * @param argv Arguments
 * 
 * @return Return value of the main function
 */
int Dmod_Main( Dmod_Context_t* Context, int argc, char *argv[] )
{
    int result = -EINVAL;
    if( Dmod_Context_IsValid( Context ) )
    {
        if( Context->Header->Main == NULL )
        {
            DMOD_LOG_INFO("Main function not set\n");
            result = 0;
        }
        else 
        {
            result = Context->Header->Main( argc, argv );
        }
    }
    return result;
}

/**
 * @brief Deinitialize module
 * 
 * @param Context Context to deinitialize
 * 
 * @return 0 on success, errno on error
 */
int Dmod_Deinit( Dmod_Context_t* Context )
{
    int result = -EINVAL;
    if( Dmod_Context_IsValid( Context ) )
    {
        if( Context->Header->Deinit == NULL )
        {
            DMOD_LOG_INFO("Deinit function not set\n");
            result = 0;
        }
        else 
        {
            result = Context->Header->Deinit();
        }
    }
    return result;
}

/**
 * @brief Call signal's handler
 * 
 * @param Context Context to signal
 * @param SignalNumber Signal number
 * 
 * @return 0 on success, errno on error
 */
int Dmod_Signal( Dmod_Context_t* Context, int SignalNumber )
{
    int result = -EINVAL;
    if( Dmod_Context_IsValid( Context ) )
    {
        if( Context->Header->Signal == NULL )
        {
            DMOD_LOG_INFO("Signal function not set\n");
            result = 0;
        }
        else 
        {
            result = Context->Header->Signal( SignalNumber );
        }
    }
    return result;
}

/**
 * @brief Call IRQ handler
 * 
 * @param Context Context to IRQ
 * @param IrqNumber IRQ number
 * 
 * @return 0 on success, errno on error
 */
int Dmod_Irq( Dmod_Context_t* Context, const char* Signature )
{
    int result = -EINVAL;
    if( Dmod_Context_IsValid( Context ) )
    {
        void (*function)() = Dmod_GetFunction( Context, Signature );
        if( function != NULL )
        {
            DMOD_LOG_VERBOSE("Calling IRQ %s for %s\n", Signature, Dmod_Context_GetModuleName( Context ));
            function();
        }
        result = 0;
    }
    return result;
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

    if(!LoadRequiredModules(Context))
    {
        DMOD_LOG_ERROR("Cannot run module - cannot load required modules\n");
        Dmod_Mutex_Unlock(Context->Mutex);
        return -ENOEXEC;
    }

    if(!EnableRequiredModules(Context))
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

    if( !AreRequiredModulesEnabled( Context ) )
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
    Dmod_Context_t* dependentModule = FindDependentModule( Context, true );
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

    if(!LoadRequiredModules(Context))
    {
        DMOD_LOG_ERROR("Cannot run module - cannot load required modules\n");
        Dmod_Mutex_Unlock(Context->Mutex);
        return -ENOEXEC;
    }

    if(!EnableRequiredModules(Context))
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

    int result = Dmod_Main( Context, argc, argv );
    Dmod_Event_ModuleStopped( Context );
    Context->Running = false;

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

    return IsModuleRequired( context, RequiredModuleName );
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

//==============================================================================
//                              LOCAL FUNCTIONS IMPLEMENTATIONS
//==============================================================================

/**
 * @brief Load module
 * 
 * @param Context Context to load
 * 
 * @return True if module was loaded successfully, false otherwise
 */
static bool Load( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    return Dmod_Ldr_LoadHeader( Context ) 
        && Dmod_Ldr_LoadFooter( Context )
        && Dmod_Ldr_LoadOutput( Context )
        && Dmod_Ldr_LoadInput( Context )
        && Dmod_Ldr_LoadGot( Context )
        && Dmod_Ldr_LoadBss( Context )
        && ReadRequiredModules( Context );
}

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
 * @brief Find context
 * 
 * @param ModuleName Name of the module to find
 * 
 * @return Pointer to the context
 */
static Dmod_RequiredModule_t*   FindRequiredModule( Dmod_Context_t* Context, const char* ModuleName )
{
    if( Context == NULL || ModuleName == NULL )
    {
        return NULL;
    }

    for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
    {
        if( Context->RequiredModules[i].Name[0] == 0 )
        {
            continue;
        }

        if( strcmp( Context->RequiredModules[i].Name, ModuleName ) == 0 )
        {
            return &Context->RequiredModules[i];
        }
    }

    return NULL;
}

/**
 * @brief Find empty required module
 * 
 * @param Context Context to find empty required module in
 * 
 * @return Pointer to the empty required module
 */
static Dmod_RequiredModule_t*   FindEmptyRequiredModule( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return NULL;
    }

    for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
    {
        if( Context->RequiredModules[i].Name[0] == 0 )
        {
            return &Context->RequiredModules[i];
        }
    }

    return NULL;
}

/**
 * @brief checks if the given module is required by the current module
 * 
 * @param Context Context to check
 * @param ModuleName Name of the module to check
 * 
 * @return True if module is required, false otherwise
 */
static bool IsModuleRequired( Dmod_Context_t* Context, const char* ModuleName )
{
    return FindRequiredModule( Context, ModuleName ) != NULL;
}

/**
 * @brief Read required modules
 * 
 * @param Context Context to read required modules to
 * 
 * @return True if required modules were read successfully, false otherwise
 */
static bool ReadRequiredModules( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    // Clear required modules
    memset( Context->RequiredModules, 0, sizeof(Context->RequiredModules) );

    size_t numberOfOuptuts = Dmod_Api_GetNumberOfEntries( &Context->Outputs );
    for(size_t outputIndex = 0; outputIndex < numberOfOuptuts; outputIndex++)
    {
        const char* apiSignature = Context->Outputs.OutputSection->Entries[outputIndex];
        if(apiSignature == NULL)
        {
            continue;
        }
        if(!Dmod_ApiSignature_IsModuleNameGiven(apiSignature) || Dmod_ApiSignature_IsMal(apiSignature))
        {
            continue;
        }
        if(!AddRequiredModule( Context, apiSignature ))
        {
            DMOD_LOG_ERROR("Cannot read required modules for %s - cannot add required module\n", Dmod_Context_GetModuleName( Context ));
            return false;
        }
    }

    return true;
}

/**
 * @brief Add required module
 * 
 * @param Context Context to add required module to
 * @param ApiSignature Signature of the API
 * 
 * @return True if required module was added successfully, false otherwise
 */
static bool AddRequiredModule( Dmod_Context_t* Context, const char* ApiSignature )
{
    if( Context == NULL || ApiSignature == NULL )
    {
        return false;
    }

    char moduleName[DMOD_MAX_MODULE_NAME_LENGTH] = {0};
    if( !Dmod_ApiSignature_ReadModuleName( ApiSignature, moduleName, sizeof(moduleName) ) )
    {
        DMOD_LOG_ERROR("Cannot add required module - cannot read module name\n");
        return false;
    }

    if(strncmp(Dmod_Context_GetModuleName(Context), moduleName, sizeof(moduleName)) == 0)
    {
        DMOD_LOG_ERROR("Cannot add required module - module cannot require itself\n");
        return false;
    }

    if(IsModuleRequired( Context, moduleName ))
    {
        return true;
    }

    Dmod_RequiredModule_t* requiredModule = FindEmptyRequiredModule( Context );
    if( requiredModule == NULL )
    {
        DMOD_LOG_ERROR("Cannot add required module - no space left\n");
        return false;
    }

    strncpy( requiredModule->Name, moduleName, sizeof(requiredModule->Name) );
    if(Dmod_ApiSignature_ReadVersion( ApiSignature, requiredModule->Version, sizeof(requiredModule->Version) ) == false)
    {
        DMOD_LOG_ERROR("Cannot add required module - cannot read version\n");
        return false;
    }

    DMOD_LOG_VERBOSE("Required module '%s' added to '%s'\n", requiredModule->Name, Dmod_Context_GetModuleName( Context ));

    return true;
}

/**
 * @brief Are required modules enabled
 * 
 * @param Context Context to check
 * 
 * @return True if required modules are enabled, false otherwise
 */
static bool AreRequiredModulesEnabled( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        DMOD_LOG_ERROR("Cannot check required modules - invalid context\n");
        return false;
    }

    for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
    {
        if( Context->RequiredModules[i].Name[0] == 0 )
        {
            continue;
        }

        if( !Dmod_Mgr_IsEnabled( Context->RequiredModules[i].Name ) )
        {
            DMOD_LOG_VERBOSE("Required module '%s' is not enabled\n", Context->RequiredModules[i].Name);
            return false;
        }
    }

    return true;
}

/**
 * @brief Checks if the given module is required by another module
 * 
 * @param Context Context to check
 * @param OnlyEnabled If true, only enabled modules will be checked
 * 
 * @return Pointer to the module that requires the given module
 */
static Dmod_Context_t* FindDependentModule( Dmod_Context_t* Context, bool OnlyEnabled )
{
    if( Context == NULL )
    {
        return NULL;
    }

    for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
    {
        if( Dmod_Contexts[i] == NULL )
        {
            continue;
        }

        if( Dmod_Contexts[i] == Context )
        {
            continue;
        }

        if( OnlyEnabled && !Dmod_IsEnabled( Dmod_Contexts[i] ) )
        {
            continue;
        }

        if( IsModuleRequired( Dmod_Contexts[i], Dmod_Context_GetModuleName( Context ) ) )
        {
            return Dmod_Contexts[i];
        }
    }

    return NULL;
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
 * @brief Load required modules
 * 
 * @param Context Context to load required modules to
 * 
 * @return True if required modules were loaded successfully, false otherwise
 */
static bool LoadRequiredModules( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    for(size_t i = 0; i < DMOD_MAX_REQUIRED_MODULES; i++)
    {
        if( Context->RequiredModules[i].Name[0] == 0 )
        {
            continue;
        }

        if(Dmod_Mgr_IsSystemModule(Context->RequiredModules[i].Name))
        {
            continue;
        }

        if( !Dmod_LoadModuleByName( Context->RequiredModules[i].Name ) )
        {
            DMOD_LOG_ERROR("Cannot load required module '%s'\n", Context->RequiredModules[i].Name);
            return false;
        }
    }

    DMOD_LOG_VERBOSE("All required modules loaded for '%s'\n", Dmod_Context_GetModuleName( Context ));
    return true;
}

/**
 * @brief Enable required modules
 * 
 * @param Context Context to enable required modules to
 * 
 * @return True if required modules were enabled successfully, false otherwise
 */
static bool EnableRequiredModules( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    for(size_t i = 0; i < DMOD_MAX_REQUIRED_MODULES; i++)
    {
        if( Context->RequiredModules[i].Name[0] == 0 )
        {
            continue;
        }

        if(Dmod_Mgr_IsSystemModule(Context->RequiredModules[i].Name))
        {
            continue;
        }

        if( !Dmod_EnableModule( Context->RequiredModules[i].Name, false, NULL ) )
        {
            DMOD_LOG_ERROR("Cannot enable required module '%s'\n", Context->RequiredModules[i].Name);
            return false;
        }
    }

    return true;
}