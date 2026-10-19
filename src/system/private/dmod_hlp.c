#define DMOD_PRIVATE
#include <string.h>
#include "dmod.h"
#include "private/dmod_hlp.h"

/**
 * @brief Resolve a serialized cross-platform offset without modifying it.
 *
 * Cross-platform sections are stored using the module's pointer width.  On a
 * 64-bit host Dmod_CrossPtr_t is intentionally still 32 bits, so a native
 * host pointer must never be written back into the section.  Keep the file
 * representation intact and return the native pointer as a temporary value.
 */
void* Dmod_Hlp_GetPointerCP( Dmod_Context_t* Context, Dmod_CrossPtr_t Pointer, const char* PointerName )
{
    if( Context == NULL )
    {
        DMOD_LOG_ERROR("Cannot resolve pointer %s - unexpected NULL context\n", PointerName);
        return NULL;
    }
    size_t offset = (size_t)Pointer;
    if( offset == 0 )
    {
        return NULL;
    }
    if( offset > Context->Size )
    {
        DMOD_LOG_ERROR("Cannot resolve pointer %s - invalid offset: 0x%08X\n", PointerName, offset);
        return NULL;
    }
    return (char*)Context->Data + offset;
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
bool Dmod_Hlp_InitPointer( Dmod_Context_t* Context, void** PointerRef, const char* PointerName )
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
