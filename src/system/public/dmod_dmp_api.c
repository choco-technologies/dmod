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

    // Print list of modules being added
    DMOD_LOG_INFO("Adding %u module(s) to package '%s':\n", moduleCount, PackageName);
    for( uint32_t i = 0; i < moduleCount; i++ )
    {
        DMOD_LOG_INFO("  [%u] %s (size: %u bytes, offset: %u)%s\n", 
            i, 
            moduleEntries[i].ModuleName, 
            moduleEntries[i].FileSize,
            moduleEntries[i].ModuleOffset,
            (i == mainIndex) ? " [MAIN]" : "");
    }

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

/**
 * @brief Helper structure to track modules for packaging
 */
typedef struct 
{
    char ModuleName[DMOD_MAX_MODULE_NAME_LENGTH];
    char FilePath[DMOD_MAX_PATH_LENGTH];
    bool Found;
} Dmod_ModuleToPackage_t;

/**
 * @brief Extract module dependencies from a module file
 * 
 * @param FilePath Path to the module file
 * @param outDependencies Array to store dependencies (module names)
 * @param MaxDependencies Maximum number of dependencies to extract
 * @param outCount Number of dependencies found
 * @return true If dependencies were extracted successfully
 * @return false If dependencies could not be extracted
 */
static bool Dmod_ExtractModuleDependencies( const char* FilePath, char (*outDependencies)[DMOD_MAX_MODULE_NAME_LENGTH], size_t MaxDependencies, size_t* outCount )
{
    if( FilePath == NULL || outDependencies == NULL || outCount == NULL )
    {
        DMOD_LOG_ERROR("Cannot extract dependencies - invalid parameters\n");
        return false;
    }

    *outCount = 0;

    // Open the file
    void* file = Dmod_FileOpen( FilePath, "rb" );
    if( file == NULL )
    {
        DMOD_LOG_ERROR("Cannot extract dependencies - cannot open file '%s'\n", FilePath);
        return false;
    }

    size_t fileSize = Dmod_FileSize( file );
    if( fileSize == 0 )
    {
        DMOD_LOG_ERROR("Cannot extract dependencies - file is empty '%s'\n", FilePath);
        Dmod_FileClose( file );
        return false;
    }

    // Read the entire file into memory
    void* fileData = Dmod_Malloc( fileSize );
    if( fileData == NULL )
    {
        DMOD_LOG_ERROR("Cannot extract dependencies - cannot allocate memory\n");
        Dmod_FileClose( file );
        return false;
    }

    if( Dmod_FileRead( fileData, 1, fileSize, file ) != fileSize )
    {
        DMOD_LOG_ERROR("Cannot extract dependencies - cannot read file\n");
        Dmod_Free( fileData );
        Dmod_FileClose( file );
        return false;
    }
    Dmod_FileClose( file );

    // Check if it's a DMFC file - if so, decompress it first
    if( Dmod_IsDMFC( fileData, fileSize ) )
    {
        void* dmfData = NULL;
        size_t dmfSize = 0;
        if( !Dmod_FromDMFC( fileData, fileSize, &dmfData, &dmfSize ) )
        {
            DMOD_LOG_ERROR("Cannot extract dependencies - cannot decompress DMFC file\n");
            Dmod_Free( fileData );
            return false;
        }
        Dmod_Free( fileData );
        fileData = dmfData;
        fileSize = dmfSize;
    }

    // Read module header
    Dmod_ModuleHeader_t* header = (Dmod_ModuleHeader_t*)fileData;
    if( header->Signature != DMOD_HEADER_SIGNATURE )
    {
        DMOD_LOG_ERROR("Cannot extract dependencies - invalid module signature\n");
        Dmod_Free( fileData );
        return false;
    }

    // Get the footer
    if( header->Footer == NULL )
    {
        // No footer, no dependencies
        Dmod_Free( fileData );
        return true;
    }

    // Calculate footer offset
    size_t footerOffset = (size_t)header->Footer;
    if( footerOffset + sizeof(Dmod_ModuleFooter_t) > fileSize )
    {
        DMOD_LOG_ERROR("Cannot extract dependencies - footer out of bounds\n");
        Dmod_Free( fileData );
        return false;
    }

    Dmod_ModuleFooter_t* footer = (Dmod_ModuleFooter_t*)((uint8_t*)fileData + footerOffset);
    
    // Check if there's an inputs section
    if( footer->Inputs.SectionStart == 0 || footer->Inputs.SectionSize == 0 )
    {
        // No inputs section, no dependencies
        Dmod_Free( fileData );
        return true;
    }

    // Read inputs section
    size_t inputsOffset = footer->Inputs.SectionStart;
    size_t inputsSize = footer->Inputs.SectionSize;

    if( inputsOffset + inputsSize > fileSize )
    {
        DMOD_LOG_ERROR("Cannot extract dependencies - inputs section out of bounds\n");
        Dmod_Free( fileData );
        return false;
    }

    Dmod_InputsSection_t* inputsSection = (Dmod_InputsSection_t*)((uint8_t*)fileData + inputsOffset);
    size_t numberOfInputs = inputsSize / sizeof( inputsSection->Entries[0] );

    // Extract unique module names from input signatures
    for( size_t i = 0; i < numberOfInputs && *outCount < MaxDependencies; i++ )
    {
        if( inputsSection->Entries[i].Signature == NULL )
        {
            continue;
        }

        // Calculate signature offset
        size_t signatureOffset = (size_t)inputsSection->Entries[i].Signature;
        if( signatureOffset >= fileSize )
        {
            continue;
        }

        const char* signature = (const char*)((uint8_t*)fileData + signatureOffset);
        
        // Extract module name from signature
        char moduleName[DMOD_MAX_MODULE_NAME_LENGTH];
        if( !Dmod_ApiSignature_ReadModuleName( signature, moduleName, sizeof(moduleName) ) )
        {
            continue;
        }

        // Skip empty module names
        if( strlen(moduleName) == 0 )
        {
            continue;
        }

        // Check if this module is already in the list
        bool alreadyAdded = false;
        for( size_t j = 0; j < *outCount; j++ )
        {
            if( strcmp( outDependencies[j], moduleName ) == 0 )
            {
                alreadyAdded = true;
                break;
            }
        }

        if( !alreadyAdded )
        {
            strncpy( outDependencies[*outCount], moduleName, DMOD_MAX_MODULE_NAME_LENGTH );
            outDependencies[*outCount][DMOD_MAX_MODULE_NAME_LENGTH - 1] = '\0';
            (*outCount)++;
        }
    }

    Dmod_Free( fileData );
    return true;
}

/**
 * @brief Search for a module file in repository paths
 * 
 * @param ModuleName Name of the module to search for
 * @param outPath Buffer to store the found module path
 * @param PathMaxLength Maximum length of the path buffer
 * @return true If the module was found
 * @return false If the module was not found
 */
static bool Dmod_SearchModuleInRepoPaths( const char* ModuleName, char* outPath, size_t PathMaxLength )
{
    if( ModuleName == NULL || outPath == NULL )
    {
        return false;
    }

    // Get repository paths from environment
    const char* repoPaths = Dmod_GetEnv("DMOD_REPO_PATHS");
    char* repoEnv = NULL;
    size_t repoEnvSize = 0;
    
    if( repoPaths != NULL )
    {
        repoEnvSize = strlen(repoPaths);
        if( repoEnvSize > 0 )
        {
            repoEnv = Dmod_Malloc(repoEnvSize + 1);
            if( repoEnv == NULL )
            {
                return false;
            }
            strcpy(repoEnv, repoPaths);
        }
    }

    const char* repoDir = repoEnv != NULL ? strtok(repoEnv, DMOD_ARRAY_SEP) : NULL;
    if( repoDir == NULL )
    {
        repoDir = Dmod_GetRepoDir();
        repoPaths = NULL;
    }

    bool lastTry = false;
    bool found = false;

    do 
    {
        if( repoDir != NULL )
        {
            // Try .dmf extension
            Dmod_SnPrintf( outPath, PathMaxLength, "%s/%s.dmf", repoDir, ModuleName );
            if( Dmod_FileAvailable( outPath ) )
            {
                found = true;
                break;
            }

            // Try .dmfc extension
            Dmod_SnPrintf( outPath, PathMaxLength, "%s/%s.dmfc", repoDir, ModuleName );
            if( Dmod_FileAvailable( outPath ) )
            {
                found = true;
                break;
            }
        }

        if( repoPaths != NULL )
        {
            repoDir = strtok(NULL, DMOD_ARRAY_SEP);
            if( repoDir == NULL )
            {
                repoDir = Dmod_GetRepoDir();
                lastTry = true;
            }
        }
        else
        {
            break;
        }
    } while( repoDir != NULL && !lastTry );

    if( repoEnv != NULL )
    {
        Dmod_Free( repoEnv );
    }

    return found;
}

/**
 * @brief Recursively collect all module dependencies
 * 
 * @param ModulePath Path to the main module file
 * @param modules Array to store modules to package
 * @param moduleCount Current count of modules in the array
 * @param maxModules Maximum number of modules
 * @return true If dependencies were collected successfully
 * @return false If there was an error
 */
static bool Dmod_CollectModuleDependencies( const char* ModulePath, Dmod_ModuleToPackage_t* modules, size_t* moduleCount, size_t maxModules )
{
    if( ModulePath == NULL || modules == NULL || moduleCount == NULL )
    {
        return false;
    }

    if( *moduleCount >= maxModules )
    {
        DMOD_LOG_ERROR("Cannot collect dependencies - too many modules (max: %zu)\n", maxModules);
        return false;
    }

    // Extract module name from file path
    const char* fileName = strrchr( ModulePath, '/' );
    if( fileName == NULL )
    {
        fileName = ModulePath;
    }
    else
    {
        fileName++; // Skip the '/'
    }

    char currentModuleName[DMOD_MAX_MODULE_NAME_LENGTH];
    size_t nameLen = strlen(fileName);
    
    // Remove .dmf or .dmfc extension
    if( nameLen > 4 && strcmp( fileName + nameLen - 4, ".dmf" ) == 0 )
    {
        nameLen -= 4;
    }
    else if( nameLen > 5 && strcmp( fileName + nameLen - 5, ".dmfc" ) == 0 )
    {
        nameLen -= 5;
    }

    if( nameLen >= DMOD_MAX_MODULE_NAME_LENGTH )
    {
        nameLen = DMOD_MAX_MODULE_NAME_LENGTH - 1;
    }

    memcpy( currentModuleName, fileName, nameLen );
    currentModuleName[nameLen] = '\0';

    // Check if this module is already in the list
    for( size_t i = 0; i < *moduleCount; i++ )
    {
        if( strcmp( modules[i].ModuleName, currentModuleName ) == 0 )
        {
            // Already processed
            return true;
        }
    }

    // Add current module to the list
    strncpy( modules[*moduleCount].ModuleName, currentModuleName, DMOD_MAX_MODULE_NAME_LENGTH );
    modules[*moduleCount].ModuleName[DMOD_MAX_MODULE_NAME_LENGTH - 1] = '\0';
    strncpy( modules[*moduleCount].FilePath, ModulePath, DMOD_MAX_PATH_LENGTH );
    modules[*moduleCount].FilePath[DMOD_MAX_PATH_LENGTH - 1] = '\0';
    modules[*moduleCount].Found = true;
    (*moduleCount)++;

    // Extract dependencies from this module
    char dependencies[DMOD_MAX_REQUIRED_MODULES][DMOD_MAX_MODULE_NAME_LENGTH];
    size_t depCount = 0;

    if( !Dmod_ExtractModuleDependencies( ModulePath, dependencies, DMOD_MAX_REQUIRED_MODULES, &depCount ) )
    {
        DMOD_LOG_ERROR("Cannot collect dependencies - failed to extract dependencies from '%s'\n", ModulePath);
        return false;
    }

    // Recursively process each dependency
    for( size_t i = 0; i < depCount; i++ )
    {
        // Search for the dependency module
        char depPath[DMOD_MAX_PATH_LENGTH];
        if( Dmod_SearchModuleInRepoPaths( dependencies[i], depPath, sizeof(depPath) ) )
        {
            DMOD_LOG_INFO("Found dependency: %s -> %s\n", dependencies[i], depPath);
            if( !Dmod_CollectModuleDependencies( depPath, modules, moduleCount, maxModules ) )
            {
                return false;
            }
        }
        else
        {
            DMOD_LOG_WARN("Dependency not found: %s (required by %s)\n", dependencies[i], currentModuleName);
        }
    }

    return true;
}

/**
 * @brief Create a DMP package file with dependencies
 * 
 * This function creates a DMP package that includes the main module and all its dependencies.
 * Dependencies are searched in the paths specified by the DMOD_REPO_PATHS environment variable.
 * 
 * @param PackageName Name of the package
 * @param MainModulePath Path to the main module file
 * @param OutputFile Path to the output .dmp file
 * @return true If the DMP file was created successfully
 * @return false If the DMP file could not be created
 */
bool Dmod_ToDMPFileWithDependencies( const char* PackageName, const char* MainModulePath, const char* OutputFile )
{
    if( PackageName == NULL || MainModulePath == NULL || OutputFile == NULL )
    {
        DMOD_LOG_ERROR("Cannot create DMP file with dependencies - invalid parameters\n");
        return false;
    }

    // Check if main module file exists
    if( !Dmod_FileAvailable( MainModulePath ) )
    {
        DMOD_LOG_ERROR("Cannot create DMP file with dependencies - main module file not found: '%s'\n", MainModulePath);
        return false;
    }

    // Collect all modules (main + dependencies)
    Dmod_ModuleToPackage_t modules[DMOD_MAX_REQUIRED_MODULES + 1]; // +1 for main module
    size_t moduleCount = 0;

    DMOD_LOG_INFO("Collecting dependencies for module: %s\n", MainModulePath);
    if( !Dmod_CollectModuleDependencies( MainModulePath, modules, &moduleCount, DMOD_MAX_REQUIRED_MODULES + 1 ) )
    {
        DMOD_LOG_ERROR("Cannot create DMP file with dependencies - failed to collect dependencies\n");
        return false;
    }

    if( moduleCount == 0 )
    {
        DMOD_LOG_ERROR("Cannot create DMP file with dependencies - no modules to package\n");
        return false;
    }

    DMOD_LOG_INFO("Total modules to package: %zu\n", moduleCount);

    // Allocate memory for module entries
    Dmod_DmpModuleEntry_t* moduleEntries = (Dmod_DmpModuleEntry_t*)Dmod_Malloc( moduleCount * sizeof(Dmod_DmpModuleEntry_t) );
    if( moduleEntries == NULL )
    {
        DMOD_LOG_ERROR("Cannot create DMP file with dependencies - cannot allocate memory for module entries\n");
        return false;
    }

    // Calculate total size and fill module entries
    uint32_t mainIndex = 0;
    size_t dataOffset = sizeof(Dmod_DmpHeader_t) + moduleCount * sizeof(Dmod_DmpModuleEntry_t);
    size_t totalSize = dataOffset;

    for( size_t i = 0; i < moduleCount; i++ )
    {
        // Open the file to get its size
        void* file = Dmod_FileOpen( modules[i].FilePath, "rb" );
        if( file == NULL )
        {
            DMOD_LOG_ERROR("Cannot create DMP file with dependencies - cannot open module file '%s'\n", modules[i].FilePath);
            Dmod_Free( moduleEntries );
            return false;
        }

        size_t fileSize = Dmod_FileSize( file );
        Dmod_FileClose( file );

        // Fill module entry
        moduleEntries[i].ModuleOffset = (uint32_t)dataOffset;
        moduleEntries[i].FileSize = (uint32_t)fileSize;
        strncpy( moduleEntries[i].ModuleName, modules[i].ModuleName, DMOD_MAX_MODULE_NAME_LENGTH );
        moduleEntries[i].ModuleName[DMOD_MAX_MODULE_NAME_LENGTH - 1] = '\0';

        // Main module is always the first one (index 0)
        if( i == 0 )
        {
            mainIndex = 0;
        }

        dataOffset += fileSize;
        totalSize += fileSize;
    }

    // Print list of modules being added
    DMOD_LOG_INFO("Adding %zu module(s) to package '%s':\n", moduleCount, PackageName);
    for( size_t i = 0; i < moduleCount; i++ )
    {
        DMOD_LOG_INFO("  [%zu] %s (size: %u bytes, offset: %u)%s\n", 
            i, 
            moduleEntries[i].ModuleName, 
            moduleEntries[i].FileSize,
            moduleEntries[i].ModuleOffset,
            (i == mainIndex) ? " [MAIN]" : "");
    }

    // Create DMP header
    Dmod_DmpHeader_t header;
    header.Signature = DMOD_DMP_SIGNATURE;
    header.HeaderSize = sizeof(Dmod_DmpHeader_t);
    header.HeaderVersion = DMOD_DMP_VERSION;
    strncpy( header.Name, PackageName, DMOD_MAX_PACKAGE_NAME_LENGTH );
    header.Name[DMOD_MAX_PACKAGE_NAME_LENGTH - 1] = '\0';
    header.MainIndex = mainIndex;
    header.ModuleCount = (uint32_t)moduleCount;

    // Open output file
    void* outFile = Dmod_FileOpen( OutputFile, "wb" );
    if( outFile == NULL )
    {
        DMOD_LOG_ERROR("Cannot create DMP file with dependencies - cannot open output file '%s'\n", OutputFile);
        Dmod_Free( moduleEntries );
        return false;
    }

    // Write header
    if( Dmod_FileWrite( &header, sizeof(header), 1, outFile ) != 1 )
    {
        DMOD_LOG_ERROR("Cannot create DMP file with dependencies - cannot write header\n");
        Dmod_FileClose( outFile );
        Dmod_Free( moduleEntries );
        return false;
    }

    // Write module entries
    if( Dmod_FileWrite( moduleEntries, sizeof(Dmod_DmpModuleEntry_t), moduleCount, outFile ) != moduleCount )
    {
        DMOD_LOG_ERROR("Cannot create DMP file with dependencies - cannot write module entries\n");
        Dmod_FileClose( outFile );
        Dmod_Free( moduleEntries );
        return false;
    }

    // Write module data
    for( size_t i = 0; i < moduleCount; i++ )
    {
        // Open the file
        void* file = Dmod_FileOpen( modules[i].FilePath, "rb" );
        if( file == NULL )
        {
            DMOD_LOG_ERROR("Cannot create DMP file with dependencies - cannot open module file '%s' for reading\n", modules[i].FilePath);
            Dmod_FileClose( outFile );
            Dmod_Free( moduleEntries );
            return false;
        }

        size_t fileSize = Dmod_FileSize( file );
        void* buffer = Dmod_Malloc( fileSize );
        if( buffer == NULL )
        {
            DMOD_LOG_ERROR("Cannot create DMP file with dependencies - cannot allocate buffer for file '%s'\n", modules[i].FilePath);
            Dmod_FileClose( file );
            Dmod_FileClose( outFile );
            Dmod_Free( moduleEntries );
            return false;
        }

        if( Dmod_FileRead( buffer, 1, fileSize, file ) != fileSize )
        {
            DMOD_LOG_ERROR("Cannot create DMP file with dependencies - cannot read file '%s'\n", modules[i].FilePath);
            Dmod_Free( buffer );
            Dmod_FileClose( file );
            Dmod_FileClose( outFile );
            Dmod_Free( moduleEntries );
            return false;
        }
        Dmod_FileClose( file );

        if( Dmod_FileWrite( buffer, 1, fileSize, outFile ) != fileSize )
        {
            DMOD_LOG_ERROR("Cannot create DMP file with dependencies - cannot write file data '%s'\n", modules[i].FilePath);
            Dmod_Free( buffer );
            Dmod_FileClose( outFile );
            Dmod_Free( moduleEntries );
            return false;
        }

        Dmod_Free( buffer );
    }

    Dmod_FileClose( outFile );
    Dmod_Free( moduleEntries );

    return true;
}


