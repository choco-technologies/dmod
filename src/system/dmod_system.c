#include "dmod.h"
#include "dmod_system.h"
#include <stdbool.h>
#include <string.h>
#include <errno.h>

//==============================================================================
//                              LOCAL FUNCTION PROTOTYPES
//==============================================================================

static Dmod_Context_t*  Context_New( size_t FileSize );
static bool             Context_IsValid( Dmod_Context_t* Context );
static void             Context_Delete( Dmod_Context_t* Context );
static bool             ReadFile( Dmod_Context_t* Context, void* File );
static bool             LoadHeader( Dmod_Context_t* Context );
static bool             LoadFooter( Dmod_Context_t* Context );
static bool             LoadOutput( Dmod_Context_t* Context );
static bool             LoadInput( Dmod_Context_t* Context );
static bool             InitPointer( Dmod_Context_t* Context, void** PointerRef, const char* PointerName );

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
Dmod_Context_t* Dmod_Load( const char* Path )
{
    if( Path == NULL )
    {
        DMOD_LOG_ERROR("Cannot load module - invalid path\n");
        return NULL;
    }

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

    Dmod_Context_t* context = Context_New( fileSize );
    if( context == NULL )
    {
        Dmod_FileClose( file );
        return NULL;
    }

    // Load header
    if( 
        !ReadFile( context, file ) 
     || !LoadHeader( context ) 
     || !LoadFooter( context )
     || !LoadOutput( context )
     || !LoadInput( context )
        )
    {
        Dmod_FileClose( file );
        Context_Delete( context );
        return NULL;
    }

    return context;
}

/**
 * @brief Unload module
 * 
 * @param Context Context to unload
 */
void Dmod_Unload( Dmod_Context_t* Context )
{
    if( Context == NULL || !Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot unload module - invalid context\n");
        return;
    }

    Context_Delete( Context );
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
    if( Context == NULL || !Context_IsValid( Context ) || !Dmod_IsApiSignatureValid( Signature ) )
    {
        DMOD_LOG_ERROR("Cannot get function - invalid context or signature\n");
        return NULL;
    }

    if( Context->Output.Section == NULL )
    {
        DMOD_LOG_ERROR("Cannot get function - no output section\n");
        return NULL;
    }

    for(size_t i = 0; i < Context->Input.NumberOfEntries; i++)
    {
        if( strcmp( Context->Input.Section->Entries[i].Signature, Signature ) == 0 )
        {
            return Context->Input.Section->Entries[i].Function;
        }
    }

    DMOD_LOG_ERROR("Cannot get function - function not found: %s\n", Signature);
    return NULL;
}

/**
 * @brief Initialize module
 * 
 * @param Context Context to initialize
 * @param Config Configuration
 * 
 * @return 0 on success, errno on error
 */
int Dmod_Init( Dmod_Context_t* Context, Dmod_Config_t* Config )
{
    int result = -EINVAL;
    if( Context_IsValid( Context ) )
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
    if( Context_IsValid( Context ) )
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
    if( Context_IsValid( Context ) )
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
 * @brief Get stack size
 * 
 * @param Context Context to get stack size from
 * 
 * @return Stack size
 */
uint64_t Dmod_GetStackSize( Dmod_Context_t* Context )
{
    if( Context == NULL || !Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot get stack size - invalid context\n");
        return 0;
    }

    return Context->Header->RequiredStackSize;
}

//==============================================================================
//                              LOCAL FUNCTIONS IMPLEMENTATIONS
//==============================================================================

/**
 * @brief Create new context
 * 
 * @param FileSize      Size of the file
 * 
 * @return Pointer to new context
 * 
 * @note This function creates a new context and initializes it
 */
static Dmod_Context_t*  Context_New( size_t FileSize )
{
    Dmod_Context_t* Context = Dmod_Malloc( sizeof( Dmod_Context_t ) );
    if( Context == NULL )
    {
        DMOD_LOG_ERROR("Cannot create new context - cannot allocate memory\n");
        return NULL;
    }

    Context->Signature  = DMOD_CONTEXT_SIGNATURE;
    Context->Header     = NULL;
    Context->Footer     = NULL;
    Context->Data       = Dmod_AlignedMalloc( FileSize, DMOD_STACK_ALIGNMENT );
    Context->Size       = FileSize;

    if( Context->Data == NULL )
    {
        DMOD_LOG_ERROR("Cannot create new context - cannot allocate memory for file data. The required size: %d\n", FileSize);
        Context_Delete( Context );
        return NULL;
    }

    return Context;
}

/**
 * @brief Check if context is valid
 * 
 * @param Context Context to check
 * 
 * @return True if context is valid, false otherwise
 */
static bool Context_IsValid( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    if( Context->Signature != DMOD_CONTEXT_SIGNATURE )
    {
        return false;
    }

    return true;
}

/**
 * @brief Delete context
 * 
 * @param Context Context to delete
 * 
 * @note This function deletes the context and frees the memory
 */
static void Context_Delete( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return;
    }

    if( Context->Data != NULL )
    {
        Dmod_Free( Context->Data );
    }
    Dmod_Free( Context );
}

/**
 * @brief Read file
 * 
 * @param Context Context to read file to
 * @param File File to read
 * 
 * @return True if file was read successfully, false otherwise
 */
static bool ReadFile( Dmod_Context_t* Context, void* File )
{
    if( Context == NULL || File == NULL )
    {
        return false;
    }

    // Seek to the beginning of the file
    if( Dmod_FileSeek( File, 0, DMOD_SEEK_SET ) != 0 )
    {
        DMOD_LOG_ERROR("Cannot read file - cannot seek to the beginning of the file\n");
        return false;
    }

    // Read file
    size_t read = Dmod_FileRead( Context->Data, 1, Context->Size, File );
    if( read != Context->Size )
    {
        DMOD_LOG_ERROR("Cannot read file - not all data read: %d\n", read);
        return false;
    }

    return true;
}

/**
 * @brief Load header
 * 
 * @param Context Context to load header to
 * 
 * @return True if header was loaded successfully, false otherwise
 */
static bool LoadHeader( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    // Check signature
    Dmod_ModuleHeader_t* header = (Dmod_ModuleHeader_t*)Context->Data;
    if( header->Signature != DMOD_HEADER_SIGNATURE )
    {
        DMOD_LOG_ERROR("Cannot load header - invalid signature\n");
        return false;
    }

    // Check version
    if( header->Version != DMOD_VERSION )
    {
        DMOD_LOG_ERROR("Cannot load header - invalid version\n");
        return false;
    }

    // Check architecture
    if( strcmp( header->Arch, DMOD_ARCH ) != 0 )
    {
        DMOD_LOG_ERROR("Cannot load header - invalid architecture: %s != %s\n", header->Arch, DMOD_ARCH);
        return false;
    }

    // Check name
    if( strlen( header->Name ) > DMOD_MAX_MODULE_NAME_LENGTH )
    {
        DMOD_LOG_ERROR("Cannot load header - The given name is too long: %s\n", header->Name);
        return false;
    }

    Context->Header = header;

    return InitPointer( Context, (void**)&header->Init,     "Init"   ) 
        && InitPointer( Context, (void**)&header->Main,     "Main"   ) 
        && InitPointer( Context, (void**)&header->Deinit,   "Deinit" );
}

/**
 * @brief Load footer
 * 
 * @param Context Context to load footer to
 * 
 * @return True if footer was loaded successfully, false otherwise
 */
static bool LoadFooter( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    Dmod_ModuleFooter_t* footer = Context->Data + Context->Size - sizeof( Dmod_ModuleFooter_t );
    
    if( footer->Header.SectionStart != 0 || footer->Header.SectionSize != sizeof( Dmod_ModuleHeader_t ) )
    {
        DMOD_LOG_ERROR("Cannot load footer - invalid header section\n");
        return false;
    }

    Context->Footer = footer;

    return true;
}

/**
 * @brief Load output
 * 
 * @param Context Context to load output to
 * 
 * @return True if output was loaded successfully, false otherwise
 */
static bool LoadOutput( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    Dmod_ModuleFooter_t* footer = Context->Footer;
    Dmod_ModuleSection_t* output = &footer->Output;

    if( output->SectionStart == 0 || output->SectionSize == 0 )
    {
        DMOD_LOG_WARN("Cannot load output - missing output section\n");
        return true;
    }

    if( output->SectionStart + output->SectionSize > Context->Size )
    {
        DMOD_LOG_ERROR("Cannot load output - output section out of bounds\n");
        return false;
    }

    Dmod_OutputSection_t* outputSection = Context->Data + output->SectionStart;
    size_t numberOfEntries = output->SectionSize / sizeof( outputSection->Entries[0] );
    if( numberOfEntries == 0 )
    {
        DMOD_LOG_ERROR("Cannot load output - Invalid output's sections size\n");
        return false;
    }

    for(size_t i = 0; i < numberOfEntries; i++)
    {
        if( !InitPointer( Context, &outputSection->Entries[i], "Output Entry" ) )
        {
            DMOD_LOG_ERROR("Cannot load output - cannot initialize output entry at index %d\n", i);
            return false;
        }
        else if( outputSection->Entries[i] == NULL )
        {
            DMOD_LOG_WARN("Empty output entry at index: %d\n", i);
        }
        else
        {
            const char* entrySignature = outputSection->Entries[i];
            if( strncmp( entrySignature, DMOD_SIGNATURE_PREFIX, sizeof( DMOD_SIGNATURE_PREFIX ) - 1 ) != 0 )
            {
                DMOD_LOG_ERROR("Cannot load output - Invalid output entry signature\n");
                return false;
            }
        }
    }

    Context->Output.Section = outputSection;
    Context->Output.NumberOfEntries = numberOfEntries;

    return true;
}

/**
 * @brief Load input
 * 
 * @param Context Context to load input to
 * 
 * @return True if input was loaded successfully, false otherwise
 */
static bool LoadInput( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    Dmod_ModuleFooter_t* footer = Context->Footer;
    Dmod_ModuleSection_t* input = &footer->Input;

    if( input->SectionStart == 0 || input->SectionSize == 0 )
    {
        DMOD_LOG_INFO("No inputs to load\n");
        if( Context->Header->Init == NULL && Context->Header->Main == NULL && Context->Header->Deinit == NULL )
        {
            DMOD_LOG_ERROR("No inputs to load and no functions to call\n");
            return false;
        }
        else 
        {
            return true;
        }
    }

    if( input->SectionStart + input->SectionSize > Context->Size )
    {
        DMOD_LOG_ERROR("Cannot load input - input section out of bounds\n");
        return false;
    }

    Dmod_InputSection_t* inputSection = Context->Data + input->SectionStart;
    size_t numberOfEntries = input->SectionSize / sizeof( inputSection->Entries[0] );
    if( numberOfEntries == 0 )
    {
        DMOD_LOG_ERROR("Cannot load input - Invalid input's sections size\n");
        return false;
    }

    for(size_t i = 0; i < numberOfEntries; i++)
    {
        if( !InitPointer( Context, &inputSection->Entries[i].Function, "Input Function" ) )
        {
            DMOD_LOG_ERROR("Cannot load input - cannot initialize input entry at index %d\n", i);
            return false;
        }
        else if( inputSection->Entries[i].Function == NULL )
        {
            DMOD_LOG_WARN("Empty input entry at index: %d\n", i);
            continue;
        }
        else 
        {
            if( !InitPointer( Context, (void**)&inputSection->Entries[i].Signature, "Input Signature" ) 
             || !InitPointer( Context, (void**)&inputSection->Entries[i].Function , "Input Function"  ) 
                )
            {
                DMOD_LOG_ERROR("Cannot load input - cannot initialize input entry at index %d\n", i);
                return false;
            }
            else if(strncmp( inputSection->Entries[i].Signature, DMOD_SIGNATURE_PREFIX, sizeof( DMOD_SIGNATURE_PREFIX ) - 1 ) != 0 )
            {
                DMOD_LOG_ERROR("Cannot load input - Invalid input entry signature: %s\n", inputSection->Entries[i].Signature);
                return false;
            }
        }
    }

    Context->Input.Section = inputSection;
    Context->Input.NumberOfEntries = numberOfEntries;

    return true;
}

/**
 * @brief Initialize pointer
 * 
 * @param Context       Context to initialize pointer in
 * @param Pointer       Pointer to initialize
 * @param PointerName   Name of the pointer
 * 
 * @return Initialized pointer
 */
static bool InitPointer( Dmod_Context_t* Context, void** PointerRef, const char* PointerName )
{
    if(PointerRef == NULL || Context == NULL)
    {
        DMOD_LOG_ERROR("Cannot initialize pointer %s - unexpected NULL\n", PointerName);
        return false;
    }
    void* pointer = *PointerRef;
    if( pointer == NULL )
    {
        return true;
    }

    size_t offset = (size_t)pointer;
    if( offset == 0 || offset > Context->Size )
    {
        DMOD_LOG_ERROR("Cannot initialize pointer %s - invalid offset: 0x%08X\n", PointerName, offset);
        return NULL;
    }

    *PointerRef = Context->Data + offset;
    return true;
}