#define DMOD_PRIVATE
#include "dmod.h"
#include "private/dmod_ctx.h"
#include "private/dmod_vars.h"
#include "private/dmod_ldr.h"
#include "private/dmod_mgr.h"
#include "private/dmod_hlp.h"
#include "private/dmod_rmod.h"

#include <string.h>

/**
 * @brief Load header
 * 
 * @param Context Context to load header to
 * 
 * @return True if header was loaded successfully, false otherwise
 */
bool Dmod_Ldr_LoadHeader( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    // Check signature
    Dmod_ModuleHeader_t* header = (Dmod_ModuleHeader_t*)Context->Data;
    if( header->Signature != DMOD_HEADER_SIGNATURE )
    {
        DMOD_LOG_ERROR("Cannot load header - invalid signature: 0x%08x\n", header->Signature);
        return false;
    }
    Dmod_Event_ModuleLoadingInProgress( header->Name, 80 );

    if( Dmod_SystemCrossplatformMode )
    {
        DMOD_LOG_WARN("Crossplatform mode enabled - skipping version and architecture checks\n");
    }

    // Check version
    if( !Dmod_SystemCrossplatformMode && !DMOD_COMPATIBLE_VERSION(header->DmodVersion) )
    {
        DMOD_LOG_ERROR("Cannot load header - incompatible version: 0x%08X != 0x%08X "DMOD_VERSION_STRING"\n", header->Version, (uint32_t)DMOD_VERSION );
        return false;
    }

    // check header size
    if( header->HeaderSize != sizeof( Dmod_ModuleHeader_t ) )
    {
        DMOD_LOG_ERROR("Cannot load header - invalid header size: %d != %d\n", header->HeaderSize, sizeof( Dmod_ModuleHeader_t ) );
        return false;
    }

    // Check architecture
    if( !Dmod_SystemCrossplatformMode && strcmp( header->Arch, DMOD_ARCH ) != 0 )
    {
        DMOD_LOG_ERROR("Cannot load header - invalid architecture: %s != %s\n", header->Arch, DMOD_ARCH);
        return false;
    }

    // Check target cpu 
    if( !Dmod_SystemCrossplatformMode && header->CpuName[0] != 0 && strcmp( header->CpuName, DMOD_CPU_NAME ) != 0 )
    {
        DMOD_LOG_ERROR("Cannot load header - invalid target cpu: %s != %s\n", header->CpuName, DMOD_CPU_NAME);
        return false;
    }

    // Check name
    if( strlen( header->Name ) > DMOD_MAX_MODULE_NAME_LENGTH )
    {
        DMOD_LOG_ERROR("Cannot load header - The given name is too long: %s\n", header->Name);
        return false;
    }

    switch( header->ModuleType )
    {
        case Dmod_ModuleType_Library:
            if( header->Init.Ptr == NULL || header->Deinit.Ptr == NULL )
            {
                DMOD_LOG_ERROR("Cannot load header - missing Init or Deinit function\n");
                return false;
            }
            if(Dmod_Mgr_IsLoaded(header->Name))
            {
                DMOD_LOG_ERROR("Cannot load header - module already loaded: %s\n", header->Name);
                return false;
            }
            break;
        case Dmod_ModuleType_Application:
            if( header->Main.Ptr == NULL )
            {
                DMOD_LOG_ERROR("Cannot load header - missing Main function\n");
                return false;
            }
            break;
        default:
            DMOD_LOG_ERROR("Cannot load header - invalid module type: %d\n", header->ModuleType);
            return false;
    }

    if(Dmod_Hlp_InitPointer(Context, (void**)&header->Footer, "Footer") == false)
    {
        DMOD_LOG_ERROR("Cannot load header of module '%s' - cannot initialize footer pointer\n", header->Name);
        return false;
    }

    if( !Dmod_Hlp_InitPointer(Context, (void**)&header->License, "License") )
    {
        DMOD_LOG_ERROR("Cannot load header of module '%s' - cannot initialize license pointer\n", header->Name);
        return false;
    }
    Dmod_License_t* license = (Dmod_License_t*)header->License.Ptr;
    if(license != NULL && !Dmod_Hlp_InitPointer(Context, (void**)&license->Text, "License Text"))
    {
        DMOD_LOG_ERROR("Cannot load header of module '%s' - cannot initialize license text pointer\n", header->Name);
        return false;
    }

    Context->Header = header;

    bool result = Dmod_Hlp_InitPointer( Context, (void**)&header->Preinit,  "Preinit"    ) 
               && Dmod_Hlp_InitPointer( Context, (void**)&header->Init,     "Init"       ) 
               && Dmod_Hlp_InitPointer( Context, (void**)&header->Main,     "Main"       ) 
               && Dmod_Hlp_InitPointer( Context, (void**)&header->Deinit,   "Deinit"     )
               && Dmod_Hlp_InitPointer( Context, (void**)&header->Signal,   "Signal"     );
               ;
    Dmod_Event_ModuleLoadingInProgress( Dmod_Context_GetModuleName(Context), 85 );
    return result;
}

/**
 * @brief Load footer
 * 
 * @param Context Context to load footer to
 * 
 * @return True if footer was loaded successfully, false otherwise
 */
bool Dmod_Ldr_LoadFooter( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    if( Context->Header == NULL )
    {
        DMOD_LOG_ERROR("Cannot load footer - missing header\n");
        return false;
    }

    Dmod_ModuleFooter_t* footer = Context->Header->Footer.Ptr;
    
    if( footer->Header.SectionStart != 0 || footer->Header.SectionSize != sizeof( Dmod_ModuleHeader_t ) )
    {
        DMOD_LOG_ERROR("Cannot load footer - invalid header section\n");
        return false;
    }

    Context->Footer = footer;
    Dmod_Event_ModuleLoadingInProgress( Dmod_Context_GetModuleName(Context), 88 );

    return true;
}

/**
 * @brief Load output
 * 
 * @param Context Context to load output to
 * 
 * @return True if output was loaded successfully, false otherwise
 */
bool Dmod_Ldr_LoadOutput( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    if( Context->Footer == NULL )
    {
        DMOD_LOG_ERROR("Cannot load output - missing footer\n");
        return false;
    }

    Dmod_ModuleFooter_t* footer = Context->Footer;
    Dmod_ModuleSection_t* output = &footer->Outputs;

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

    Dmod_OutputsSection_t* outputSection = Context->Data + output->SectionStart;
    size_t numberOfEntries = output->SectionSize / sizeof( outputSection->Entries[0] );
    if( numberOfEntries == 0 )
    {
        DMOD_LOG_ERROR("Cannot load output - Invalid output's sections size\n");
        return false;
    }

    for(size_t i = 0; i < numberOfEntries; i++)
    {
        if( !Dmod_Hlp_InitPointer( Context, &outputSection->Entries[i], "Output Entry" ) )
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
            if(!Dmod_ApiSignature_IsValid( entrySignature ))
            {
                DMOD_LOG_ERROR("Cannot load output - Invalid output entry signature\n");
                return false;
            }
        }
    }

    Context->Outputs.OutputSection      = outputSection;
    Context->Outputs.SectionSize        = output->SectionSize;
    Context->Outputs.ApiType            = Dmod_ApiType_Output;

    Dmod_Event_ModuleLoadingInProgress( Dmod_Context_GetModuleName(Context), 90 );

    return true;
}

/**
 * @brief Load input
 * 
 * @param Context Context to load input to
 * 
 * @return True if input was loaded successfully, false otherwise
 */
bool Dmod_Ldr_LoadInput( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    if( Context->Footer == NULL )
    {
        DMOD_LOG_ERROR("Cannot load input - missing footer\n");
        return false;
    }

    Dmod_ModuleFooter_t* footer = Context->Footer;
    Dmod_ModuleSection_t* input = &footer->Inputs;

    if( input->SectionStart == 0 || input->SectionSize == 0 )
    {
        DMOD_LOG_INFO("No inputs to load\n");
        if( Context->Header->Init.Ptr == NULL && Context->Header->Main.Ptr == NULL && Context->Header->Deinit.Ptr == NULL )
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

    Dmod_InputsSection_t* inputSection = Context->Data + input->SectionStart;
    size_t numberOfEntries = input->SectionSize / sizeof( inputSection->Entries[0] );
    if( numberOfEntries == 0 )
    {
        DMOD_LOG_ERROR("Cannot load input - Invalid input's sections size\n");
        return false;
    }

    for(size_t i = 0; i < numberOfEntries; i++)
    {
        if( inputSection->Entries[i].Function == NULL )
        {
            DMOD_LOG_WARN("Empty input entry at index: %d\n", i);
            continue;
        }
        else 
        {
            if( !Dmod_Hlp_InitPointer( Context, (void**)&inputSection->Entries[i].Signature, "Input Signature" ) 
             || !Dmod_Hlp_InitPointer( Context, (void**)&inputSection->Entries[i].Function , "Input Function"  ) 
                )
            {
                DMOD_LOG_ERROR("Cannot load input - cannot initialize input entry at index %d\n", i);
                return false;
            }
            else if(!Dmod_ApiSignature_IsValid(inputSection->Entries[i].Signature))
            {
                DMOD_LOG_ERROR("Cannot load input - Invalid input entry signature: %s\n", inputSection->Entries[i].Signature);
                return false;
            }
        }
    }

    Context->Inputs.InputSection    = inputSection;
    Context->Inputs.SectionSize     = input->SectionSize;
    Context->Inputs.ApiType         = Dmod_ApiType_Input;

    Dmod_Event_ModuleLoadingInProgress( Dmod_Context_GetModuleName(Context), 90 );

    return true;
}

/**
 * @brief Load got section
 * 
 * @param Context Context to load got to
 * 
 * @return True if got was loaded successfully, false otherwise
 */
bool Dmod_Ldr_LoadGot( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    if( Context->Footer == NULL )
    {
        DMOD_LOG_ERROR("Cannot load got - missing footer\n");
        return false;
    }

    Dmod_ModuleFooter_t* footer = Context->Footer;
    Dmod_ModuleSection_t* got = &footer->Got;

    if( got->SectionStart == 0 || got->SectionSize == 0 )
    {
        DMOD_LOG_INFO("No got to load\n");
        return true;
    }

    if( got->SectionStart + got->SectionSize > Context->Size )
    {
        DMOD_LOG_ERROR("Cannot load got - got section out of bounds\n");
        return false;
    }

    Dmod_GotSection_t* gotSection = Context->Data + got->SectionStart;
    size_t numberOfEntries = got->SectionSize / sizeof( gotSection->Entries[0] );

    for(size_t i = 0; i < numberOfEntries; i++)
    {
        gotSection->Entries[i] += (size_t)Context->Data;
    }

    Dmod_Event_ModuleLoadingInProgress( Dmod_Context_GetModuleName(Context), 95 );

    return true;
}

/**
 * @brief Load bss section
 * 
 * @param Context Context to load bss to
 * 
 * @return True if bss was loaded successfully, false otherwise
 */
bool Dmod_Ldr_LoadBss( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    if( Context->Footer == NULL )
    {
        DMOD_LOG_ERROR("Cannot load bss - missing footer\n");
        return false;
    }

    Dmod_ModuleFooter_t* footer = Context->Footer;
    Dmod_ModuleSection_t* bss = &footer->Bss;

    if( bss->SectionStart == 0 || bss->SectionSize == 0 )
    {
        DMOD_LOG_INFO("No bss to load\n");
        return true;
    }

    if( bss->SectionStart + bss->SectionSize > Context->Size )
    {
        DMOD_LOG_ERROR("Cannot load bss - bss section out of bounds\n");
        return false;
    }

    memset( Context->Data + bss->SectionStart, 0, bss->SectionSize );

    Dmod_Event_ModuleLoadingInProgress( Dmod_Context_GetModuleName(Context), 97 );

    return true;
}

/**
 * @brief Load module
 * 
 * @param Context Context to load
 * 
 * @return True if module was loaded successfully, false otherwise
 */
bool Dmod_Ldr_Load( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    return Dmod_Ldr_LoadHeader( Context ) 
        && Dmod_Ldr_LoadFooter( Context )
        && Dmod_Ldr_LoadOutput( Context )
        && Dmod_Ldr_LoadInput( Context )
        && Dmod_Ldr_LoadGot( Context )
        && Dmod_Ldr_LoadBss( Context )
        && Dmod_RMod_ReadRequiredModules( Context );
}