#define DMOD_PRIVATE
#include <string.h>
#include "dmod.h"
#include "private/dmod_vars.h"
#include "private/dmod_pck.h"

/**
 * @brief Add a package from a memory buffer
 * 
 * The function adds a package to the DMOD system from a memory buffer. 
 * Please be aware that the buffer must remain valid until the package is removed.
 * 
 * This function is designed to be used when the package is inside the flash memory. 
 * 
 * @param Buffer Pointer to the package buffer
 * @param Size Size of the package buffer
 * @param outIndex Pointer to store the index of the added package
 * @return true If the package was added successfully
 * @return false If the package could not be added
 */
bool Dmod_AddPackageBuffer( const void* Buffer, size_t Size, uint32_t* outIndex )
{
    if( Buffer == NULL || Size == 0 )
    {
        DMOD_LOG_ERROR("Cannot add NULL or empty package buffer\n");
        return false;
    }

    Dmod_PackageSlot_t* slot = Dmod_Pck_GetFreeSlot(outIndex);
    if( slot == NULL )
    {
        DMOD_LOG_ERROR("Cannot add package buffer '%p', no free slots available\n", Buffer);
        return false;
    }

    slot->PackageBuffer = Buffer;
    slot->DmpHeader = (Dmod_DmpHeader_t*)Buffer;
    slot->PackageSize = Size;
    slot->ModuleEntries = (Dmod_DmpModuleEntry_t*)((uintptr_t)Buffer + slot->DmpHeader->HeaderSize);
    slot->FilePath = NULL;

    if( !Dmod_Pck_IsValidSlot(slot) )
    {
        Dmod_Pck_ReleaseSlot( slot );
        DMOD_LOG_ERROR("Cannot add package buffer '%p', invalid package format\n", Buffer);
        return false;
    }

    DMOD_LOG_INFO("Package buffer added: %s\n", Dmod_Pck_GetPackageName(slot));
    return true;
}

/**
 * @brief Add a package from a file
 * 
 * The function adds a package to the DMOD system from a file.
 * 
 * @param FilePath Path to the package file
 * @param outIndex Pointer to store the index of the added package
 * @return true If the package was added successfully
 * @return false If the package could not be added
 */
bool Dmod_AddPackageFile( const char* FilePath, uint32_t* outIndex )
{
    if( FilePath == NULL )
    {
        DMOD_LOG_ERROR("Cannot add package from NULL file path\n");
        return false;
    }

    Dmod_PackageSlot_t* slot = Dmod_Pck_GetFreeSlot(outIndex);
    if( slot == NULL )
    {
        DMOD_LOG_ERROR("Cannot add package file '%s', no free slots available\n", FilePath);
        return false;   
    }

    // Open file
    void* file = Dmod_FileOpen( FilePath, "rb" );
    if( file == NULL )
    {
        DMOD_LOG_ERROR("Cannot add package file '%s', cannot open file\n", FilePath);
        return false;   
    }

    size_t fileSize = Dmod_FileSize( file );
    if( fileSize == 0 )
    {
        DMOD_LOG_ERROR("Cannot add package file '%s', file is empty\n", FilePath);
        Dmod_FileClose( file );
        return false;
    }

    Dmod_DmpHeader_t* dmpHeader = Dmod_Malloc( fileSize );
    if( dmpHeader == NULL )
    {
        DMOD_LOG_ERROR("Cannot add package file '%s', cannot allocate memory\n", FilePath);
        Dmod_FileClose( file );
        return false;
    }

    if( Dmod_FileRead( dmpHeader, sizeof(*dmpHeader), 1, file ) != 1 )
    {
        DMOD_LOG_ERROR("Cannot add package file '%s', cannot read file\n", FilePath);
        Dmod_Free( dmpHeader );
        Dmod_FileClose( file );
        return false;
    }

    size_t entriesSize = dmpHeader->ModuleCount * sizeof(Dmod_DmpModuleEntry_t);
    Dmod_DmpModuleEntry_t* moduleEntries = Dmod_Malloc( entriesSize );
    if( moduleEntries == NULL )
    {
        DMOD_LOG_ERROR("Cannot add package file '%s', cannot allocate memory for module entries (%u)\n", FilePath, dmpHeader->ModuleCount);
        Dmod_Free( dmpHeader );
        Dmod_FileClose( file );
        return false;
    }
 
    if( Dmod_FileRead( moduleEntries, entriesSize, 1, file ) != 1 )
    {
        DMOD_LOG_ERROR("Cannot add package file '%s', cannot read module entries\n", FilePath);
        Dmod_Free( moduleEntries );
        Dmod_Free( dmpHeader );
        Dmod_FileClose( file );
        return false;
    }

    Dmod_FileClose( file );
    slot->PackageBuffer = NULL;
    slot->PackageSize = fileSize;
    slot->FilePath = FilePath;
    slot->DmpHeader = dmpHeader;
    slot->ModuleEntries = moduleEntries;
    if( !Dmod_Pck_IsValidSlot(slot) )
    {
        Dmod_Pck_ReleaseSlot( slot );
        DMOD_LOG_ERROR("Cannot add package file '%s', invalid package format\n", FilePath);
        return false;
    }

    DMOD_LOG_INFO("Package file added: %s\n", Dmod_Pck_GetPackageName(slot));
    return true;
}

/**
 * @brief Remove a package by its name
 * 
 * @param PackageName Name of the package to remove
 * @return true If the package was removed successfully
 * @return false If the package could not be removed
 */
bool Dmod_RemovePackage( const char* PackageName )
{
    if( PackageName == NULL )
    {
        DMOD_LOG_ERROR("Cannot remove package - invalid name\n");
        return false;
    }

    Dmod_PackageSlot_t* slot = Dmod_Pck_FindSlotByName( PackageName, NULL );
    if( slot == NULL )
    {
        DMOD_LOG_ERROR("Cannot remove package '%s' - package not found\n", PackageName);
        return false;
    }

    DMOD_LOG_INFO("Removing package: %s ... ", PackageName);
    Dmod_Pck_ReleaseSlot( slot );
    DMOD_LOG_INFO("Done\n");
    return true;
}

/**
 * @brief Remove a package by its buffer
 * 
 * @param Buffer Pointer to the package buffer to remove
 * @return true If the package was removed successfully
 * @return false If the package could not be removed
 */
bool Dmod_RemovePackageBuffer( const void* Buffer )
{
    if( Buffer == NULL )
    {
        DMOD_LOG_ERROR("Cannot remove package - invalid buffer\n");
        return false;
    }

    Dmod_PackageSlot_t* slot = Dmod_Pck_FindSlotByBuffer( Buffer );
    if( slot == NULL )
    {
        DMOD_LOG_ERROR("Cannot remove package '%p' - package not found\n", Buffer);
        return false;
    }

    const char* packageName = Dmod_Pck_GetPackageName( slot );
    DMOD_LOG_INFO("Package removing: %s ... ", packageName);
    Dmod_Pck_ReleaseSlot( slot );
    DMOD_LOG_INFO("Done\n");
    return true;
}

/**
 * @brief Remove a package by its file path
 * 
 * @param FilePath Path to the package file to remove
 * @return true If the package was removed successfully
 * @return false If the package could not be removed
 */
bool Dmod_RemovePackageFile( const char* FilePath )
{
    if( FilePath == NULL )
    {
        DMOD_LOG_ERROR("Cannot remove package - invalid file path\n");
        return false;
    }

    Dmod_PackageSlot_t* slot = Dmod_Pck_FindSlotByFilePath( FilePath );
    if( slot == NULL )
    {
        DMOD_LOG_ERROR("Cannot remove package file '%s' - package not found\n", FilePath);
        return false;        
    }

    const char* packageName = Dmod_Pck_GetPackageName( slot );
    DMOD_LOG_INFO("Package removing: %s ... ", packageName);
    Dmod_Pck_ReleaseSlot( slot );
    DMOD_LOG_INFO("Done\n");
    return true;
}

/**
 * @brief Check if a package is available
 * 
 * @param PackageName Name of the package to check
 * @return true If the package is available
 * @return false If the package is not available
 */
bool Dmod_IsPackageAvailable( const char* PackageName )
{
    if( PackageName == NULL )
    {
        DMOD_LOG_ERROR("Cannot check package availability - invalid name\n");
        return false;
    }

    Dmod_PackageSlot_t* slot = Dmod_Pck_FindSlotByName( PackageName, NULL );
    return slot != NULL;
}

/**
 * @brief Get the number of packages
 * 
 * @return size_t Number of packages
 */
size_t Dmod_GetNumberOfPackages( void )
{
    return Dmod_Pck_GetNumberOfPackages();
}

/**
 * @brief Get information about a package
 * 
 * @param PackageIndex Index of the package to get information about
 * @param outName Pointer to store the name of the package
 * @param NameMaxLength Maximum length of the name buffer
 * @param outSize Pointer to store the size of the package
 * @return true If the package information was retrieved successfully
 * @return false If the package information could not be retrieved
 */
bool Dmod_GetPackageInfo( uint32_t PackageIndex, char* outName, size_t NameMaxLength, size_t* outSize )
{
    if( PackageIndex >= DMOD_MAX_NUMBER_OF_PACKAGES )
    {
        DMOD_LOG_ERROR("Cannot get package info - invalid package index: %u\n", PackageIndex);
        return false;
    }

    Dmod_PackageSlot_t* slot = &Dmod_Packages[PackageIndex];
    if( !Dmod_Pck_IsValidSlot( slot ) )
    {
        DMOD_LOG_ERROR("Cannot get package info - package index %u is not valid\n", PackageIndex);
        return false;
    }

    const char* packageName = Dmod_Pck_GetPackageName( slot );
    if( outName != NULL && NameMaxLength > 0 )
    {
        strncpy( outName, packageName, NameMaxLength );
        outName[NameMaxLength - 1] = '\0';
    }

    if( outSize != NULL )
    {
        *outSize = slot->PackageSize;
    }

    return true;
}

/**
 * @brief Get the main module index from a package
 * 
 * @param PackageName Name of the package
 * @return uint32_t Index of the main module, or 0 if not found
 */
uint32_t Dmod_GetMainIndexFromPackage( const char* PackageName )
{
    if( PackageName == NULL )
    {
        DMOD_LOG_ERROR("Cannot get main index from package - invalid name\n");
        return 0;
    }

    Dmod_PackageSlot_t* slot = Dmod_Pck_FindSlotByName( PackageName, NULL );
    if( slot == NULL )
    {
        DMOD_LOG_ERROR("Cannot get main index from package '%s' - package not found\n", PackageName);
        return 0;
    }

    return slot->DmpHeader->MainIndex;
}

