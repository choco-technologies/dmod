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

/**
 * @brief Create a DMP package file from a directory of module files
 * 
 * @param PackageName Name of the package
 * @param InputDir Directory containing the module files (.dmf or .dmfc)
 * @param OutputFile Path to the output .dmp file
 * @param MainModuleName Name of the main module (can be NULL)
 * @return true If the DMP file was created successfully
 * @return false If the DMP file could not be created
 */
bool Dmod_ToDMPFile( const char* PackageName, const char* InputDir, const char* OutputFile, const char* MainModuleName )
{
    if( PackageName == NULL || InputDir == NULL || OutputFile == NULL )
    {
        DMOD_LOG_ERROR("Cannot create DMP file - invalid parameters\n");
        return false;
    }

    // Open the input directory
    void* dir = Dmod_OpenDir( InputDir );
    if( dir == NULL )
    {
        DMOD_LOG_ERROR("Cannot create DMP file - cannot open input directory '%s'\n", InputDir);
        return false;
    }

    // Count the number of module files
    uint32_t moduleCount = 0;
    const char* fileName = NULL;
    while( (fileName = Dmod_ReadDir( dir )) != NULL )
    {
        size_t len = strlen( fileName );
        if( len > 4 && (strcmp( fileName + len - 4, ".dmf" ) == 0 || strcmp( fileName + len - 5, ".dmfc" ) == 0) )
        {
            moduleCount++;
        }
    }
    Dmod_CloseDir( dir );

    if( moduleCount == 0 )
    {
        DMOD_LOG_ERROR("Cannot create DMP file - no module files found in '%s'\n", InputDir);
        return false;
    }

    // Allocate memory for module entries
    Dmod_DmpModuleEntry_t* moduleEntries = (Dmod_DmpModuleEntry_t*)Dmod_Malloc( moduleCount * sizeof(Dmod_DmpModuleEntry_t) );
    if( moduleEntries == NULL )
    {
        DMOD_LOG_ERROR("Cannot create DMP file - cannot allocate memory for module entries\n");
        return false;
    }

    // Calculate total size and fill module entries
    dir = Dmod_OpenDir( InputDir );
    if( dir == NULL )
    {
        DMOD_LOG_ERROR("Cannot create DMP file - cannot reopen input directory '%s'\n", InputDir);
        Dmod_Free( moduleEntries );
        return false;
    }

    uint32_t currentIndex = 0;
    uint32_t mainIndex = 0;
    size_t dataOffset = sizeof(Dmod_DmpHeader_t) + moduleCount * sizeof(Dmod_DmpModuleEntry_t);
    size_t totalSize = dataOffset;

    while( (fileName = Dmod_ReadDir( dir )) != NULL )
    {
        size_t len = strlen( fileName );
        bool isDmf = (len > 4 && strcmp( fileName + len - 4, ".dmf" ) == 0);
        bool isDmfc = (len > 5 && strcmp( fileName + len - 5, ".dmfc" ) == 0);
        
        if( !isDmf && !isDmfc )
        {
            continue;
        }

        // Build full path
        char filePath[DMOD_MAX_PATH_LENGTH];
        Dmod_SnPrintf( filePath, sizeof(filePath), "%s/%s", InputDir, fileName );

        // Open the file to get its size
        void* file = Dmod_FileOpen( filePath, "rb" );
        if( file == NULL )
        {
            DMOD_LOG_ERROR("Cannot create DMP file - cannot open module file '%s'\n", filePath);
            Dmod_CloseDir( dir );
            Dmod_Free( moduleEntries );
            return false;
        }

        size_t fileSize = Dmod_FileSize( file );
        Dmod_FileClose( file );

        // Extract module name from file name (without extension)
        char moduleName[DMOD_MAX_MODULE_NAME_LENGTH];
        size_t nameLen = isDmf ? len - 4 : len - 5;
        if( nameLen >= DMOD_MAX_MODULE_NAME_LENGTH )
        {
            nameLen = DMOD_MAX_MODULE_NAME_LENGTH - 1;
        }
        memcpy( moduleName, fileName, nameLen );
        moduleName[nameLen] = '\0';

        // Fill module entry
        moduleEntries[currentIndex].ModuleOffset = (uint32_t)dataOffset;
        moduleEntries[currentIndex].FileSize = (uint32_t)fileSize;
        strncpy( moduleEntries[currentIndex].ModuleName, moduleName, DMOD_MAX_MODULE_NAME_LENGTH );
        moduleEntries[currentIndex].ModuleName[DMOD_MAX_MODULE_NAME_LENGTH - 1] = '\0';

        // Check if this is the main module
        if( MainModuleName != NULL && strcmp( moduleName, MainModuleName ) == 0 )
        {
            mainIndex = currentIndex;
        }

        dataOffset += fileSize;
        totalSize += fileSize;
        currentIndex++;
    }
    Dmod_CloseDir( dir );

    // Create DMP header
    Dmod_DmpHeader_t header;
    header.Signature = DMOD_DMP_SIGNATURE;
    header.HeaderSize = sizeof(Dmod_DmpHeader_t);
    header.HeaderVersion = DMOD_DMP_VERSION;
    strncpy( header.Name, PackageName, DMOD_MAX_PACKAGE_NAME_LENGTH );
    header.Name[DMOD_MAX_PACKAGE_NAME_LENGTH - 1] = '\0';
    header.MainIndex = mainIndex;
    header.ModuleCount = moduleCount;

    // Open output file
    void* outFile = Dmod_FileOpen( OutputFile, "wb" );
    if( outFile == NULL )
    {
        DMOD_LOG_ERROR("Cannot create DMP file - cannot open output file '%s'\n", OutputFile);
        Dmod_Free( moduleEntries );
        return false;
    }

    // Write header
    if( Dmod_FileWrite( &header, sizeof(header), 1, outFile ) != 1 )
    {
        DMOD_LOG_ERROR("Cannot create DMP file - cannot write header\n");
        Dmod_FileClose( outFile );
        Dmod_Free( moduleEntries );
        return false;
    }

    // Write module entries
    if( Dmod_FileWrite( moduleEntries, sizeof(Dmod_DmpModuleEntry_t), moduleCount, outFile ) != moduleCount )
    {
        DMOD_LOG_ERROR("Cannot create DMP file - cannot write module entries\n");
        Dmod_FileClose( outFile );
        Dmod_Free( moduleEntries );
        return false;
    }

    // Write module data
    dir = Dmod_OpenDir( InputDir );
    if( dir == NULL )
    {
        DMOD_LOG_ERROR("Cannot create DMP file - cannot reopen input directory for writing data\n");
        Dmod_FileClose( outFile );
        Dmod_Free( moduleEntries );
        return false;
    }

    while( (fileName = Dmod_ReadDir( dir )) != NULL )
    {
        size_t len = strlen( fileName );
        bool isDmf = (len > 4 && strcmp( fileName + len - 4, ".dmf" ) == 0);
        bool isDmfc = (len > 5 && strcmp( fileName + len - 5, ".dmfc" ) == 0);
        
        if( !isDmf && !isDmfc )
        {
            continue;
        }

        // Build full path
        char filePath[DMOD_MAX_PATH_LENGTH];
        Dmod_SnPrintf( filePath, sizeof(filePath), "%s/%s", InputDir, fileName );

        // Open the file
        void* file = Dmod_FileOpen( filePath, "rb" );
        if( file == NULL )
        {
            DMOD_LOG_ERROR("Cannot create DMP file - cannot open module file '%s' for reading\n", filePath);
            Dmod_CloseDir( dir );
            Dmod_FileClose( outFile );
            Dmod_Free( moduleEntries );
            return false;
        }

        size_t fileSize = Dmod_FileSize( file );
        void* buffer = Dmod_Malloc( fileSize );
        if( buffer == NULL )
        {
            DMOD_LOG_ERROR("Cannot create DMP file - cannot allocate buffer for file '%s'\n", filePath);
            Dmod_FileClose( file );
            Dmod_CloseDir( dir );
            Dmod_FileClose( outFile );
            Dmod_Free( moduleEntries );
            return false;
        }

        if( Dmod_FileRead( buffer, 1, fileSize, file ) != fileSize )
        {
            DMOD_LOG_ERROR("Cannot create DMP file - cannot read file '%s'\n", filePath);
            Dmod_Free( buffer );
            Dmod_FileClose( file );
            Dmod_CloseDir( dir );
            Dmod_FileClose( outFile );
            Dmod_Free( moduleEntries );
            return false;
        }
        Dmod_FileClose( file );

        if( Dmod_FileWrite( buffer, 1, fileSize, outFile ) != fileSize )
        {
            DMOD_LOG_ERROR("Cannot create DMP file - cannot write file data '%s'\n", filePath);
            Dmod_Free( buffer );
            Dmod_CloseDir( dir );
            Dmod_FileClose( outFile );
            Dmod_Free( moduleEntries );
            return false;
        }

        Dmod_Free( buffer );
    }

    Dmod_CloseDir( dir );
    Dmod_FileClose( outFile );
    Dmod_Free( moduleEntries );

    return true;
}


