#define DMOD_PRIVATE
#include "dmod.h"
#include "private/dmod_hlp.h"

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

/**
 * @brief Add search path node
 * 
 * @param Tail  Tail of the list
 * @param Path  Path to add
 * 
 * @return New node
 */
Dmod_SearchNode_t* Dmod_Hlp_AddSearchNode( Dmod_SearchNode_t* Tail, const char* Path )
{
    Dmod_SearchNode_t* newNode = (Dmod_SearchNode_t*)Dmod_Malloc( sizeof(Dmod_SearchNode_t) );
    if( newNode == NULL )
    {
        DMOD_LOG_ERROR("Cannot add search node - out of memory\n");
        return NULL;
    }
    newNode->Path = (char*)Dmod_Malloc( strlen( Path ) + 1 );
    if( newNode->Path == NULL )
    {
        DMOD_LOG_ERROR("Cannot add search node - out of memory for path\n");
        Dmod_Free( newNode );
        return NULL;
    }
    strcpy( newNode->Path, Path );
    newNode->Prev = Tail;
    return newNode;
}

/**
 * @brief Free search path list
 * 
 * @param Tail Tail of the list to free
 */
void Dmod_Hlp_FreeSearchPathList( Dmod_SearchNode_t* Tail )
{
    Dmod_SearchNode_t* current = Tail;
    while( current != NULL )
    {
        Dmod_SearchNode_t* prev = current->Prev;
        Dmod_Free( current->Path );
        Dmod_Free( current );
        current = prev;
    }
}

/**
 * @brief Prepare search nodes from paths string (seperated by DMOD_ARRAY_SEP)
 * 
 * @param Tail Tail of the list
 * @param Paths Paths string seperated by DMOD_ARRAY_SEP
 * 
 * @return
 */
Dmod_SearchNode_t* Dmod_Hlp_PrepareSearchNodes( Dmod_SearchNode_t* Tail, const char* Paths )
{
    if( Paths == NULL )
    {
        return Tail;
    }

    char* pathsCopy = (char*)Dmod_Malloc( strlen( Paths ) + 1 );
    if( pathsCopy == NULL )
    {
        DMOD_LOG_ERROR("Cannot prepare search nodes - out of memory\n");
        return Tail;
    }
    strcpy( pathsCopy, Paths );

    char* token = strtok( pathsCopy, DMOD_ARRAY_SEP );
    while( token != NULL )
    {
        Tail = Dmod_Hlp_AddSearchNode( Tail, token );
        token = strtok( NULL, DMOD_ARRAY_SEP );
    }

    Dmod_Free( pathsCopy );
    return Tail;
}

/**
 * @brief Prepare search nodes for modules searching paths
 * 
 * @return Pointer to the search nodes
 */
Dmod_SearchNode_t* Dmod_Hlp_PrepareModulesSearchNodes( void )
{
    Dmod_SearchNode_t* tail = NULL;
    #ifdef DMOD_DMF_DIR
    tail = Dmod_Hlp_PrepareSearchNodes( tail, DMOD_DMF_DIR );
    #endif
    #ifdef DMOD_DMFC_DIR
    tail = Dmod_Hlp_PrepareSearchNodes( tail, DMOD_DMFC_DIR );
    #endif
    tail = Dmod_Hlp_PrepareSearchNodes( tail, DMOD_REPO_PATHS );
    tail = Dmod_Hlp_PrepareSearchNodes( tail, Dmod_GetRepoDir() );
    tail = Dmod_Hlp_PrepareSearchNodes( tail, Dmod_GetEnv("DMOD_REPO_PATHS") );
    tail = Dmod_Hlp_PrepareSearchNodes( tail, Dmod_GetEnv("DMOD_DMFC_DIR") );
    tail = Dmod_Hlp_PrepareSearchNodes( tail, Dmod_GetEnv("DMOD_DMF_DIR") );

    return tail;
}