#define DMOD_PRIVATE
#include <string.h>
#include "dmod.h"
#include "private/dmod_hlp.h"

typedef struct Dmod_SearchCache
{
    Dmod_SearchNode_t* Sources;
    Dmod_SearchNode_t* Paths;
    size_t References;
    struct Dmod_SearchCache* Next;
} Dmod_SearchCache_t;

static Dmod_SearchCache_t* SearchCache = NULL;
static void* SearchCacheMutex = NULL;
static unsigned long SearchCacheGeneration = 0;

static void ReleaseSearchCache( Dmod_SearchCache_t* Cache );
static bool PreparePaths( Dmod_SearchNode_t** Tail, const char* Paths );
static bool MatchSearchSources( Dmod_SearchNode_t* Sources );
typedef enum
{
    SearchSource_Path,
    SearchSource_DmfDir,
    SearchSource_DmfcDir,
    SearchSource_RepoPaths,
    SearchSource_RepoDir,
    SearchSource_EnvRepoPaths,
    SearchSource_EnvDmfcDir,
    SearchSource_EnvDmfDir,
    SearchSource_Count
} SearchSource_t;

static const char* GetSearchSource( SearchSource_t Source );

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
    newNode->Snapshot = NULL;
    return newNode;
}

/**
 * @brief Free search path list
 * 
 * @param Tail Tail of the list to free
 */
void Dmod_Hlp_FreeSearchPathList( Dmod_SearchNode_t* Tail )
{
    if( Tail != NULL && Tail->Snapshot != NULL )
    {
        ReleaseSearchCache( Tail->Snapshot );
        return;
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
    Dmod_SearchNode_t* added = NULL;
    if( !PreparePaths( &added, Paths ) )
    {
        Dmod_Hlp_FreeSearchPathList( added );
        return Tail;
    }
    if( added == NULL )
    {
        return Tail;
    }
    Dmod_SearchNode_t* last = added;
    while( last->Prev != NULL )
    {
        last = last->Prev;
    }
    last->Prev = Tail;
    return added;
}

/**
 * @brief Prepare search nodes for modules searching paths
 * 
 * @return Pointer to the search nodes
 */
Dmod_SearchNode_t* Dmod_Hlp_PrepareModulesSearchNodes( void )
{
    unsigned long generation = 0;
    if( SearchCacheMutex != NULL )
    {
        Dmod_Mutex_Lock( SearchCacheMutex );
        generation = SearchCacheGeneration;
        for( Dmod_SearchCache_t* cached = SearchCache; cached != NULL; cached = cached->Next )
        {
            if( MatchSearchSources( cached->Sources ) )
            {
                Dmod_SearchNode_t* paths = cached->Paths;
                if( paths != NULL )
                {
                    ++cached->References;
                }
                Dmod_Mutex_Unlock( SearchCacheMutex );
                return paths;
            }
        }
        Dmod_Mutex_Unlock( SearchCacheMutex );
    }

    Dmod_SearchNode_t* sources = NULL;
    bool complete = true;
    /* Prepend in reverse order so the source list retains the existing order. */
    for( int i = SearchSource_Count - 1; i >= 0; --i )
    {
        Dmod_SearchNode_t* source = Dmod_Hlp_AddSearchNode( sources, GetSearchSource( i ) );
        if( source == NULL )
        {
            complete = false;
            break;
        }
        sources = source;
    }

    Dmod_SearchNode_t* tail = NULL;
    for( Dmod_SearchNode_t* source = sources; complete && source != NULL; source = source->Prev )
    {
        complete = PreparePaths( &tail, source->Path );
    }
    if( !complete || SearchCacheMutex == NULL )
    {
        Dmod_Hlp_FreeSearchPathList( sources );
        if( !complete )
        {
            Dmod_Hlp_FreeSearchPathList( tail );
            return NULL;
        }
        return tail;
    }

    Dmod_SearchCache_t* built = Dmod_Malloc( sizeof(*built) );
    if( built == NULL )
    {
        Dmod_Hlp_FreeSearchPathList( sources );
        return tail;
    }
    built->Sources = sources;
    built->Paths = tail;
    built->References = 1;
    built->Next = NULL;
    if( tail != NULL )
    {
        tail->Snapshot = built;
    }

    Dmod_Mutex_Lock( SearchCacheMutex );
    if( generation == SearchCacheGeneration )
    {
        Dmod_SearchCache_t* cached = SearchCache;
        while( cached != NULL && !MatchSearchSources( cached->Sources ) )
        {
            cached = cached->Next;
        }
        /* A concurrent builder may have published this configuration already.
         * In that case our snapshot stays private to this lookup. */
        if( cached == NULL )
        {
            built->Next = SearchCache;
            SearchCache = built;
            ++built->References;
        }
    }
    Dmod_Mutex_Unlock( SearchCacheMutex );
    if( tail == NULL )
    {
        ReleaseSearchCache( built );
    }
    return tail;
}

/* The system initializes/deinitializes the mutex while callers are quiescent.
 * Before initialization, or without mutex support, lookups remain uncached. */
void Dmod_Hlp_InitSearchCache( void )
{
    if( SearchCacheMutex == NULL )
    {
        SearchCacheMutex = Dmod_Mutex_New( false );
    }
}

void Dmod_Hlp_DeinitSearchCache( void )
{
    Dmod_InvalidateModuleSearchPaths();
    if( SearchCacheMutex != NULL )
    {
        Dmod_Mutex_Delete( SearchCacheMutex );
        SearchCacheMutex = NULL;
    }
}

static void ReleaseSearchCache( Dmod_SearchCache_t* Cache )
{
    Dmod_Mutex_Lock( SearchCacheMutex );
    bool last = --Cache->References == 0;
    Dmod_Mutex_Unlock( SearchCacheMutex );
    if( last )
    {
        if( Cache->Paths != NULL )
        {
            Cache->Paths->Snapshot = NULL;
        }
        Dmod_Hlp_FreeSearchPathList( Cache->Paths );
        Dmod_Hlp_FreeSearchPathList( Cache->Sources );
        Dmod_Free( Cache );
    }
}

void Dmod_InvalidateModuleSearchPaths( void )
{
    if( SearchCacheMutex == NULL )
    {
        return;
    }
    Dmod_Mutex_Lock( SearchCacheMutex );
    ++SearchCacheGeneration;
    Dmod_SearchCache_t* old = SearchCache;
    SearchCache = NULL;
    Dmod_Mutex_Unlock( SearchCacheMutex );
    while( old != NULL )
    {
        Dmod_SearchCache_t* next = old->Next;
        ReleaseSearchCache( old );
        old = next;
    }
}

/* One definition of the effective configuration, used for both cache hits
 * and construction of the linked source list. No directory I/O here. */
static const char* GetSearchSource( SearchSource_t Source )
{
    const char* value = NULL;
    switch( Source )
    {
        case SearchSource_Path:         value = Dmod_GetEnv("PATH"); break;
        case SearchSource_DmfDir:
            #ifdef DMOD_DMF_DIR
            value = DMOD_DMF_DIR;
            #endif
            break;
        case SearchSource_DmfcDir:
            #ifdef DMOD_DMFC_DIR
            value = DMOD_DMFC_DIR;
            #endif
            break;
        case SearchSource_RepoPaths:    value = DMOD_REPO_PATHS; break;
        case SearchSource_RepoDir:      value = Dmod_GetRepoDir(); break;
        case SearchSource_EnvRepoPaths: value = Dmod_GetEnv("DMOD_REPO_PATHS"); break;
        case SearchSource_EnvDmfcDir:   value = Dmod_GetEnv("DMOD_DMFC_DIR"); break;
        case SearchSource_EnvDmfDir:    value = Dmod_GetEnv("DMOD_DMF_DIR"); break;
        default: break;
    }
    return value != NULL ? value : "";
}

static bool MatchSearchSources( Dmod_SearchNode_t* Sources )
{
    for( int i = 0; i < SearchSource_Count; ++i )
    {
        if( Sources == NULL || strcmp( Sources->Path, GetSearchSource( i ) ) != 0 )
        {
            return false;
        }
        Sources = Sources->Prev;
    }
    return Sources == NULL;
}

/* Expand one complete '*' directory component in an absolute path. The suffix
 * stays literal and need not exist (e.g. bin may be created by a later upload).
 * No recursive globs, filename patterns or relative wildcard paths. */
static bool ExpandPath(Dmod_SearchNode_t** tail, const char* path)
{
    const char* star = strchr(path, '*');
    if (star == NULL)
    {
        Dmod_SearchNode_t* node = Dmod_Hlp_AddSearchNode(*tail, path);
        if (node == NULL) return false;
        *tail = node;
        return true;
    }
    if (path[0] != '/' || star == path || star[-1] != '/' ||
        (star[1] != '/' && star[1] != '\0') || strchr(star + 1, '*') != NULL)
    {
        DMOD_LOG_WARN("Unsupported module search wildcard: %s\n", path);
        return true;
    }

    size_t prefixLen = (size_t)(star - path);
    char* parent = Dmod_StrDup(path);
    if (parent == NULL) return false;
    parent[prefixLen] = '\0';
    void* dir = Dmod_OpenDir(parent);
    Dmod_Free(parent);
    if (dir == NULL) return true; /* no mounts yet: cached until invalidation */

    bool ok = true;
    Dmod_SearchNode_t* matches = NULL;
    const char* name;
    while ((name = Dmod_ReadDir(dir)) != NULL)
    {
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
        size_t nameLen = strlen(name);
        char* expanded = Dmod_Malloc(prefixLen + nameLen + strlen(star + 1) + 1);
        if (expanded == NULL) { ok = false; break; }
        memcpy(expanded, path, prefixLen);
        memcpy(expanded + prefixLen, name, nameLen + 1);
        /* ReadDir works on per-handle storage; unlike some ReadDirEx SALs it
         * does not share a static entry across simultaneous cache builders. */
        void* child = Dmod_OpenDir(expanded);
        if (child == NULL) { Dmod_Free(expanded); continue; }
        Dmod_CloseDir(child);
        strcpy(expanded + prefixLen + nameLen, star + 1);
        Dmod_SearchNode_t* node = Dmod_Hlp_AddSearchNode(NULL, expanded);
        Dmod_Free(expanded);
        if (node == NULL) { ok = false; break; }
        /* Deterministic first match across cards, independent of readdir order. */
        Dmod_SearchNode_t** at = &matches;
        while (*at != NULL && strcmp((*at)->Path, node->Path) < 0) at = &(*at)->Prev;
        node->Prev = *at;
        *at = node;
    }
    Dmod_CloseDir(dir);
    if (!ok) { Dmod_Hlp_FreeSearchPathList(matches); return false; }
    if (matches != NULL)
    {
        Dmod_SearchNode_t* last = matches;
        while (last->Prev != NULL) last = last->Prev;
        last->Prev = *tail;
        *tail = matches;
    }
    return true;
}

static bool PreparePaths(Dmod_SearchNode_t** tail, const char* paths)
{
    if (paths == NULL || *paths == '\0') return true;
    char* copy = Dmod_StrDup(paths);
    if (copy == NULL) return false;
    char* token = copy;
    bool ok = true;
    /* Do not use strtok: different contexts may rebuild concurrently. */
    while (*token != '\0')
    {
        token += strspn(token, DMOD_ARRAY_SEP);
        if (*token == '\0') break;
        char* end = token + strcspn(token, DMOD_ARRAY_SEP);
        char separator = *end;
        *end = '\0';
        if (!ExpandPath(tail, token)) { ok = false; break; }
        if (separator == '\0') break;
        token = end + 1;
    }
    Dmod_Free(copy);
    return ok;
}
