#define DMOD_PRIVATE
#include "dmod.h"
#include "private/dmod_ctx.h"
#include "private/dmod_vars.h"
#include <string.h>

/**
 * @brief Creates new context
 *
 * @param Data          Pointer to the data (if NULL, the data will be allocated)
 * @param FileSize      Size of the file
 * @param ModuleName    Name of the module being loaded, used (with a uniquing counter) to
 *                       build this context's allocator identity right away - pass NULL if
 *                       not known yet at this point (e.g. loading straight from a raw file
 *                       or buffer, before the header has been parsed).
 *
 * @return Pointer to new context
 *
 * @note This function creates a new context and initializes it
 */
Dmod_Context_t* Dmod_Context_New( void* Data, size_t FileSize, const char* ModuleName )
{
    if( FileSize == 0 )
    {
        DMOD_LOG_ERROR("Cannot create new context - invalid file size\n");
        return NULL;
    }

    /* Unique per-instance identity for heap allocation tracking (see
     * Dmod_GetCurrentAllocatorNameEx) - a monotonic counter makes it unique even when
     * the same module name repeats (e.g. a shell spawning another instance of itself),
     * without needing the context's own address (which isn't known until after this
     * struct itself is allocated). Built up front so the context struct and its data
     * buffer - both allocated right here in kernel code with no ambient module identity
     * of their own - get tagged with it too, instead of always falling back to NULL. */
    static uint64_t contextCounter = 0;
    char allocatorName[DMOD_MAX_MODULE_NAME_LENGTH + 24];
    Dmod_SnPrintf( allocatorName, sizeof(allocatorName), "%s#%llu", ModuleName != NULL ? ModuleName : "module", (unsigned long long)(contextCounter++) );

    Dmod_Context_t* Context = Dmod_MallocEx( sizeof( Dmod_Context_t ), allocatorName );
    if( Context == NULL )
    {
        DMOD_LOG_ERROR("Cannot create new context - cannot allocate memory\n");
        return NULL;
    }

    Context->Signature  = DMOD_CONTEXT_SIGNATURE;
    Context->Header     = NULL;
    Context->Footer     = NULL;
    if( Data != NULL )
    {
        /* Already allocated by the caller (e.g. a decompression buffer from
         * Dmod_FromDMFC) under whatever identity was ambient at that point, not
         * necessarily this context's. Move it into this context's bucket instead of
         * leaving it permanently mistagged - a no-op if the backend can't retag. */
        Dmod_RetagEx( Data, allocatorName );
        Context->Data = Data;
    }
    else
    {
        Context->Data = Dmod_AlignedMallocEx( FileSize, DMOD_STACK_ALIGNMENT, allocatorName );
    }
    Context->Size       = FileSize;
    Context->Mutex      = Dmod_Mutex_New(true);
    Context->Enabled    = false;
    Context->Running    = false;
    Context->UsageCounter = 0;
    Context->PackageName = NULL;
    Context->LogLevel   = Dmod_LogLevel_Count; /* inherit from global until env var is applied */
    strncpy( Context->AllocatorName, allocatorName, sizeof(Context->AllocatorName)-1 );
    Context->AllocatorName[sizeof(Context->AllocatorName)-1] = '\0';

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
    char moduleName[DMOD_MAX_MODULE_NAME_LENGTH] = {0};
    strncpy( moduleName, Dmod_Context_GetModuleName( Context ), sizeof(moduleName)-1 );

    char allocatorName[sizeof(Context->AllocatorName)] = {0};
    strncpy( allocatorName, Context->AllocatorName, sizeof(allocatorName)-1 );

    Dmod_Mutex_Delete( Context->Mutex );
    if( Context->Data != NULL )
    {
        /* Concatenate on free: Data is aligned to DMOD_STACK_ALIGNMENT (16) while the
         * heap's own base alignment can be smaller (e.g. 4), so a freed Data block
         * often doesn't have enough padding room to satisfy the next same-size,
         * differently-aligned request and is skipped by find_suitable_block() instead
         * of being reused. Without eager coalescing here, every load/unload cycle of
         * the same module (e.g. repeatedly running a command) permanently strands one
         * more orphaned block instead of returning it to the pool, and the heap's
         * largest contiguous block keeps shrinking one instance at a time until the
         * emergency fragmentation retry in _aligned_alloc() finally kicks in. */
        Dmod_FreeEx( Context->Data, true );
    }
    Context->Signature = 0;
    Dmod_FreeEx( Context, true );

    /* Bulk-free whatever this module's own code allocated. Sweep both possible keys: the
     * unique per-instance identity (used whenever a current context could be determined -
     * see Dmod_GetCurrentAllocatorNameEx) and the plain module name (used as a fallback
     * when it couldn't, e.g. no real process tracking available at all). Each call is a
     * harmless no-op if nothing was ever tagged with that particular key. */
    if( allocatorName[0] != '\0' )
    {
        Dmod_FreeModule( allocatorName );
    }
    else 
    {
        Dmod_FreeModule( moduleName );
    }
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
 * @brief Get module type
 * 
 * @param Context Context to get module type from
 * 
 * @return Module type
 */
Dmod_ModuleType_t Dmod_Context_GetModuleType( Dmod_Context_t* Context )
{
    if( Context == NULL || !Dmod_Context_IsValid( Context ) )
    {
        return Dmod_ModuleType_Unknown;
    }

    if( Context->Header == NULL )
    {
        return Dmod_ModuleType_Unknown;
    }

    return Context->Header->ModuleType;
}

/**
 * @brief Getter for the data size
 */
size_t Dmod_Context_GetSize( Dmod_Context_t* Context )
{
    if( Context == NULL || !Dmod_Context_IsValid( Context ) )
    {
        return Dmod_ModuleType_Unknown;
    }
    return Context->Size;
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
        if( Dmod_Contexts[i] == NULL || Dmod_Contexts[i]->Header == NULL )
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
