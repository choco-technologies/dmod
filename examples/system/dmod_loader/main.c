#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "dmod.h"

// -----------------------------------------
//
//      Stack analysis constants
//
// -----------------------------------------
#define STACK_SENTINEL_BYTE              ((uint8_t)0xAA)
#define STACK_DEFAULT_SIZE               (1024 * 1024)    // 1 MB default
#define STACK_USAGE_WARNING_THRESHOLD    90               // Warn if usage exceeds 90%

// -----------------------------------------
//
//      Stack analysis thread argument
//
// -----------------------------------------
typedef struct
{
    Dmod_Context_t* context;
    int             appArgc;
    char**          appArgv;
    int             result;
} StackAnalysisArgs_t;

// -----------------------------------------
//
//      Thread function for stack analysis
//
// -----------------------------------------
static void* StackAnalysisThread( void* arg )
{
    StackAnalysisArgs_t* args = (StackAnalysisArgs_t*)arg;
    Dmod_ModuleType_t moduleType = Dmod_GetModuleType( args->context );

    if( moduleType == Dmod_ModuleType_Application )
    {
        args->result = Dmod_Run( args->context, args->appArgc, args->appArgv );
    }
    else if( moduleType == Dmod_ModuleType_Library )
    {
        if( !Dmod_Enable( args->context, false, NULL ) )
        {
            args->result = -1;
        }
        else
        {
            Dmod_Disable( args->context, false );
            args->result = 0;
        }
    }
    else
    {
        printf( "Unknown module type: %d\n", (int)moduleType );
        args->result = -1;
    }
    return NULL;
}

// -----------------------------------------
//
//      No-op thread used to measure pthread baseline overhead
//
// -----------------------------------------
static void* StackOverheadThread( void* arg )
{
    (void)arg;
    return NULL;
}

// -----------------------------------------
//
//      Parse a size string (optional k/M/G suffix)
//
// -----------------------------------------
static size_t ParseStackSize( const char* str )
{
    char* end;
    unsigned long long value = strtoull( str, &end, 0 );
    if( *end == 'k' || *end == 'K' ) value *= 1024ULL;
    else if( *end == 'm' || *end == 'M' ) value *= 1024ULL * 1024ULL;
    else if( *end == 'g' || *end == 'G' ) value *= 1024ULL * 1024ULL * 1024ULL;
    return (size_t)value;
}

// -----------------------------------------
//
//      Measure peak stack usage via sentinel scan
//      (stack grows downward, scan from base upward)
//
// -----------------------------------------
static size_t MeasureStackUsage( const uint8_t* stackBase, size_t stackSize )
{
    for( size_t i = 0; i < stackSize; i++ )
    {
        if( stackBase[i] != STACK_SENTINEL_BYTE )
        {
            return stackSize - i;
        }
    }
    return 0;
}

// -----------------------------------------
//
//      Measure the stack consumed by pthread itself (TLS, TCB,
//      startup frames) by running an empty thread on a
//      sentinel-painted buffer of the same size.
//
// -----------------------------------------
static size_t MeasureThreadBaselineOverhead( size_t stackSize )
{
    uint8_t* stackBuffer = NULL;
    if( posix_memalign( (void**)&stackBuffer, DMOD_STACK_ALIGNMENT, stackSize ) != 0 )
        return 0;

    memset( stackBuffer, STACK_SENTINEL_BYTE, stackSize );

    pthread_attr_t attr;
    pthread_attr_init( &attr );
    pthread_attr_setstack( &attr, stackBuffer, stackSize );

    pthread_t thread;
    int rc = pthread_create( &thread, &attr, StackOverheadThread, NULL );
    pthread_attr_destroy( &attr );
    if( rc != 0 )
    {
        free( stackBuffer );
        return 0;
    }

    pthread_join( thread, NULL );

    size_t overhead = MeasureStackUsage( stackBuffer, stackSize );
    free( stackBuffer );
    return overhead;
}

// -----------------------------------------
//
//      Run module in a custom-painted stack thread
//      and report peak stack usage
//
// -----------------------------------------
static int RunWithStackAnalysis( Dmod_Context_t* context, int appArgc, char** appArgv,
                                 size_t stackSize, unsigned int timeoutSeconds )
{
    printf( "\n" );
    printf( "================================================================================\n" );
    printf( "                         DMOD STACK ANALYSIS MODE                               \n" );
    printf( "================================================================================\n" );
    printf( "Stack size:    %zu bytes (%.2f KB)\n", stackSize, (double)stackSize / 1024.0 );
    if( timeoutSeconds > 0 )
    {
        printf( "Timeout:       %u seconds\n", timeoutSeconds );
    }
    else
    {
        printf( "Timeout:       none\n" );
    }
    printf( "\n" );

    // Align stack size up to DMOD_STACK_ALIGNMENT (16 bytes): round up to nearest multiple
    stackSize = (stackSize + (DMOD_STACK_ALIGNMENT - 1)) & ~(size_t)(DMOD_STACK_ALIGNMENT - 1);

    // Ensure minimum stack size required by pthread
    if( stackSize < (size_t)PTHREAD_STACK_MIN )
    {
        printf( "Warning: Stack size increased to PTHREAD_STACK_MIN (%ld bytes)\n",
                (long)PTHREAD_STACK_MIN );
        stackSize = (size_t)PTHREAD_STACK_MIN;
    }

    // Allocate aligned stack buffer
    uint8_t* stackBuffer = NULL;
    if( posix_memalign( (void**)&stackBuffer, DMOD_STACK_ALIGNMENT, stackSize ) != 0 )
    {
        printf( "Error: Failed to allocate stack buffer (%zu bytes)\n", stackSize );
        return -1;
    }

    // Paint the entire stack with the sentinel pattern
    memset( stackBuffer, STACK_SENTINEL_BYTE, stackSize );

    // Configure thread to use our custom stack
    pthread_attr_t attr;
    pthread_attr_init( &attr );
    pthread_attr_setstack( &attr, stackBuffer, stackSize );

    // Set up thread argument structure
    StackAnalysisArgs_t args;
    args.context = context;
    args.appArgc = appArgc;
    args.appArgv = appArgv;
    args.result  = 0;

    printf( "Running module in stack analysis thread...\n" );

    pthread_t thread;
    int rc = pthread_create( &thread, &attr, StackAnalysisThread, &args );
    pthread_attr_destroy( &attr );

    if( rc != 0 )
    {
        printf( "Error: Failed to create thread: %s\n", strerror( rc ) );
        free( stackBuffer );
        return -1;
    }

    bool threadTimedOut = false;
    if( timeoutSeconds > 0 )
    {
        struct timespec ts;
        clock_gettime( CLOCK_REALTIME, &ts );
        ts.tv_sec += (time_t)timeoutSeconds;
        rc = pthread_timedjoin_np( thread, NULL, &ts );
        if( rc == ETIMEDOUT )
        {
            printf( "\nTimeout exceeded (%u seconds). Cancelling thread...\n", timeoutSeconds );
            pthread_cancel( thread );
            pthread_join( thread, NULL );
            threadTimedOut = true;
        }
    }
    else
    {
        pthread_join( thread, NULL );
    }

    // Scan for peak stack usage
    size_t rawUsedStack = MeasureStackUsage( stackBuffer, stackSize );

    // Subtract the pthread/TLS baseline overhead so that only the
    // application's own stack consumption is reported (relevant for
    // embedded targets where pthread overhead does not exist).
    size_t pthreadOverhead = MeasureThreadBaselineOverhead( stackSize );
    size_t usedStack = ( rawUsedStack > pthreadOverhead ) ? ( rawUsedStack - pthreadOverhead ) : 0;

    printf( "\n" );
    printf( "================================================================================\n" );
    printf( "                         STACK ANALYSIS RESULTS                                 \n" );
    printf( "================================================================================\n" );
    printf( "\n" );
    printf( "Module:          %s\n", Dmod_GetName( context ) );
    if( threadTimedOut )
    {
        printf( "Status:          TIMED OUT after %u seconds\n", timeoutSeconds );
    }
    else
    {
        printf( "Status:          Completed (exit code: %d)\n", args.result );
    }
    printf( "\n" );
    printf( "Stack allocated: %zu bytes (%.2f KB)\n", stackSize, (double)stackSize / 1024.0 );
    printf( "Stack used:      %zu bytes (%.2f KB)\n", usedStack, (double)usedStack / 1024.0 );
    printf( "Stack free:      %zu bytes (%.2f KB)\n",
            stackSize - usedStack, (double)(stackSize - usedStack) / 1024.0 );
    printf( "Stack usage:     %.1f%%\n", (double)usedStack * 100.0 / (double)stackSize );
    printf( "System overhead: %zu bytes (%.2f KB)\n", pthreadOverhead, (double)pthreadOverhead / 1024.0 );

    if( rawUsedStack >= stackSize * STACK_USAGE_WARNING_THRESHOLD / 100 )
    {
        printf( "\n" );
        printf( "WARNING: Stack usage exceeds 90%%! Stack may have overflowed.\n" );
        printf( "         Consider re-running with a larger --stack size.\n" );
    }

    uint64_t declaredStackSize = Dmod_GetStackSize( context );
    if( declaredStackSize > 0 )
    {
        printf( "\n" );
        printf( "Declared stack requirement: %llu bytes (%.2f KB)\n",
                (unsigned long long)declaredStackSize,
                (double)declaredStackSize / 1024.0 );
        if( usedStack > (size_t)declaredStackSize )
        {
            printf( "WARNING: Measured peak usage exceeds the declared stack requirement!\n" );
        }
    }

    printf( "\n" );
    printf( "================================================================================\n" );

    free( stackBuffer );
    return args.result;
}

// -----------------------------------------
//
//      Generate debug helper scripts
//
// -----------------------------------------
void GenerateDebugScripts( Dmod_Context_t* context, const char* elfPath )
{
    // Calculate the text section address for GDB symbol loading
    void* textAddress = (void*)((uintptr_t)context->Data + context->Footer->Text.SectionStart);
    pid_t pid = getpid();
    
    // Generate GDB script: dmod_gdb_script.gdb
    FILE* gdbScript = fopen("dmod_gdb_script.gdb", "w");
    if( gdbScript != NULL )
    {
        fprintf(gdbScript, "# DMOD GDB Debug Script\n");
        fprintf(gdbScript, "# Generated automatically by dmod_loader --debug\n");
        fprintf(gdbScript, "# Module: %s\n\n", Dmod_GetName( context ));
        fprintf(gdbScript, "# Load symbols at the correct text section address\n");
        fprintf(gdbScript, "add-symbol-file %s %p\n\n", elfPath, textAddress);
        fprintf(gdbScript, "# Common breakpoints (uncomment as needed)\n");
        fprintf(gdbScript, "# break main\n");
        fprintf(gdbScript, "# break dmod_init\n\n");
        fprintf(gdbScript, "# Continue execution after attaching\n");
        fprintf(gdbScript, "# continue\n");
        fclose(gdbScript);
        printf("  Generated: dmod_gdb_script.gdb\n");
    }
    else
    {
        printf("  Warning: Could not create dmod_gdb_script.gdb\n");
    }
    
    // Generate bash script: dmod_debug.sh
    FILE* bashScript = fopen("dmod_debug.sh", "w");
    if( bashScript != NULL )
    {
        fprintf(bashScript, "#!/bin/bash\n");
        fprintf(bashScript, "# DMOD Debug Helper Script\n");
        fprintf(bashScript, "# Generated automatically by dmod_loader --debug\n");
        fprintf(bashScript, "# Module: %s\n\n", Dmod_GetName( context ));
        fprintf(bashScript, "# This script attaches GDB to the running dmod_loader process\n");
        fprintf(bashScript, "# and loads the debug symbols at the correct address.\n\n");
        fprintf(bashScript, "PID=%d\n", pid);
        fprintf(bashScript, "GDB_SCRIPT=\"dmod_gdb_script.gdb\"\n\n");
        fprintf(bashScript, "echo \"Attaching GDB to process $PID...\"\n");
        fprintf(bashScript, "echo \"NOTE: You may need to run this with sudo\"\n");
        fprintf(bashScript, "echo \"\"\n\n");
        fprintf(bashScript, "sudo gdb -p $PID -x $GDB_SCRIPT\n");
        fclose(bashScript);
        // Make the script executable
        chmod("dmod_debug.sh", 0755);
        printf("  Generated: dmod_debug.sh\n");
    }
    else
    {
        printf("  Warning: Could not create dmod_debug.sh\n");
    }
}

// -----------------------------------------
//
//      Print debug info and wait for debugger
//
// -----------------------------------------
void WaitForDebugger( Dmod_Context_t* context, const char* elfPath )
{
    // Calculate the text section address for GDB symbol loading
    // GDB needs the .text section address, not the base data address
    void* textAddress = (void*)((uintptr_t)context->Data + context->Footer->Text.SectionStart);
    
    printf("\n");
    printf("================================================================================\n");
    printf("                         DMOD DEBUG MODE                                        \n");
    printf("================================================================================\n");
    printf("\n");
    printf("Module loaded successfully. Debug information:\n");
    printf("  Module name:    %s\n", Dmod_GetName( context ));
    printf("  Base address:   %p\n", context->Data);
    printf("  Text section:   %p (offset: 0x%x)\n", textAddress, context->Footer->Text.SectionStart);
    printf("  Module size:    %zu bytes\n", context->Size);
    printf("\n");
    
    if( elfPath != NULL )
    {
        printf("Debug scripts generated in current directory:\n");
        GenerateDebugScripts( context, elfPath );
        printf("\n");
        printf("To debug, simply run in another terminal:\n");
        printf("  ./dmod_debug.sh\n");
        printf("\n");
        printf("Or manually:\n");
        printf("  sudo gdb -p %d -x dmod_gdb_script.gdb\n", getpid());
    }
    else
    {
        printf("To debug this module with GDB:\n");
        printf("\n");
        printf("  1. In another terminal, attach GDB to this process:\n");
        printf("     sudo gdb -p %d\n", getpid());
        printf("\n");
        printf("     NOTE: If you get 'ptrace: Operation not permitted', run:\n");
        printf("     echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope\n");
        printf("\n");
        printf("  2. In GDB, load symbols from the module's ELF file:\n");
        printf("     add-symbol-file <path/to/module_elf> %p\n", textAddress);
        printf("\n");
        printf("  3. Set breakpoints and continue:\n");
        printf("     break main\n");
        printf("     continue\n");
        printf("\n");
        printf("TIP: Use --debug-symbols <elf_path> to auto-generate debug scripts!\n");
    }
    
    printf("\n");
    printf("================================================================================\n");
    printf("Press ENTER to continue execution...\n");
    printf("================================================================================\n");
    
    // Wait for user input
    getchar();
}

// -----------------------------------------
//
//      Returns module type as a string
//
// -----------------------------------------
static const char* ModuleTypeToString( uint8_t moduleType )
{
    switch( (Dmod_ModuleType_t)moduleType )
    {
        case Dmod_ModuleType_Library:     return "Library";
        case Dmod_ModuleType_Application: return "Application";
        default:                          return "Unknown";
    }
}

// -----------------------------------------
//
//      Prints DMF module header info
//
// -----------------------------------------
static void PrintDmfInfo( const char* filePath )
{
    Dmod_ModuleHeader_t header;
    if( !Dmod_ReadModuleHeader( filePath, &header ) )
    {
        printf("Error: Cannot read DMF header from '%s'\n", filePath);
        return;
    }

    printf("DMF Module Information:\n");
    printf("  Name:              %s\n",     header.Name);
    printf("  Version:           %s\n",     header.Version);
    printf("  Author:            %s\n",     header.Author);
    printf("  Architecture:      %s\n",     header.Arch);
    printf("  CPU:               %s\n",     header.CpuName);
    printf("  Module Type:       %s\n",     ModuleTypeToString( header.ModuleType ));
    printf("  DMOD Version:      0x%08X\n", header.DmodVersion);
    printf("  Pointer Size:      %u bits\n", header.PointerSize * 8);
    printf("  Header Size:       %u bytes\n", header.HeaderSize);
    printf("  Stack Size:        %llu bytes\n", (unsigned long long)header.RequiredStackSize);
    printf("  Priority:          %u\n",     header.Priority);
    printf("  Manual Load:       %s\n",     header.ManualLoad ? "yes" : "no");
}

// -----------------------------------------
//
//      Prints DMFC header info
//
// -----------------------------------------
static void PrintDmfcInfo( const char* filePath )
{
    void* file = Dmod_FileOpen( filePath, "rb" );
    if( file == NULL )
    {
        printf("Error: Cannot open file '%s'\n", filePath);
        return;
    }

    Dmod_DmfcHeader_t dmfcHeader;
    if( Dmod_FileRead( &dmfcHeader, sizeof(dmfcHeader), 1, file ) != 1 )
    {
        printf("Error: Cannot read DMFC header from '%s'\n", filePath);
        Dmod_FileClose( file );
        return;
    }
    size_t fileSize = Dmod_FileSize( file );
    Dmod_FileClose( file );

    printf("DMFC Compressed Module Information:\n");
    printf("  Name:              %s\n",     dmfcHeader.Name);
    printf("  Compression:       %s\n",     dmfcHeader.Compression);
    printf("  Original Size:     %u bytes\n", dmfcHeader.OriginalSize);
    printf("  Compressed Size:   %zu bytes\n", fileSize);
    printf("  Header Version:    0x%04X\n", dmfcHeader.HeaderVersion);
    printf("  Header Size:       %u bytes\n", dmfcHeader.HeaderSize);
    printf("\n");

    printf("Inner DMF Module Information:\n");
    // Dmod_ReadModuleHeader handles DMFC decompression internally
    PrintDmfInfo( filePath );
}

// -----------------------------------------
//
//      Prints DMP package header info
//
// -----------------------------------------
static void PrintDmpInfo( const char* filePath )
{
    void* file = Dmod_FileOpen( filePath, "rb" );
    if( file == NULL )
    {
        printf("Error: Cannot open file '%s'\n", filePath);
        return;
    }

    Dmod_DmpHeader_t dmpHeader;
    if( Dmod_FileRead( &dmpHeader, sizeof(dmpHeader), 1, file ) != 1 )
    {
        printf("Error: Cannot read DMP header from '%s'\n", filePath);
        Dmod_FileClose( file );
        return;
    }

    printf("DMP Package Information:\n");
    printf("  Name:              %s\n",     dmpHeader.Name);
    printf("  Module Count:      %u\n",     dmpHeader.ModuleCount);
    printf("  Main Module Index: %u\n",     dmpHeader.MainIndex);
    printf("  Header Version:    0x%04X\n", dmpHeader.HeaderVersion);
    printf("  Header Size:       %u bytes\n", dmpHeader.HeaderSize);

    if( dmpHeader.ModuleCount == 0 )
    {
        printf("\nNo modules in package.\n");
        Dmod_FileClose( file );
        return;
    }

    Dmod_DmpModuleEntry_t* entries = (Dmod_DmpModuleEntry_t*)Dmod_Malloc(
        dmpHeader.ModuleCount * sizeof(Dmod_DmpModuleEntry_t) );
    if( entries == NULL )
    {
        printf("Error: Cannot allocate memory for module entries\n");
        Dmod_FileClose( file );
        return;
    }

    if( Dmod_FileRead( entries, sizeof(Dmod_DmpModuleEntry_t), dmpHeader.ModuleCount, file )
        != dmpHeader.ModuleCount )
    {
        printf("Error: Cannot read module entries from '%s'\n", filePath);
        Dmod_Free( entries );
        Dmod_FileClose( file );
        return;
    }
    Dmod_FileClose( file );

    printf("\nModules:\n");
    for( uint32_t i = 0; i < dmpHeader.ModuleCount; i++ )
    {
        printf("  [%u] %s\n", i, entries[i].ModuleName);
        printf("      Offset: %u bytes\n",   entries[i].ModuleOffset);
        printf("      Size:   %u bytes\n",   entries[i].FileSize);
        if( i == dmpHeader.MainIndex )
        {
            printf("      [MAIN MODULE]\n");
        }
    }

    Dmod_Free( entries );
}

// -----------------------------------------
//
//      Prints file header information
//
// -----------------------------------------
static void PrintFileInfo( const char* filePath )
{
    printf("File: %s\n\n", filePath);

    if( Dmod_IsDMPFile( filePath ) )
    {
        PrintDmpInfo( filePath );
    }
    else if( Dmod_IsDMFCFile( filePath ) )
    {
        PrintDmfcInfo( filePath );
    }
    else
    {
        PrintDmfInfo( filePath );
    }
}

// -----------------------------------------
//
//      Check if string is a file path or module name
//
// -----------------------------------------
bool IsFilePath( const char* str )
{
    if( str == NULL )
    {
        return false;
    }
    
    // First, check if file exists - if yes, it's a file path
    if( Dmod_FileAvailable( str ) )
    {
        return true;
    }
    
    // If file doesn't exist, check if the name contains only valid module name characters
    // Module names can only contain: a-z, A-Z, 0-9, and underscore
    for( const char* p = str; *p != '\0'; p++ )
    {
        char c = *p;
        if( !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') )
        {
            // Contains invalid character for module name, treat as file path
            return true;
        }
    }
    
    // Valid module name - not a file path
    return false;
}

// -----------------------------------------
//
//      Prints usage message
//
// -----------------------------------------
void PrintUsage( const char* AppName )
{
    printf("Usage: %s <path/to/file.dmf | module_name> [--module <module_name>] [--args <arguments>] [--debug [elf_path]] [--info] [--stack [size]] [--stack-timeout <seconds>]\n", AppName);
}

// -----------------------------------------
//
//      Prints help message
//
// -----------------------------------------
void PrintHelp( const char* AppName )
{
    printf("-- Dynamic Module Loader ver. " DMOD_VERSION_STRING " --\n\n");
    printf("The DMOD is a dynamic module loader that allows to load and unload modules\n");
    printf("This is an example application that uses the DMOD system\n\n");
    printf("Usage: %s <path/to/file.dmf | module_name> [--module <module_name>] [--args <arguments>] [--debug [elf_path]] [--info] [--stack [size]] [--stack-timeout <seconds>]\n", AppName);
    printf("Options:\n");
    printf("  -h, --help                Print this help message\n");
    printf("  -v, --version             Print version information\n");
    printf("  --info                    Print header information from a dmf/dmfc/dmp file without running it\n");
    printf("  --module <module_name>    Specify which module to load from a DMP package\n");
    printf("  --args <arguments>        Arguments to pass to the application module\n");
    printf("  --debug [elf_path]        Debug mode: pause after load, show addresses\n");
    printf("                            If elf_path is provided, generates ready-to-use debug scripts\n");
    printf("  --stack [size]            Stack analysis mode: allocate a large painted stack, run the\n");
    printf("                            module in a new thread and report peak stack usage.\n");
    printf("                            Optional size can use k/M/G suffix (e.g. 512k, 2M).\n");
    printf("                            Default size: 1 MB.\n");
    printf("  --stack-timeout <secs>    Timeout in seconds for --stack mode. The thread is cancelled\n");
    printf("                            after the timeout and stack usage is still reported.\n\n");
    printf("Module Types:\n");
    printf("  Application    Runs the module's main function\n");
    printf("  Library        Enables the module, then disables it\n\n");
    printf("Loading Modes:\n");
    printf("  File Path      If file exists or contains invalid module name characters\n");
    printf("  Module Name    Valid name (a-Z, 0-9, _) that doesn't exist as file\n\n");
    printf("Examples:\n");
    printf("  %s my-app.dmf                                  # Load from file\n", AppName);
    printf("  %s difs                                        # Load module by name\n", AppName);
    printf("  %s my-package.dmp --module my_module          # Load specific module from package\n", AppName);
    printf("  %s my-app.dmf --args \"arg1 arg2\"              # Load file with arguments\n", AppName);
    printf("  %s my_module --args \"--verbose\"               # Load module by name with arguments\n", AppName);
    printf("  %s my-app.dmf --debug                         # Debug mode: shows base address\n", AppName);
    printf("  %s my-app.dmf --debug ./my-app                # Debug mode + generate scripts\n", AppName);
    printf("  %s my-app.dmf --info                          # Print header info without running\n", AppName);
    printf("  %s my-package.dmp --info                      # Print DMP package info\n", AppName);
    printf("  %s my-app.dmf --stack                         # Stack analysis with default 1 MB stack\n", AppName);
    printf("  %s my-app.dmf --stack 2M                      # Stack analysis with 2 MB stack\n", AppName);
    printf("  %s my-app.dmf --stack 512k --stack-timeout 5  # Stack analysis, 512 KB, 5 s timeout\n", AppName);
}

// -----------------------------------------
//
//      Main function
//
// -----------------------------------------
int main( int argc, char *argv[] )
{
    // Initialize Dmod system
    if (!Dmod_Initialize(0, 0))
    {
        printf("Error: Failed to initialize Dmod system\n");
        return -1;
    }

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

    // Parse arguments
    const char* pathOrName = argv[1];
    const char* moduleName = NULL;
    const char* debugElfPath = NULL;
    int appArgc = 0;
    char** appArgv = NULL;
    bool debugMode = false;
    bool infoMode = false;
    bool stackMode = false;
    size_t stackSize = STACK_DEFAULT_SIZE;
    unsigned int stackTimeout = 0;

    // Look for --module, --args, --debug, --info, --stack and --stack-timeout flags
    int moduleIndex = -1;
    int argsIndex = -1;
    int debugIndex = -1;
    int stackIndex = -1;
    int stackTimeoutIndex = -1;
    for( int i = 2; i < argc; i++ )
    {
        if( strcmp( argv[i], "--module" ) == 0 )
        {
            moduleIndex = i;
        }
        else if( strcmp( argv[i], "--args" ) == 0 )
        {
            argsIndex = i;
            // Don't break here to allow detecting all flags
        }
        else if( strcmp( argv[i], "--debug" ) == 0 )
        {
            debugMode = true;
            debugIndex = i;
        }
        else if( strcmp( argv[i], "--info" ) == 0 )
        {
            infoMode = true;
        }
        else if( strcmp( argv[i], "--stack" ) == 0 )
        {
            stackMode = true;
            stackIndex = i;
        }
        else if( strcmp( argv[i], "--stack-timeout" ) == 0 )
        {
            stackTimeoutIndex = i;
        }
    }

    // Handle --info: print header info and exit (no module loading)
    if( infoMode )
    {
        PrintFileInfo( pathOrName );
        return 0;
    }

    // Get optional stack size if provided after --stack (e.g. --stack 2M)
    if( stackIndex != -1 && stackIndex + 1 < argc )
    {
        const char* nextArg = argv[stackIndex + 1];
        if( nextArg[0] != '-' )
        {
            size_t parsed = ParseStackSize( nextArg );
            if( parsed == 0 )
            {
                printf("Error: Invalid stack size '%s'\n", nextArg);
                PrintUsage( argv[0] );
                return -1;
            }
            stackSize = parsed;
        }
    }

    // Get timeout if --stack-timeout was provided
    if( stackTimeoutIndex != -1 )
    {
        if( stackTimeoutIndex + 1 >= argc )
        {
            printf("Error: --stack-timeout requires a value in seconds\n");
            PrintUsage( argv[0] );
            return -1;
        }
        stackTimeout = (unsigned int)strtoul( argv[stackTimeoutIndex + 1], NULL, 10 );
    }

    // Get ELF path if provided after --debug (optional)
    if( debugIndex != -1 && argc > debugIndex + 1 )
    {
        // Check if next argument is not another flag
        const char* nextArg = argv[debugIndex + 1];
        if( nextArg[0] != '-' )
        {
            debugElfPath = nextArg;
        }
    }

    // Get module name if --module flag was provided
    if( moduleIndex != -1 )
    {
        if( argc <= moduleIndex + 1 )
        {
            printf("Error: --module flag requires a module name\n");
            PrintUsage( argv[0] );
            return -1;
        }
        moduleName = argv[moduleIndex + 1];
    }

    // Prepare arguments to pass to the module
    // We need to construct argv array with pathOrName as argv[0]
    // followed by any arguments after --args
    int extraArgc = 0;
    if( argsIndex != -1 && argc > argsIndex + 1 )
    {
        extraArgc = argc - argsIndex - 1;
    }
    
    // Allocate appArgv: 1 for pathOrName + extraArgc arguments + 1 for NULL terminator
    appArgc = 1 + extraArgc;
    appArgv = (char**)Dmod_Malloc( (appArgc + 1) * sizeof(char*) );
    if( appArgv == NULL )
    {
        printf("Error: Failed to allocate memory for arguments\n");
        return -1;
    }
    
    // Set argv[0] to the module path/name
    appArgv[0] = (char*)pathOrName;
    
    // Copy remaining arguments after --args
    for( int i = 0; i < extraArgc; i++ )
    {
        appArgv[1 + i] = argv[argsIndex + 1 + i];
    }
    
    // NULL terminate the argv array
    appArgv[appArgc] = NULL;

    // Load the module or package
    Dmod_Context_t* context = NULL;
    
    // Determine if pathOrName is a file path or module name
    bool isPath = IsFilePath( pathOrName );
    char filePath[DMOD_MAX_PATH_LENGTH];

    if( !isPath )
    {
        // It's a module name, use Dmod_LoadModuleByName
        printf("Loading module by name: %s\n", pathOrName);
        if( !Dmod_FindModuleFile( pathOrName, DMOD_ARCH, filePath, sizeof(filePath) ) )
        {
            printf("Cannot find module file for module name: %s\n", pathOrName);
            Dmod_Free( appArgv );
            return -1;
        }
        context = Dmod_LoadFile( filePath );
    }
    // Check if the file is a DMP package
    else if( Dmod_IsDMPFile( pathOrName ) )
    {
        // If it's a package and no module name specified, load the main module
        if( moduleName == NULL )
        {
            printf("Loading DMP package: %s (main module)\n", pathOrName);
            context = Dmod_LoadFile( pathOrName );
        }
        else
        {
            // Load the specified module from the package
            printf("Loading module '%s' from DMP package: %s\n", moduleName, pathOrName);
            
            // First, add the package to the system
            uint32_t packageIndex = UINT32_MAX;
            if( !Dmod_AddPackageFile( pathOrName, &packageIndex ) )
            {
                printf("Cannot add DMP package: %s\n", pathOrName);
                Dmod_Free( appArgv );
                return -1;
            }
            
            // Get package name from the added package
            char packageName[DMOD_MAX_PACKAGE_NAME_LENGTH] = {0};
            size_t packageSize = 0;
            if( !Dmod_GetPackageInfo( packageIndex, packageName, sizeof(packageName), &packageSize ) )
            {
                printf("Cannot get package info\n");
                Dmod_Free( appArgv );
                return -1;
            }
            
            // Load the specific module from the package
            context = Dmod_LoadFromPackage( packageName, moduleName );
        }
    }
    else
    {
        // For regular DMF files, ignore --module parameter
        if( moduleName != NULL )
        {
            printf("Warning: --module parameter is only valid for DMP packages, ignoring\n");
        }
        context = Dmod_LoadFile( pathOrName );
    }
    
    // Handle module loaded from file/package
    if( context == NULL )
    {
        printf("Cannot load module: %s\n", pathOrName);
        Dmod_Free( appArgv );
        return -1;
    }

    // If debug mode is enabled, print debug info and wait for debugger
    if( debugMode )
    {
        WaitForDebugger( context, debugElfPath );
    }

    const Dmod_RequiredModule_t* reqModule = Dmod_GetNextRequiredModule( context, NULL );
    while( reqModule != NULL)
    {
        DMOD_LOG_INFO("Module '%s' requires module '%s' version '%s'\n", Dmod_GetName(context), reqModule->Name, reqModule->Version );
        reqModule = Dmod_GetNextRequiredModule( context, reqModule );
    }

    // If stack analysis mode is enabled, run in a painted-stack thread and report usage
    if( stackMode )
    {
        int result = RunWithStackAnalysis( context, appArgc, appArgv, stackSize, stackTimeout );
        Dmod_Unload( context, false );
        Dmod_Free( appArgv );
        return result;
    }

    // Check module type and handle accordingly
    Dmod_ModuleType_t moduleType = Dmod_GetModuleType( context );
    
    if( moduleType == Dmod_ModuleType_Library )
    {
        // For library modules: enable, then disable
        printf("Module is a library, enabling...\n");
        if( !Dmod_Enable( context, false, NULL ) )
        {
            printf("Cannot enable library module: %s\n", pathOrName);
            Dmod_Unload( context, false );
            Dmod_Free( appArgv );
            return -1;
        }
        printf("Library module enabled successfully\n");
        
        printf("Disabling library module...\n");
        if( !Dmod_Disable( context, false ) )
        {
            printf("Cannot disable library module: %s\n", pathOrName);
            Dmod_Unload( context, false );
            Dmod_Free( appArgv );
            return -1;
        }
        printf("Library module disabled successfully\n");
        Dmod_Unload( context, false );
        Dmod_Free( appArgv );
        return 0;
    }
    else if( moduleType == Dmod_ModuleType_Application )
    {
        // For application modules: run with parsed arguments
        int result = Dmod_Run( context, appArgc, appArgv );
        Dmod_Unload( context, false );
        Dmod_Free( appArgv );
        return result;
    }
    else
    {
        printf("Unknown module type: %d\n", moduleType);
        Dmod_Unload( context, false );
        Dmod_Free( appArgv );
        return -1;
    }
}

