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

    if( Slot->DmpHeader->Signature != 0x48504D44 ) // 'DMPH'
    {
        DMOD_LOG_ERROR("Package slot has invalid DMP header signature: 0x%08X", Slot->DmpHeader->Signature);
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
 * @brief Get the index of a module in a package
 * 
 * @param Slot Pointer to the package slot
 * @param ModuleName Name of the module to find
 * @return uint32_t Index of the module in the package, or UINT32_MAX if not found
 */
uint32_t Dmod_Pck_GetModuleIndexInPackage( Dmod_PackageSlot_t* Slot, const char* ModuleName )
{
    if( Slot == NULL || Slot->DmpHeader == NULL || Slot->ModuleEntries == NULL )
    {
        DMOD_LOG_ERROR("Cannot get module index - invalid package slot");
        return UINT32_MAX;
    }

    for( uint32_t i = 0; i < Slot->DmpHeader->ModuleCount; i++ )
    {
        Dmod_DmpModuleEntry_t* entry = &Slot->ModuleEntries[i];
        if( strcmp( entry->ModuleName, ModuleName ) == 0 )
        {
            return i;
        }
    }

    DMOD_LOG_ERROR("Module '%s' not found in package '%s'", ModuleName, Dmod_Pck_GetPackageName( Slot ));
    return UINT32_MAX;
}




