/**
 * @file main.c
 * @brief mkdmrpkg - Prepare a release package directory from a .dmr resource file
 *
 * This tool parses a .dmr resource file and copies files from their [origin]
 * locations into an output directory, replicating the package structure
 * defined by the source paths in each resource entry.
 * The resulting directory can then be archived (e.g. with zip) to produce
 * the final release package.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dmod.h"
#include "dmod_resource.h"

// Maximum path length used internally
#define MKDMRPKG_MAX_PATH_LEN 1024

// Buffer size for file copy
#define MKDMRPKG_COPY_BUF_SIZE (64 * 1024)

// Maximum number of additional files that can be added via --add-file
#define MKDMRPKG_MAX_ADDITIONAL_FILES 64

// -----------------------------------------
//
//      Check whether a path is a directory
//
// -----------------------------------------
static bool IsDirectory( const char* path )
{
    void* dir = Dmod_OpenDir( path );
    if( dir == NULL )
    {
        return false;
    }
    Dmod_CloseDir( dir );
    return true;
}

// -----------------------------------------
//
//      Check whether a path is a regular file
//
// -----------------------------------------
static bool IsFile( const char* path )
{
    if( Dmod_Access( path, DMOD_F_OK ) != 0 )
    {
        return false;
    }
    // If it can also be opened as a directory, it is not a file
    void* dir = Dmod_OpenDir( path );
    if( dir != NULL )
    {
        Dmod_CloseDir( dir );
        return false;
    }
    return true;
}

// -----------------------------------------
//
//      Create all intermediate directories
//
// -----------------------------------------
static bool MakeDirs( const char* path )
{
    char tmp[MKDMRPKG_MAX_PATH_LEN];
    strncpy( tmp, path, sizeof(tmp) - 1 );
    tmp[sizeof(tmp) - 1] = '\0';

    size_t len = strlen( tmp );

    // Strip trailing slash
    if( len > 0 && tmp[len - 1] == '/' )
    {
        tmp[len - 1] = '\0';
        len--;
    }

    for( size_t i = 1; i <= len; i++ )
    {
        if( tmp[i] == '/' || tmp[i] == '\0' )
        {
            char saved = tmp[i];
            tmp[i] = '\0';

            if( Dmod_Access( tmp, DMOD_F_OK ) != 0 )
            {
                if( Dmod_MakeDir( tmp, 0755 ) != 0 )
                {
                    // Directory might have been created concurrently; verify
                    if( Dmod_Access( tmp, DMOD_F_OK ) != 0 )
                    {
                        DMOD_LOG_ERROR("Cannot create directory: %s\n", tmp);
                        return false;
                    }
                }
            }

            tmp[i] = saved;
        }
    }
    return true;
}

// -----------------------------------------
//
//      Copy a single file from src to dst
//
// -----------------------------------------
static bool CopyFile( const char* src, const char* dst )
{
    void* srcFile = Dmod_FileOpen( src, "rb" );
    if( !srcFile )
    {
        DMOD_LOG_ERROR("Cannot open source file: %s\n", src);
        return false;
    }

    // Ensure destination directory exists
    char dstDir[MKDMRPKG_MAX_PATH_LEN];
    strncpy( dstDir, dst, sizeof(dstDir) - 1 );
    dstDir[sizeof(dstDir) - 1] = '\0';

    char* lastSlash = strrchr( dstDir, '/' );
    if( lastSlash )
    {
        *lastSlash = '\0';
        if( !MakeDirs( dstDir ) )
        {
            Dmod_FileClose( srcFile );
            return false;
        }
    }

    void* dstFile = Dmod_FileOpen( dst, "wb" );
    if( !dstFile )
    {
        DMOD_LOG_ERROR("Cannot create destination file: %s\n", dst);
        Dmod_FileClose( srcFile );
        return false;
    }

    char* buf = (char*)Dmod_Malloc( MKDMRPKG_COPY_BUF_SIZE );
    if( !buf )
    {
        DMOD_LOG_ERROR("Out of memory\n");
        Dmod_FileClose( srcFile );
        Dmod_FileClose( dstFile );
        return false;
    }

    bool ok = true;
    size_t bytesRead;
    while( (bytesRead = Dmod_FileRead( buf, 1, MKDMRPKG_COPY_BUF_SIZE, srcFile )) > 0 )
    {
        size_t bytesWritten = Dmod_FileWrite( buf, 1, bytesRead, dstFile );
        if( bytesWritten != bytesRead )
        {
            DMOD_LOG_ERROR("Write failed for: %s\n", dst);
            ok = false;
            break;
        }
    }

    Dmod_Free( buf );
    Dmod_FileClose( srcFile );
    Dmod_FileClose( dstFile );
    return ok;
}

// -----------------------------------------
//
//      Recursively copy a directory tree
//
// -----------------------------------------
static bool CopyDirRecursive( const char* srcDir, const char* dstDir )
{
    if( !MakeDirs( dstDir ) )
    {
        return false;
    }

    void* dir = Dmod_OpenDir( srcDir );
    if( !dir )
    {
        DMOD_LOG_ERROR("Cannot open directory: %s\n", srcDir);
        return false;
    }

    bool ok = true;
    const Dmod_DirEntry_t* entry;
    while( (entry = Dmod_ReadDirEx( dir )) != NULL )
    {
        // Skip . and ..
        if( strcmp( entry->name, "." ) == 0 || strcmp( entry->name, ".." ) == 0 )
        {
            continue;
        }

        char srcPath[MKDMRPKG_MAX_PATH_LEN];
        char dstPath[MKDMRPKG_MAX_PATH_LEN];
        Dmod_SnPrintf( srcPath, sizeof(srcPath), "%s/%s", srcDir, entry->name );
        Dmod_SnPrintf( dstPath, sizeof(dstPath), "%s/%s", dstDir, entry->name );

        if( entry->type == Dmod_DirEntryType_Dir )
        {
            if( !CopyDirRecursive( srcPath, dstPath ) )
            {
                ok = false;
                break;
            }
        }
        else if( entry->type == Dmod_DirEntryType_File )
        {
            DMOD_LOG_VERBOSE("    Copying file: %s -> %s\n", srcPath, dstPath);
            if( !CopyFile( srcPath, dstPath ) )
            {
                ok = false;
                break;
            }
        }
    }

    Dmod_CloseDir( dir );
    return ok;
}

// -----------------------------------------
//
//      Copy one origin path into dstPath.
//      - If origin is a file and dst looks like a file: copy directly.
//      - If origin is a file and dst looks like a directory: copy file into directory.
//      - If origin is a directory: recursively copy into dst directory.
//
// -----------------------------------------
static bool CopyOrigin( const char* origin, const char* dst )
{
    if( IsFile( origin ) )
    {
        // Check whether dst itself is (or should be) a file
        // We treat dst as a file when it already exists as one, or when
        // it has the same basename as origin (e.g. source=./module.dmf)
        bool dstIsDir = IsDirectory( dst );

        if( dstIsDir )
        {
            // Copy the file into the directory using origin's basename
            const char* baseName = strrchr( origin, '/' );
            baseName = baseName ? baseName + 1 : origin;

            char targetPath[MKDMRPKG_MAX_PATH_LEN];
            Dmod_SnPrintf( targetPath, sizeof(targetPath), "%s/%s", dst, baseName );
            DMOD_LOG_VERBOSE("    Copying file: %s -> %s\n", origin, targetPath);
            return CopyFile( origin, targetPath );
        }
        else
        {
            DMOD_LOG_VERBOSE("    Copying file: %s -> %s\n", origin, dst);
            return CopyFile( origin, dst );
        }
    }
    else if( IsDirectory( origin ) )
    {
        DMOD_LOG_VERBOSE("    Copying directory: %s -> %s\n", origin, dst);
        return CopyDirRecursive( origin, dst );
    }
    else
    {
        DMOD_LOG_WARN("Origin path does not exist or is not accessible: %s\n", origin);
        return true; // Non-fatal: skip missing origins
    }
}

// -----------------------------------------
//
//      Prints usage message
//
// -----------------------------------------
static void PrintUsage( const char* AppName )
{
    printf("Usage: %s <file.dmr> [options]\n", AppName);
}

// -----------------------------------------
//
//      Prints help message
//
// -----------------------------------------
static void PrintHelp( const char* AppName )
{
    printf("-- Release Package Maker ver. " DMOD_VERSION_STRING " --\n\n");
    printf("This tool parses a .dmr resource file and copies files from their\n");
    printf("[origin] locations into an output directory, reproducing the package\n");
    printf("structure. The directory can then be archived to create a release package.\n\n");
    PrintUsage( AppName );
    printf("\nOptions:\n");
    printf("  -h, --help            Print this help message\n");
    printf("  -v, --version         Print version information\n");
    printf("  --verbose             Enable verbose output\n");
    printf("  -o <dir>              Output directory (default: package name or ./package)\n");
    printf("  -n, --name <name>     Package name; used as output directory when -o is not set\n");
    printf("  --add-file <path>     Copy an extra file into the output directory root\n");
    printf("                        (can be specified multiple times)\n");
    printf("  -d <destination>      Value for ${destination} variable substitution\n");
    printf("  -m <module>           Value for ${module} variable substitution\n");
    printf("  -r <repo_dir>         Value for ${repo_dir} variable substitution\n");
    printf("  --dmf-dir <dir>       Value for ${dmf_dir} variable substitution\n");
    printf("  -b <build_dir>        Value for ${build_dir} variable substitution\n");
    printf("\nArguments:\n");
    printf("  <file.dmr>            Path to the .dmr resource file\n");
    printf("\nExamples:\n");
    printf("  %s module.dmr -m mymodule -n mymodule-1.0.0 -o ./release_package\n", AppName);
    printf("  %s module.dmr -m mymodule -n mymodule-1.0.0 --add-file release-notes.txt\n", AppName);
    printf("  %s module.dmr -m mymodule -r /path/to/repo -b /path/to/build -o /tmp/pkg\n", AppName);
    printf("\nDescription:\n");
    printf("  For each resource entry in the .dmr file that has one or more [origin]\n");
    printf("  directives, the tool copies files/directories from those origin paths\n");
    printf("  into the output directory at the location specified by the entry's source\n");
    printf("  path. Entries without [origin] directives are skipped.\n");
}

// -----------------------------------------
//
//      Main function
//
// -----------------------------------------
int main( int argc, char *argv[] )
{
    // Set default log level to Warning
    Dmod_SetLogLevel( Dmod_LogLevel_Warn );

    if( argc < 2 )
    {
        PrintUsage( argv[0] );
        return 0;
    }

    if( strcmp( argv[1], "-h" ) == 0 || strcmp( argv[1], "--help" ) == 0 )
    {
        PrintHelp( argv[0] );
        return 0;
    }

    if( strcmp( argv[1], "-v" ) == 0 || strcmp( argv[1], "--version" ) == 0 )
    {
        printf("Dynamic Module Loader ver. " DMOD_VERSION_STRING "\n");
        return 0;
    }

    const char* dmrPath      = argv[1];
    const char* outputDir    = NULL;
    const char* packageName  = NULL;
    const char* destination  = "";
    const char* moduleName   = "";
    const char* repoDir      = NULL;
    const char* dmfDir       = NULL;
    const char* buildDir     = NULL;

    const char* additionalFiles[MKDMRPKG_MAX_ADDITIONAL_FILES];
    int additionalFileCount = 0;

    // Parse optional arguments
    for( int i = 2; i < argc; i++ )
    {
        if( strcmp( argv[i], "--verbose" ) == 0 )
        {
            Dmod_SetLogLevel( Dmod_LogLevel_Verbose );
        }
        else if( strcmp( argv[i], "-o" ) == 0 )
        {
            if( i + 1 < argc )
            {
                outputDir = argv[++i];
            }
            else
            {
                printf("Error: -o option requires a directory path\n");
                PrintUsage( argv[0] );
                return -1;
            }
        }
        else if( strcmp( argv[i], "-n" ) == 0 || strcmp( argv[i], "--name" ) == 0 )
        {
            if( i + 1 < argc )
            {
                packageName = argv[++i];
            }
            else
            {
                printf("Error: %s option requires a package name\n", argv[i]);
                PrintUsage( argv[0] );
                return -1;
            }
        }
        else if( strcmp( argv[i], "--add-file" ) == 0 )
        {
            if( i + 1 < argc )
            {
                if( additionalFileCount < MKDMRPKG_MAX_ADDITIONAL_FILES )
                {
                    additionalFiles[additionalFileCount++] = argv[++i];
                }
                else
                {
                    printf("Error: Too many --add-file arguments (max %d)\n",
                           MKDMRPKG_MAX_ADDITIONAL_FILES);
                    return -1;
                }
            }
            else
            {
                printf("Error: --add-file option requires a file path\n");
                PrintUsage( argv[0] );
                return -1;
            }
        }
        else if( strcmp( argv[i], "-d" ) == 0 )
        {
            if( i + 1 < argc )
            {
                destination = argv[++i];
            }
            else
            {
                printf("Error: -d option requires a value\n");
                PrintUsage( argv[0] );
                return -1;
            }
        }
        else if( strcmp( argv[i], "-m" ) == 0 )
        {
            if( i + 1 < argc )
            {
                moduleName = argv[++i];
            }
            else
            {
                printf("Error: -m option requires a value\n");
                PrintUsage( argv[0] );
                return -1;
            }
        }
        else if( strcmp( argv[i], "-r" ) == 0 )
        {
            if( i + 1 < argc )
            {
                repoDir = argv[++i];
            }
            else
            {
                printf("Error: -r option requires a value\n");
                PrintUsage( argv[0] );
                return -1;
            }
        }
        else if( strcmp( argv[i], "--dmf-dir" ) == 0 )
        {
            if( i + 1 < argc )
            {
                dmfDir = argv[++i];
            }
            else
            {
                printf("Error: --dmf-dir option requires a value\n");
                PrintUsage( argv[0] );
                return -1;
            }
        }
        else if( strcmp( argv[i], "-b" ) == 0 )
        {
            if( i + 1 < argc )
            {
                buildDir = argv[++i];
            }
            else
            {
                printf("Error: -b option requires a value\n");
                PrintUsage( argv[0] );
                return -1;
            }
        }
        else
        {
            printf("Error: Unknown argument: %s\n", argv[i]);
            PrintUsage( argv[0] );
            return -1;
        }
    }

    // Resolve the output directory: explicit -o takes priority, then --name, then default
    if( outputDir == NULL )
    {
        outputDir = (packageName != NULL) ? packageName : "package";
    }

    Dmod_Printf("Parsing resource file: %s\n", dmrPath);

    // Initialize Dmod system
    if( !Dmod_Initialize( 0, 0 ) )
    {
        DMOD_LOG_ERROR("Failed to initialize Dmod system\n");
        return -1;
    }

    // Initialize resource context
    Dmod_ResourceContext_t* ctx = Dmod_Resource_Init(
        destination, moduleName, repoDir, dmfDir, buildDir );

    if( !ctx )
    {
        DMOD_LOG_ERROR("Failed to initialize resource context\n");
        Dmod_Deinitialize();
        return -1;
    }

    // Parse the .dmr file
    if( !Dmod_Resource_ParseFile( ctx, dmrPath ) )
    {
        const char* err = Dmod_Resource_GetError( ctx );
        DMOD_LOG_ERROR("Failed to parse resource file: %s\n", err ? err : "<unknown>");
        Dmod_Resource_Free( ctx );
        Dmod_Deinitialize();
        return -1;
    }

    size_t entryCount = Dmod_Resource_GetEntryCount( ctx );
    Dmod_Printf("Found %zu resource entr%s in: %s\n", entryCount,
                entryCount == 1 ? "y" : "ies", dmrPath);

    // Create the output directory
    if( !MakeDirs( outputDir ) )
    {
        Dmod_Resource_Free( ctx );
        Dmod_Deinitialize();
        return -1;
    }

    int copiedEntries  = 0;
    int skippedEntries = 0;
    int errorCount     = 0;

    for( size_t i = 0; i < entryCount; i++ )
    {
        Dmod_ResourceEntry_t entry;
        if( !Dmod_Resource_GetEntry( ctx, i, &entry ) )
        {
            continue;
        }

        if( entry.origin_count == 0 )
        {
            DMOD_LOG_VERBOSE("  Skipping entry '%s' (no [origin] directive)\n", entry.key);
            skippedEntries++;
            continue;
        }

        // Build the destination path inside the output directory.
        // entry.source may start with "./" - normalise it.
        const char* sourcePath = entry.source;
        if( sourcePath[0] == '.' && sourcePath[1] == '/' )
        {
            sourcePath += 2;
        }

        char destPath[MKDMRPKG_MAX_PATH_LEN];
        Dmod_SnPrintf( destPath, sizeof(destPath), "%s/%s", outputDir, sourcePath );

        Dmod_Printf("  [%s] %s\n", entry.key, destPath);

        bool entryOk = true;
        for( size_t j = 0; j < entry.origin_count; j++ )
        {
            Dmod_Printf("    <- %s\n", entry.origins[j]);
            if( !CopyOrigin( entry.origins[j], destPath ) )
            {
                errorCount++;
                entryOk = false;
            }
        }

        if( entryOk )
        {
            copiedEntries++;
        }
    }

    Dmod_Resource_Free( ctx );
    Dmod_Deinitialize();

    // Copy additional files specified via --add-file into the output directory root
    for( int i = 0; i < additionalFileCount; i++ )
    {
        const char* filePath = additionalFiles[i];
        const char* baseName = strrchr( filePath, '/' );
        baseName = baseName ? baseName + 1 : filePath;

        char destPath[MKDMRPKG_MAX_PATH_LEN];
        Dmod_SnPrintf( destPath, sizeof(destPath), "%s/%s", outputDir, baseName );

        Dmod_Printf("  [extra] %s -> %s\n", filePath, destPath);
        if( !CopyFile( filePath, destPath ) )
        {
            DMOD_LOG_ERROR("Failed to copy additional file: %s\n", filePath);
            errorCount++;
        }
    }

    // Print summary
    Dmod_Printf("\nSummary:\n");
    Dmod_Printf("  Entries copied:  %d\n", copiedEntries);
    Dmod_Printf("  Entries skipped: %d (no [origin] directive)\n", skippedEntries);
    if( errorCount > 0 )
    {
        Dmod_Printf("  Errors:          %d\n", errorCount);
    }
    Dmod_Printf("  Output directory: %s\n", outputDir);

    if( errorCount > 0 )
    {
        DMOD_LOG_WARN("Package creation finished with errors.\n");
        return -1;
    }

    Dmod_Printf("\nSuccess! Package directory created successfully.\n");
    return 0;
}
