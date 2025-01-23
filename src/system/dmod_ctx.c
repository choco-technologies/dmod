#define DMOD_PRIVATE
#include "dmod.h"
#include "private/dmod_ctx.h"
#include "private/dmod_vars.h"
#include <string.h>

/**
 * @brief Creates new context
 * 
 * @param FileSize      Size of the file
 * 
 * @return Pointer to new context
 * 
 * @note This function creates a new context and initializes it
 */
Dmod_Context_t* Dmod_Context_New( void* Data, size_t FileSize )
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
    Context->Data       = Data != NULL ? Data : Dmod_AlignedMalloc( FileSize, DMOD_STACK_ALIGNMENT );
    Context->Size       = FileSize;
    Context->Mutex      = Dmod_Mutex_New(true);
    Context->Enabled    = false;
    Context->Running    = false;
    Context->UsageCounter = 0;

    if( Context->Data == NULL )
    {
        DMOD_LOG_ERROR("Cannot create new context - cannot allocate memory for file data. The required size: %d\n", FileSize);
        Dmod_Context_Delete( Context );
        return NULL;
    }

    if( Context->Mutex == NULL )
    {
        DMOD_LOG_WARN("Could not create mutex\n");
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
bool Dmod_Context_IsValid( Dmod_Context_t* Context )
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
void Dmod_Context_Delete( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return;
    }

    Dmod_Mutex_Delete( Context->Mutex );
    if( Context->Data != NULL )
    {
        Dmod_Free( Context->Data );
    }
    Dmod_Free( Context );
}

/**
 * @brief Get module name
 * 
 * @param Context Context to get module name from
 * 
 * @return Module name
 */
const char* Dmod_Context_GetModuleName( Dmod_Context_t* Context )
{
    if( Context == NULL || !Dmod_Context_IsValid( Context ) )
    {
        return "Invalid";
    }

    if( Context->Header == NULL )
    {
        return "Unknown";
    }

    return Context->Header->Name;
}

/**
 * @brief Add context
 * 
 * @param Context Context to add
 * 
 * @return True if context was added successfully, false otherwise
 */
bool Dmod_Context_Add( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
    {
        if( Dmod_Contexts[i] == NULL )
        {
            Dmod_Contexts[i] = Context;
            return true;
        }
    }

    DMOD_LOG_ERROR("Cannot add context '%s' - no space left\n", Dmod_Context_GetModuleName( Context ));
    return false;
}

/**
 * @brief Remove context
 * 
 * @param Context Context to remove
 * 
 * @return True if context was removed successfully, false otherwise
 */
bool Dmod_Context_Remove( Dmod_Context_t* Context )
{
    if( Context == NULL )
    {
        return false;
    }

    for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
    {
        if( Dmod_Contexts[i] == Context )
        {
            Dmod_Contexts[i] = NULL;
            return true;
        }
    }

    DMOD_LOG_WARN("Cannot remove context '%s' - not found\n", Dmod_Context_GetModuleName( Context ));
    return true;
}

/**
 * @brief Get context
 * 
 * @param ModuleName Name of the module to find
 * 
 * @return Pointer to the context
 */
Dmod_Context_t* Dmod_Context_Get( const char* ModuleName )
{
    if( ModuleName == NULL )
    {
        return NULL;
    }

    for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
    {
        if( Dmod_Contexts[i] == NULL )
        {
            continue;
        }

        if( strcmp( Dmod_Contexts[i]->Header->Name, ModuleName ) == 0 )
        {
            return Dmod_Contexts[i];
        }
    }

    return NULL;
}
