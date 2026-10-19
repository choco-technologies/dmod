#define DMOD_PRIVATE
#include <string.h>
#include "dmod.h"
#include "private/dmod_hlp.h"

static Dmod_SearchNode_t* SearchPaths = NULL;
static void* SearchPathsMutex = NULL;

static Dmod_SearchNode_t* ExpandSearchPath( Dmod_SearchNode_t* Tail, const char* Path );

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
    newNode->References = 0;
    return newNode;
}

/**
 * @brief Free search path list
 * 
 * @param Tail Tail of the list to free
 */
void Dmod_Hlp_FreeSearchPathList( Dmod_SearchNode_t* Tail )
{
    if( Tail == NULL )
    {
        return;
    }
    if( SearchPathsMutex != NULL )
    {
        Dmod_Mutex_Lock( SearchPathsMutex );
        bool shared = Tail->References != 0 && --Tail->References != 0;
        Dmod_Mutex_Unlock( SearchPathsMutex );
        if( shared )
        {
            return;
        }
    }
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

    /* Keep tokenization local: directory callbacks may also parse strings. */
    char* token = pathsCopy;
    while( *token != '\0' )
    {
        token += strspn( token, DMOD_ARRAY_SEP );
        if( *token == '\0' ) break;
        char* end = token + strcspn( token, DMOD_ARRAY_SEP );
        char separator = *end;
        *end = '\0';
        if( strchr( token, '*' ) != NULL )
        {
            Tail = ExpandSearchPath( Tail, token );
        }
        else
        {
            Dmod_SearchNode_t* node = Dmod_Hlp_AddSearchNode( Tail, token );
            if( node != NULL ) Tail = node;
        }
        if( separator == '\0' ) break;
        token = end + 1;
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
    if( SearchPathsMutex != NULL )
    {
        Dmod_Mutex_Lock( SearchPathsMutex );
        if( SearchPaths != NULL )
        {
            ++SearchPaths->References;
            Dmod_SearchNode_t* tail = SearchPaths;
            Dmod_Mutex_Unlock( SearchPathsMutex );
            return tail;
        }
    }
    Dmod_SearchNode_t* tail = NULL;
    tail = Dmod_Hlp_PrepareSearchNodes( tail, Dmod_GetEnv("PATH") );
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

    if( SearchPathsMutex != NULL )
    {
        SearchPaths = tail;
        if( tail != NULL ) tail->References = 2; /* Cache and caller. */
        Dmod_Mutex_Unlock( SearchPathsMutex );
    }
    return tail;
}

/* Initialization and deinitialization require quiescent callers, as does the
 * rest of the system lifecycle. Without a SAL mutex, searches remain uncached. */
void Dmod_Hlp_InitSearchCache( void )
{
    if( SearchPathsMutex == NULL )
    {
        SearchPathsMutex = Dmod_Mutex_New( false );
    }
}

void Dmod_Hlp_DeinitSearchCache( void )
{
    Dmod_InvalidateModuleSearchPaths();
    if( SearchPathsMutex != NULL )
    {
        Dmod_Mutex_Delete( SearchPathsMutex );
        SearchPathsMutex = NULL;
    }
}

void Dmod_InvalidateModuleSearchPaths( void )
{
    if( SearchPathsMutex == NULL ) return;
    Dmod_Mutex_Lock( SearchPathsMutex );
    Dmod_SearchNode_t* old = SearchPaths;
    SearchPaths = NULL;
    Dmod_Mutex_Unlock( SearchPathsMutex );
    Dmod_Hlp_FreeSearchPathList( old );
}

/* Expand a single complete '*' component, in a mount path. Only the matched
 * directory must exist; bin may arrive later with an upload. */
static Dmod_SearchNode_t* ExpandSearchPath( Dmod_SearchNode_t* Tail, const char* Path )
{
    const char* star = strchr( Path, '*' );
    if( Path[0] != '/' || star == Path || star[-1] != '/' ||
        (star[1] != '/' && star[1] != '\0') || strchr( star + 1, '*' ) != NULL )
    {
        DMOD_LOG_WARN("Unsupported module search wildcard: %s\n", Path);
        return Tail;
    }

    size_t prefixLength = (size_t)(star - Path);
    char* parent = Dmod_StrDup( Path );
    if( parent == NULL ) return Tail;
    parent[prefixLength] = '\0';
    void* dir = Dmod_OpenDir( parent );
    Dmod_Free( parent );
    if( dir == NULL ) return Tail;

    const char* name;
    while( (name = Dmod_ReadDir( dir )) != NULL )
    {
        if( strcmp( name, "." ) == 0 || strcmp( name, ".." ) == 0 ) continue;
        size_t nameLength = strlen( name );
        char* path = Dmod_Malloc( prefixLength + nameLength + strlen( star + 1 ) + 1 );
        if( path == NULL ) break;
        memcpy( path, Path, prefixLength );
        memcpy( path + prefixLength, name, nameLength + 1 );
        void* child = Dmod_OpenDir( path );
        if( child != NULL )
        {
            Dmod_CloseDir( child );
            strcpy( path + prefixLength + nameLength, star + 1 );
            Dmod_SearchNode_t* node = Dmod_Hlp_AddSearchNode( Tail, path );
            if( node != NULL ) Tail = node;
        }
        Dmod_Free( path );
    }
    Dmod_CloseDir( dir );
    return Tail;
}
