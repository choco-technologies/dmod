#define DMOD_PRIVATE
#ifndef DMOD_EXTERNAL_REGISTRATION
#   define DMOD_ENABLE_REGISTRATION
#endif
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
#include <stdio.h>

//==============================================================================
//                              LOCAL FUNCTION PROTOTYPES
//==============================================================================

static bool ReadFile( const char* ModuleName, void* Data, size_t Size, void* File, long FilePos );
static bool IsAllApiConnected( Dmod_Context_t* Context );
static bool PrepareModulePath( const char* RepoDir, const char* ModuleName, bool Compressed, char* Path, size_t MaxLength );
static bool CheckModuleArchitecture( const char* FilePath, const char* ExpectedArch );

//==============================================================================
//                              LOCAL MACROS
//==============================================================================

/**
 * @brief Get the text section address for a loaded module
 * 
 * This is the address that should be used with gdb's add-symbol-file command
 * to debug the module.
 */
#define DMOD_GET_TEXT_SECTION_ADDR(ctx) \
    ((void*)((uint8_t*)(ctx)->Data + (ctx)->Footer->Text.SectionStart))

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
    DMOD_LOG_INFO("To debug module '%s' in gdb: add-symbol-file <MODULE_ELF_FILE> %p\n", Dmod_Context_GetModuleName( context ), DMOD_GET_TEXT_SECTION_ADDR(context));

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
    bool isCompressed = Dmod_IsDMFC(Data, Size);
    if( isCompressed )
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

    // Only copy the data if it was not decompressed (i.e., not DMFC)
    // If it was DMFC, dmfData already contains the decompressed data
    if( !isCompressed )
    {
        memcpy( context->Data, Data, Size );
    }

    Dmod_Event_ModuleLoadingInProgress( Dmod_Context_GetModuleName(context), 50 );

    if(!Dmod_Ldr_Load(context) || !Dmod_Context_Add(context))
    {
        Dmod_Context_Delete( context );
        return NULL;
    }
    
    Dmod_PrintAllApis( context );
    Dmod_Event_ModuleLoadingInProgress( Dmod_Context_GetModuleName(context), 100 );

    DMOD_LOG_INFO("Module loaded: %s\n", Dmod_Context_GetModuleName( context ));
    DMOD_LOG_INFO("To debug module '%s' in gdb: add-symbol-file <MODULE_ELF_FILE> %p\n", Dmod_Context_GetModuleName( context ), DMOD_GET_TEXT_SECTION_ADDR(context));
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
    DMOD_LOG_INFO("To debug module '%s' in gdb: add-symbol-file <MODULE_ELF_FILE> %p\n", Dmod_Context_GetModuleName( context ), DMOD_GET_TEXT_SECTION_ADDR(context));

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
 * @brief Find module name that matches the partial name
 * 
 * This function searches all available paths (including packages) and looks for 
 * a module whose name starts with the provided partial name. For example, 
 * calling Dmod_FindMatch('mk', ...) will return 'mkdir' if such a module exists.
 * 
 * @param PartialName Partial module name to search for
 * @param outModuleName Output buffer for the full module name
 * @param MaxLength Maximum length of the output buffer
 * 
 * @return True if a matching module was found, false otherwise
 */
bool Dmod_FindMatch(const char* PartialName, char* outModuleName, size_t MaxLength)
{
    if( PartialName == NULL || outModuleName == NULL || MaxLength == 0 )
    {
        DMOD_LOG_ERROR("Cannot find match - invalid parameters\n");
        return false;
    }

    size_t partialLen = strlen(PartialName);
    if( partialLen == 0 )
    {
        DMOD_LOG_ERROR("Cannot find match - empty partial name\n");
        return false;
    }

    Dmod_SearchNode_t* searchNode = Dmod_Hlp_PrepareModulesSearchNodes();
    Dmod_SearchNode_t* currentNode = searchNode;
    
    // Search in filesystem paths
    while( currentNode != NULL )
    {
        const char* repoDir = currentNode->Path;
        DMOD_LOG_VERBOSE("Searching for module matching '%s' in '%s'\n", PartialName, repoDir);

        void* dir = Dmod_OpenDir(repoDir);
        if( dir != NULL )
        {
            const char* fileName;
            while( (fileName = Dmod_ReadDir(dir)) != NULL )
            {
                // Check if file has .dmf or .dmfc extension
                size_t fileNameLen = strlen(fileName);
                size_t moduleNameLen = 0;
                bool hasDmfExt = false;
                
                // Check for .dmf extension
                if( fileNameLen > 4 && strcmp(fileName + fileNameLen - 4, ".dmf") == 0 )
                {
                    moduleNameLen = fileNameLen - 4;
                    hasDmfExt = true;
                }
                // Check for .dmfc extension
                else if( fileNameLen > 5 && strcmp(fileName + fileNameLen - 5, ".dmfc") == 0 )
                {
                    moduleNameLen = fileNameLen - 5;
                    hasDmfExt = true;
                }
                
                if( hasDmfExt && moduleNameLen >= partialLen && strncmp(fileName, PartialName, partialLen) == 0 )
                {
                    // Ensure we have room for module name plus null terminator
                    if( moduleNameLen + 1 <= MaxLength )
                    {
                        memcpy(outModuleName, fileName, moduleNameLen);
                        outModuleName[moduleNameLen] = '\0';
                        DMOD_LOG_INFO("Found matching module '%s' for partial name '%s'\n", outModuleName, PartialName);
                        Dmod_CloseDir(dir);
                        Dmod_Hlp_FreeSearchPathList(searchNode);
                        return true;
                    }
                }
            }
            Dmod_CloseDir(dir);
        }
        currentNode = currentNode->Prev;
    }
    
    // Search in packages
    for( size_t i = 0; i < DMOD_MAX_NUMBER_OF_PACKAGES; i++ )
    {
        Dmod_PackageSlot_t* slot = &Dmod_Packages[i];
        if( Dmod_Pck_IsSlotUsed(slot) && slot->DmpHeader != NULL && slot->ModuleEntries != NULL )
        {
            for( uint32_t j = 0; j < slot->DmpHeader->ModuleCount; j++ )
            {
                Dmod_DmpModuleEntry_t* entry = &slot->ModuleEntries[j];
                if( entry->ModuleName != NULL )
                {
                    size_t moduleNameLen = strlen(entry->ModuleName);
                    if( moduleNameLen >= partialLen && strncmp(entry->ModuleName, PartialName, partialLen) == 0 )
                    {
                        // Ensure we have room for module name plus null terminator
                        if( moduleNameLen + 1 <= MaxLength )
                        {
                            memcpy(outModuleName, entry->ModuleName, moduleNameLen);
                            outModuleName[moduleNameLen] = '\0';
                            DMOD_LOG_INFO("Found matching module '%s' in package '%s' for partial name '%s'\n", 
                                         outModuleName, Dmod_Pck_GetPackageName(slot), PartialName);
                            Dmod_Hlp_FreeSearchPathList(searchNode);
                            return true;
                        }
                    }
                }
            }
        }
    }
    
    Dmod_Hlp_FreeSearchPathList(searchNode);
    DMOD_LOG_VERBOSE("No matching module found for partial name '%s'\n", PartialName);
    return false;
}

/**
 * @brief Load module by name
 * 
 * @param ModuleName Name of the module
 * 
 * @return True if module was loaded
 */
Dmod_Context_t* Dmod_LoadModuleByName(const char* ModuleName)
{
    if( ModuleName == NULL )
    {
        DMOD_LOG_ERROR("Cannot load module by name - invalid name\n");
        return NULL;
    }

    if(Dmod_Mgr_IsLoaded(ModuleName))
    {
        DMOD_LOG_INFO("Module %s is already loaded\n", ModuleName);
        return Dmod_Context_Get(ModuleName);
    }

    Dmod_SearchNode_t* searchNode = Dmod_Hlp_PrepareModulesSearchNodes();
    Dmod_SearchNode_t* currentNode = searchNode;
    Dmod_Context_t* context = NULL;
    while( currentNode != NULL )
    {
        const char* repoDir = currentNode->Path;
        char filePath[DMOD_MAX_FILE_PATH_LENGTH];
        DMOD_LOG_VERBOSE("Searching for module '%s' in '%s'\n", ModuleName, repoDir);
        
        if( 
            (
                PrepareModulePath(repoDir, ModuleName, false, filePath, sizeof(filePath))
             && Dmod_FileAvailable(filePath)
             && (context = Dmod_LoadFile( filePath )) != NULL
                ) ||
            (
                PrepareModulePath(repoDir, ModuleName, true, filePath, sizeof(filePath))
             && Dmod_FileAvailable(filePath)
             && (context = Dmod_LoadFile( filePath )) != NULL
                )
            )
        {   
            DMOD_LOG_INFO("Loaded module '%s' from file '%s'\n", ModuleName, filePath);
            Dmod_Hlp_FreeSearchPathList( searchNode );
            return context;
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
        context = Dmod_LoadFromPackage( Dmod_Pck_GetPackageName( slot ), ModuleName );
        return context;
    }
    DMOD_LOG_ERROR("Cannot load module by name - module not found: %s\n", ModuleName);
    Dmod_Mgr_PrintSystemModules();
    return NULL;
}

/**
 * @brief Load module by name from package
 * 
 * @param ModuleName Name of the module
 * @param PackageName Name of the package
 * 
 * @return Context pointer if module was loaded successfully, NULL otherwise
 */
Dmod_Context_t* Dmod_LoadModuleFromPackage(const char* ModuleName, const char* PackageName)
{
    if( ModuleName == NULL || PackageName == NULL )
    {
        DMOD_LOG_ERROR("Cannot load module by name from package - invalid name\n");
        return NULL;
    }

    if(Dmod_Mgr_IsLoaded(ModuleName))
    {
        DMOD_LOG_INFO("Module %s is already loaded\n", ModuleName);
        return Dmod_Context_Get(ModuleName);
    }

    Dmod_Context_t* context = Dmod_LoadFromPackage( PackageName, ModuleName );
    if( context == NULL )
    {
        DMOD_LOG_ERROR("Cannot load module by name from package - module not found: %s in package %s\n", ModuleName, PackageName);
        return NULL;
    }

    DMOD_LOG_INFO("Using module '%s' from package '%s'\n", ModuleName, PackageName);
    return context;
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
        return false;
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

    int deinitResult = Dmod_Deinit( Context );
    if( deinitResult != 0 )
    {
        DMOD_LOG_ERROR("Module deinit has failed: %d\n", deinitResult);
        result = deinitResult;
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

    return Context->Header->License.Ptr;
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
 * @brief Set log level
 * 
 * @param Level Log level to set
 */
void Dmod_SetLogLevel( Dmod_LogLevel_t Level )
{
    if( Level >= Dmod_LogLevel_Count )
    {
        DMOD_LOG_ERROR("Cannot set log level - invalid level: %d\n", Level);
        return;
    }
    Dmod_LogLevel = Level;
    DMOD_LOG_INFO("Setting log level to: %d\n", Level);
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
 * @return Pointer to the context
 */
Dmod_Context_t* Dmod_LoadModule(const char* FilePath)
{
    if( FilePath == NULL )
    {
        DMOD_LOG_ERROR("Cannot load module - invalid module name\n");
        return NULL;
    }

    Dmod_Context_t* context = Dmod_LoadFile( FilePath );
    if( context == NULL )
    {
        DMOD_LOG_ERROR("Cannot load module: %s\n", FilePath);
        return NULL;
    }

    return context;
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
int Dmod_RunModule(const char* Module, int argc, char *argv[])
{
    if( Module == NULL )
    {
        DMOD_LOG_ERROR("Cannot run module - missing module name to run\n");
        return -EINVAL;
    }

    Dmod_Context_t* context = NULL;
    if(Dmod_FileAvailable(Module))
    {
        context = Dmod_LoadFile( Module );
    }
    else 
    {
        context = Dmod_LoadModuleByName( Module );    
    }
    if( context == NULL )
    {
        DMOD_LOG_ERROR("Cannot run module - cannot load module: %s\n", Module);
        return -ENOENT;
    }

    int result = Dmod_Run( context, argc, argv );
    Dmod_Unload( context, false );
    return result;
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

/**
 * @brief Get DIF function pointer from module context
 * 
 * @param Context Module context
 * @param DifSignature DIF function signature
 * 
 * @return Function pointer or NULL if not found
 */
void* Dmod_GetDifFunction( Dmod_Context_t* Context, const char* DifSignature )
{
    if( !Dmod_Context_IsValid( Context ) || !Dmod_ApiSignature_IsValid( DifSignature ) )
    {
        DMOD_LOG_ERROR("Cannot get DIF function - invalid context or signature\n");
        return NULL;
    }

    if( !Dmod_IsEnabled(Context) && !Dmod_IsRunning(Context) )
    {
        DMOD_LOG_ERROR("Cannot get DIF function - module is neither enabled nor running: %s\n", Dmod_Context_GetModuleName( Context ));
        return NULL;
    }

    // Check if signature is a DIF signature
    if( strncmp( DifSignature, DMOD_DIF_SIGNATURE_PREFIX, sizeof( DMOD_DIF_SIGNATURE_PREFIX ) - 1 ) != 0 )
    {
        DMOD_LOG_ERROR("Cannot get DIF function - signature is not a DIF signature\n");
        return NULL;
    }

    if( Context->Inputs.InputSection == NULL )
    {
        DMOD_LOG_ERROR("Cannot get DIF function - no input section\n");
        return NULL;
    }

    size_t numberOfEntries = Dmod_Api_GetNumberOfEntries( &Context->Inputs );
    for(size_t i = 0; i < numberOfEntries; i++)
    {
        if( Dmod_ApiSignature_AreEqual( Context->Inputs.InputSection->Entries[i].Signature, DifSignature ) )
        {
            return Context->Inputs.InputSection->Entries[i].Function;
        }
    }

    DMOD_LOG_ERROR("Cannot get DIF function - function not found: %s in module %s\n", DifSignature, Dmod_Context_GetModuleName( Context ));
    return NULL;
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
 * @brief Internal state structure for module iteration
 */
typedef struct 
{
    Dmod_SearchNode_t* searchNodeHead;   //!< Head of search nodes list
    Dmod_SearchNode_t* currentNode;      //!< Current search node being processed
    void* currentDir;                    //!< Current directory handle
    size_t packageIndex;                 //!< Current package index
    uint32_t moduleIndexInPackage;       //!< Current module index within package
    bool iteratingPackages;              //!< Flag indicating if we're iterating packages
} Dmod_ModuleIterationState_t;

/**
 * @brief Open module iteration
 * 
 * Initializes the module iteration state. Must be called before using Dmod_ReadNextModule.
 * After iteration is complete or to stop early, call Dmod_CloseModules to free resources.
 * 
 * @param outModule Pointer to module node structure (user-allocated)
 * @return true if initialization was successful, false otherwise
 */
bool Dmod_OpenModules( Dmod_ModuleNode_t* outModule )
{
    if( outModule == NULL )
    {
        DMOD_LOG_ERROR("Cannot open modules - invalid output parameter\n");
        return false;
    }

    // Check if already opened
    if( outModule->_Data != NULL )
    {
        DMOD_LOG_ERROR("Cannot open modules - already opened (call Dmod_CloseModules first)\n");
        return false;
    }

    Dmod_ModuleIterationState_t* state = (Dmod_ModuleIterationState_t*)Dmod_Malloc( sizeof(Dmod_ModuleIterationState_t) );
    if( state == NULL )
    {
        DMOD_LOG_ERROR("Cannot open modules - memory allocation failed\n");
        return false;
    }
    
    state->searchNodeHead = Dmod_Hlp_PrepareModulesSearchNodes();
    if( state->searchNodeHead == NULL )
    {
        DMOD_LOG_ERROR("Cannot open modules - failed to prepare search nodes\n");
        Dmod_Free( state );
        return false;
    }
    
    state->currentNode = state->searchNodeHead;
    state->currentDir = NULL;
    state->packageIndex = 0;
    state->moduleIndexInPackage = 0;
    state->iteratingPackages = false;
    outModule->_Data = state;

    return true;
}

/**
 * @brief Read next module from available paths and packages
 * 
 * This function iterates through all available modules in the system, including
 * those in filesystem paths and packages. Must call Dmod_OpenModules first.
 * 
 * @param outModule Pointer to module node structure (user-allocated)
 * @return true if a module was found, false if iteration is complete or error occurred
 */
bool Dmod_ReadNextModule( Dmod_ModuleNode_t* outModule )
{
    if( outModule == NULL )
    {
        DMOD_LOG_ERROR("Cannot read next module - invalid output parameter\n");
        return false;
    }

    Dmod_ModuleIterationState_t* state = (Dmod_ModuleIterationState_t*)outModule->_Data;

    // Check if modules were opened
    if( state == NULL )
    {
        DMOD_LOG_ERROR("Cannot read next module - call Dmod_OpenModules first\n");
        return false;
    }

    // Iterate through filesystem paths
    while( !state->iteratingPackages )
    {
        // Open new directory if needed
        if( state->currentDir == NULL && state->currentNode != NULL )
        {
            state->currentDir = Dmod_OpenDir( state->currentNode->Path );
            if( state->currentDir == NULL )
            {
                // Failed to open directory, move to next node
                state->currentNode = state->currentNode->Prev;
                continue;
            }
        }

        // No more directories to scan, switch to packages
        if( state->currentNode == NULL )
        {
            state->iteratingPackages = true;
            break;
        }

        // Read files from current directory
        const char* fileName = Dmod_ReadDir( state->currentDir );
        if( fileName != NULL )
        {
            // Check if file has .dmf or .dmfc extension
            size_t fileNameLen = strlen(fileName);
            bool hasDmfExt = false;

            if( fileNameLen > 4 && strcmp(fileName + fileNameLen - 4, ".dmf") == 0 )
            {
                hasDmfExt = true;
            }
            else if( fileNameLen > 5 && strcmp(fileName + fileNameLen - 5, ".dmfc") == 0 )
            {
                hasDmfExt = true;
            }

            if( hasDmfExt )
            {
                // Build full path
                const char* dirPath = state->currentNode->Path;
                size_t dirPathLen = strlen(dirPath);
                size_t totalLen = dirPathLen + 1 + fileNameLen + 1; // dir + '/' + file + '\0'

                if( totalLen <= DMOD_MAX_PATH_LENGTH )
                {
                    Dmod_SnPrintf( outModule->path, DMOD_MAX_PATH_LENGTH, "%s/%s", dirPath, fileName );
                    
                    // Try to read module header
                    if( Dmod_ReadModuleHeader( outModule->path, &outModule->header ) )
                    {
                        DMOD_LOG_VERBOSE("Found module '%s' at '%s'\n", outModule->header.Name, outModule->path);
                        return true;
                    }
                }
            }
            continue; // Continue reading files from current directory
        }
        else
        {
            // Finished reading current directory, close it and move to next node
            Dmod_CloseDir( state->currentDir );
            state->currentDir = NULL;
            state->currentNode = state->currentNode->Prev;
        }
    }

    // Iterate through packages
    while( state->iteratingPackages && state->packageIndex < DMOD_MAX_NUMBER_OF_PACKAGES )
    {
        Dmod_PackageSlot_t* slot = &Dmod_Packages[state->packageIndex];
        
        if( Dmod_Pck_IsSlotUsed(slot) && slot->DmpHeader != NULL && slot->ModuleEntries != NULL && slot->PackageBuffer != NULL )
        {
            if( state->moduleIndexInPackage < slot->DmpHeader->ModuleCount )
            {
                Dmod_DmpModuleEntry_t* entry = &slot->ModuleEntries[state->moduleIndexInPackage];
                state->moduleIndexInPackage++;

                if( entry->ModuleName[0] != '\0' )
                {
                    // Calculate module data pointer from package buffer and offset
                    const void* moduleData = (const uint8_t*)slot->PackageBuffer + entry->ModuleOffset;
                    Dmod_ModuleHeader_t* moduleHeader = (Dmod_ModuleHeader_t*)moduleData;
                    memcpy( &outModule->header, moduleHeader, sizeof(Dmod_ModuleHeader_t) );
                    
                    // Build package path notation
                    const char* packageName = Dmod_Pck_GetPackageName(slot);
                    Dmod_SnPrintf( outModule->path, DMOD_MAX_PATH_LENGTH, "[%s]/%s", 
                             packageName ? packageName : "unknown", entry->ModuleName );
                    
                    DMOD_LOG_VERBOSE("Found module '%s' in package at '%s'\n", 
                                   outModule->header.Name, outModule->path);
                    return true;
                }
                continue; // Try next module in same package
            }
        }
        
        // Move to next package
        state->packageIndex++;
        state->moduleIndexInPackage = 0;
    }

    // No more modules found
    return false;
}

/**
 * @brief Close module iteration and free resources
 * 
 * Cleans up resources allocated by Dmod_OpenModules. Should be called after
 * iteration is complete or when stopping iteration early.
 * 
 * @param outModule Pointer to module node structure
 */
void Dmod_CloseModules( Dmod_ModuleNode_t* outModule )
{
    if( outModule == NULL )
    {
        return;
    }

    Dmod_ModuleIterationState_t* state = (Dmod_ModuleIterationState_t*)outModule->_Data;
    
    if( state == NULL )
    {
        return; // Already closed or never opened
    }

    // Close any open directory
    if( state->currentDir != NULL )
    {
        Dmod_CloseDir( state->currentDir );
        state->currentDir = NULL;
    }
    
    // Free search node list
    if( state->searchNodeHead != NULL )
    {
        Dmod_Hlp_FreeSearchPathList( state->searchNodeHead );
        state->searchNodeHead = NULL;
    }
    
    // Free state structure
    Dmod_Free( state );
    outModule->_Data = NULL;
}


