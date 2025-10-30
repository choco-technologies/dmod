#define DMOD_PRIVATE
#include <string.h>
#include "dmod.h"
#include "private/dmod_vars.h"
#include "private/dmod_pck.h"

/**
 * @brief Check if a package slot is used
 * 
 * @param Slot Pointer to the package slot to check
 * @return true If the slot is used
 * @return false If the slot is free
 */
bool Dmod_Pck_IsSlotUsed( Dmod_PackageSlot_t* Slot )
{
    return Slot->PackageBuffer != NULL || Slot->FilePath != NULL;
}

/**
 * @brief Release a package slot
 * 
 * @param Slot Pointer to the package slot to release
 */
void Dmod_Pck_ReleaseSlot( Dmod_PackageSlot_t* Slot )
{
    if( Slot == NULL )
    {
        DMOD_LOG_ERROR("Cannot release NULL package slot");
        return;
    }
    if(Slot->FilePath != NULL)
    {
        Dmod_Free( Slot->ModuleEntries );
        Dmod_Free( Slot->DmpHeader);
    }
    Slot->PackageBuffer = NULL;
    Slot->PackageSize = 0;
    Slot->FilePath = NULL;
    Slot->DmpHeader = NULL;
    Slot->ModuleEntries = NULL;
}

/**
 * @brief Get the name of the package from the slot
 * 
 * @param Slot Pointer to the package slot
 * @return const char* Name of the package, or NULL if slot is invalid
 */
const char* Dmod_Pck_GetPackageName( Dmod_PackageSlot_t* Slot )
{
    if( Slot == NULL || Slot->DmpHeader == NULL )
    {
        return NULL;
    }
    return Slot->DmpHeader->Name;
}

/**
 * @brief Check if a package slot is valid
 * 
 * @param Slot Pointer to the package slot to check
 * @return true If the slot is valid
 * @return false If the slot is invalid
 */
bool Dmod_Pck_IsValidSlot( Dmod_PackageSlot_t* Slot )
{
    if( Slot == NULL )
    {
        DMOD_LOG_ERROR("Package slot is NULL");
        return false;
    }

    if( Slot->DmpHeader == NULL )
    {
        DMOD_LOG_ERROR("Package slot DMP header is NULL");
        return false;
    }

    if( Slot->DmpHeader->Signature != DMOD_DMP_SIGNATURE ) 
    {
        DMOD_LOG_ERROR("Package slot has invalid DMP header signature: 0x%08X", Slot->DmpHeader->Signature);
        return false;
    }

    if( Slot->DmpHeader->Name[0] == '\0' )
    {
        DMOD_LOG_ERROR("Package slot has empty package name");
        return false;
    }

    return true;
}

/**
 * @brief Get a free package slot
 * 
 * @param outIndex Optional pointer to store the index of the free slot
 * 
 * @return Dmod_PackageSlot_t* Pointer to the free package slot, or NULL if none available
 */
Dmod_PackageSlot_t* Dmod_Pck_GetFreeSlot( uint32_t *outIndex )
{
    for( size_t i = 0; i < DMOD_MAX_NUMBER_OF_PACKAGES; i++ )
    {
        if( !Dmod_Pck_IsSlotUsed( &Dmod_Packages[i] ) )
        {
            if( outIndex != NULL )
            {
                *outIndex = (uint32_t)i;
            }
            return &Dmod_Packages[i];
        }
    }
    DMOD_LOG_ERROR("No free package slots available");
    return NULL;
}

/**
 * @brief Get a package slot by its index
 * 
 * @param Index Index of the package slot
 * 
 * @return Dmod_PackageSlot_t* Pointer to the package slot, or NULL if index is out of bounds
 */
Dmod_PackageSlot_t* Dmod_Pck_GetSlotByIndex( uint32_t Index )
{
    if( Index >= DMOD_MAX_NUMBER_OF_PACKAGES )
    {
        DMOD_LOG_ERROR("Package slot index out of bounds: %u", Index);
        return NULL;
    }
    return &Dmod_Packages[Index];
}

/**
 * @brief Find a package slot by its name
 * 
 * @param PackageName Name of the package to find
 * @param outIndex Optional pointer to store the index of the found package
 * @return Dmod_PackageSlot_t* Pointer to the found package slot, or NULL if not found
 */
Dmod_PackageSlot_t* Dmod_Pck_FindSlotByName( const char* PackageName, uint32_t *outIndex )
{
    for( size_t i = 0; i < DMOD_MAX_NUMBER_OF_PACKAGES; i++ )
    {
        const char* name = Dmod_Pck_GetPackageName( &Dmod_Packages[i] );
        if( strcmp( name, PackageName ) == 0 )
        {
            if( outIndex != NULL )
            {
                *outIndex = (uint32_t)i;
            }
            return &Dmod_Packages[i];
        }
    }
    return NULL;
}

/**
 * @brief Find a package slot by its buffer pointer
 * 
 * @param Buffer Pointer to the package buffer to find
 * @return Dmod_PackageSlot_t* Pointer to the found package slot, or NULL if not found
 */
Dmod_PackageSlot_t* Dmod_Pck_FindSlotByBuffer( const void* Buffer )
{
    for( size_t i = 0; i < DMOD_MAX_NUMBER_OF_PACKAGES; i++ )
    {
        if( Dmod_Packages[i].PackageBuffer == Buffer )
        {
            return &Dmod_Packages[i];
        }
    }
    return NULL;
}

/**
 * @brief Find a package slot by its file path
 * 
 * @param FilePath Path to the package file to find
 * @return Dmod_PackageSlot_t* Pointer to the found package slot, or NULL if not found
 */
Dmod_PackageSlot_t* Dmod_Pck_FindSlotByFilePath( const char* FilePath )
{
    for( size_t i = 0; i < DMOD_MAX_NUMBER_OF_PACKAGES; i++ )
    {
        if( Dmod_Packages[i].FilePath != NULL && strcmp( Dmod_Packages[i].FilePath, FilePath ) == 0 )
        {
            return &Dmod_Packages[i];
        }
    }
    return NULL;
}

/**
 * @brief Get the number of currently loaded packages
 * 
 * @return size_t Number of loaded packages
 */
size_t Dmod_Pck_GetNumberOfPackages( void )
{
    size_t count = 0;
    for( size_t i = 0; i < DMOD_MAX_NUMBER_OF_PACKAGES; i++ )
    {
        if( Dmod_Pck_IsSlotUsed( &Dmod_Packages[i] ) )
        {
            count++;
        }
    }
    return count;
}

/**
 * @brief Find a module entry in a package slot by module name
 * 
 * @param Slot Pointer to the package slot
 * @param ModuleName Name of the module to find
 * @return Dmod_DmpModuleEntry_t* Pointer to the found module entry, or NULL if not found
 */
Dmod_DmpModuleEntry_t* Dmod_Pck_FindModuleEntry( Dmod_PackageSlot_t* Slot, const char* ModuleName )
{
    if( Slot == NULL || Slot->DmpHeader == NULL || Slot->ModuleEntries == NULL )
    {
        DMOD_LOG_ERROR("Cannot find module entry - invalid package slot");
        return NULL;
    }

    for( uint32_t i = 0; i < Slot->DmpHeader->ModuleCount; i++ )
    {
        Dmod_DmpModuleEntry_t* entry = &Slot->ModuleEntries[i];
        if( strcmp( entry->ModuleName, ModuleName ) == 0 )
        {
            return entry;
        }
    }

    DMOD_LOG_ERROR("Module '%s' not found in package '%s'", ModuleName, Dmod_Pck_GetPackageName( Slot ));
    return NULL;
}

/**
 * @brief Find a module entry in all loaded packages by module name
 * 
 * @param ModuleName Name of the module to find
 * @param outSlot Optional pointer to store the package slot where the module was found
 * @return Dmod_DmpModuleEntry_t* Pointer to the found module entry, or NULL if not found
 */
Dmod_DmpModuleEntry_t* Dmod_Pck_FindModuleEntryInPackages( const char* ModuleName, Dmod_PackageSlot_t** outSlot )
{
    for( size_t i = 0; i < DMOD_MAX_NUMBER_OF_PACKAGES; i++ )
    {
        Dmod_PackageSlot_t* slot = &Dmod_Packages[i];
        if( Dmod_Pck_IsSlotUsed( slot ) )
        {
            Dmod_DmpModuleEntry_t* entry = Dmod_Pck_FindModuleEntry( slot, ModuleName );
            if( entry != NULL )
            {
                if( outSlot != NULL )
                {
                    *outSlot = slot;
                }
                return entry;
            }
        }
    }
    return NULL;
}

/**
 * @brief Get the main module name from a package slot
 * 
 * @param Slot Pointer to the package slot
 * 
 * @return const char* Name of the main module, or NULL if invalid
 */
const char* Dmod_Pck_GetMainModuleName( Dmod_PackageSlot_t* Slot )
{
    if( Slot == NULL || Slot->DmpHeader == NULL )
    {
        DMOD_LOG_ERROR("Cannot get main module name - invalid package slot");
        return NULL;
    }

    uint32_t mainIndex = Slot->DmpHeader->MainIndex;
    if( mainIndex >= Slot->DmpHeader->ModuleCount )
    {
        DMOD_LOG_ERROR("Cannot get main module name - main index out of bounds");
        return NULL;
    }

    return Slot->ModuleEntries[mainIndex].ModuleName;
}


