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
#include <sys/stat.h>
#include <errno.h>
#include "dmod.h"
#include "dmod_resource.h"

// Maximum path length used internally
#define MKDMRPKG_MAX_PATH_LEN 1024

// Buffer size for file copy
#define MKDMRPKG_COPY_BUF_SIZE (64 * 1024)

// -----------------------------------------
//
//      Check whether a path is a directory
//
// -----------------------------------------
static bool IsDirectory( const char* path )
{
    struct stat st;
    if( stat( path, &st ) != 0 )
    {
        return false;
    }
    return S_ISDIR( st.st_mode );
}

// -----------------------------------------
//
//      Check whether a path is a regular file
//
// -----------------------------------------
static bool IsFile( const char* path )
{
    struct stat st;
    if( stat( path, &st ) != 0 )
    {
        return false;
    }
    return S_ISREG( st.st_mode );
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

            struct stat st;
            if( stat( tmp, &st ) != 0 )
            {
                if( Dmod_MakeDir( tmp, 0755 ) != 0 && errno != EEXIST )
                {
                    printf("Error: Cannot create directory: %s (%s)\n", tmp, strerror(errno));
                    return false;
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
        printf("Error: Cannot open source file: %s\n", src);
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
        printf("Error: Cannot create destination file: %s\n", dst);
        Dmod_FileClose( srcFile );
        return false;
    }

    char* buf = (char*)Dmod_Malloc( MKDMRPKG_COPY_BUF_SIZE );
    if( !buf )
    {
        printf("Error: Out of memory\n");
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
            printf("Error: Write failed for: %s\n", dst);
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
        printf("Error: Cannot open directory: %s\n", srcDir);
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
        struct stat dstStat;
        bool dstIsDir = (stat( dst, &dstStat ) == 0 && S_ISDIR( dstStat.st_mode ));

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
        printf("Warning: Origin path does not exist or is not accessible: %s\n", origin);
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
    printf("  -o <dir>              Output directory (default: ./package)\n");
    printf("  -d <destination>      Value for ${destination} variable substitution\n");
    printf("  -m <module>           Value for ${module} variable substitution\n");
    printf("  -r <repo_dir>         Value for ${repo_dir} variable substitution\n");
    printf("  --dmf-dir <dir>       Value for ${dmf_dir} variable substitution\n");
    printf("  -b <build_dir>        Value for ${build_dir} variable substitution\n");
    printf("\nArguments:\n");
    printf("  <file.dmr>            Path to the .dmr resource file\n");
    printf("\nExamples:\n");
    printf("  %s module.dmr -m mymodule -o ./release_package\n", AppName);
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
    const char* outputDir    = "package";
    const char* destination  = "";
    const char* moduleName   = "";
    const char* repoDir      = NULL;
    const char* dmfDir       = NULL;
    const char* buildDir     = NULL;

    // Parse optional arguments
    for( int i = 2; i < argc; i++ )
    {
        if( strcmp( argv[i], "-o" ) == 0 )
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

    printf("Parsing resource file: %s\n", dmrPath);

    // Initialize Dmod system
    if( !Dmod_Initialize( 0, 0 ) )
    {
        printf("Error: Failed to initialize Dmod system\n");
        return -1;
    }

    // Initialize resource context
    Dmod_ResourceContext_t* ctx = Dmod_Resource_Init(
        destination, moduleName, repoDir, dmfDir, buildDir );

    if( !ctx )
    {
        printf("Error: Failed to initialize resource context\n");
        Dmod_Deinitialize();
        return -1;
    }

    // Parse the .dmr file
    if( !Dmod_Resource_ParseFile( ctx, dmrPath ) )
    {
        const char* err = Dmod_Resource_GetError( ctx );
        printf("Error: Failed to parse resource file: %s\n", err ? err : "<unknown>");
        Dmod_Resource_Free( ctx );
        Dmod_Deinitialize();
        return -1;
    }

    size_t entryCount = Dmod_Resource_GetEntryCount( ctx );
    printf("Found %zu resource entr%s in: %s\n", entryCount,
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

        printf("  [%s] %s\n", entry.key, destPath);

        bool entryOk = true;
        for( size_t j = 0; j < entry.origin_count; j++ )
        {
            printf("    <- %s\n", entry.origins[j]);
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

    // Print summary
    printf("\nSummary:\n");
    printf("  Entries copied:  %d\n", copiedEntries);
    printf("  Entries skipped: %d (no [origin] directive)\n", skippedEntries);
    if( errorCount > 0 )
    {
        printf("  Errors:          %d\n", errorCount);
    }
    printf("  Output directory: %s\n", outputDir);

    if( errorCount > 0 )
    {
        printf("\nWarning: Package creation finished with errors.\n");
        return -1;
    }

    printf("\nSuccess! Package directory created successfully.\n");
    return 0;
}
