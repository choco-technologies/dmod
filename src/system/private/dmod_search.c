/** Cached module search directories. No file names or module contents are cached. */
#define DMOD_PRIVATE
#include "dmod.h"
#include "private/dmod_hlp.h"
#include <string.h>
#if DMOD_USE_PTHREAD
#include <pthread.h>
static pthread_mutex_t g_searchLock = PTHREAD_MUTEX_INITIALIZER;
#define LOCK()   pthread_mutex_lock(&g_searchLock)
#define UNLOCK() pthread_mutex_unlock(&g_searchLock)
#else
/* Only metadata is accessed here: never allocate or perform I/O in this lock. */
#define LOCK()   Dmod_EnterCritical()
#define UNLOCK() Dmod_ExitCritical()
#endif

#define SEARCH_SOURCES 8
#define SEARCH_CACHE_SLOTS 4

typedef struct
{
    char* sources[SEARCH_SOURCES];
    Dmod_SearchNode_t* tail;
    size_t references; /* one for the cache, plus one for every active reader */
} SearchSnapshot;

static SearchSnapshot* g_searchCache[SEARCH_CACHE_SLOTS];
static size_t g_nextSlot;
static unsigned long g_generation;

static void FreeNodes(Dmod_SearchNode_t* tail)
{
    while (tail != NULL)
    {
        Dmod_SearchNode_t* prev = tail->Prev;
        Dmod_Free(tail->Path);
        Dmod_Free(tail);
        tail = prev;
    }
}

static void ReleaseSnapshot(SearchSnapshot* snapshot)
{
    if (snapshot == NULL) return;
    LOCK();
    bool last = --snapshot->references == 0;
    UNLOCK();
    if (last)
    {
        FreeNodes(snapshot->tail);
        for (size_t i = 0; i < SEARCH_SOURCES; ++i) Dmod_Free(snapshot->sources[i]);
        Dmod_Free(snapshot);
    }
}

void Dmod_InvalidateModuleSearchPaths(void)
{
    SearchSnapshot* old[SEARCH_CACHE_SLOTS];
    LOCK();
    ++g_generation;
    for (size_t i = 0; i < SEARCH_CACHE_SLOTS; ++i)
    {
        old[i] = g_searchCache[i];
        g_searchCache[i] = NULL;
    }
    UNLOCK();
    for (size_t i = 0; i < SEARCH_CACHE_SLOTS; ++i) ReleaseSnapshot(old[i]);
}

Dmod_SearchNode_t* Dmod_Hlp_AddSearchNode(Dmod_SearchNode_t* tail, const char* path)
{
    Dmod_SearchNode_t* node = Dmod_Malloc(sizeof(*node));
    if (node == NULL) return NULL;
    node->Path = Dmod_StrDup(path);
    if (node->Path == NULL)
    {
        Dmod_Free(node);
        return NULL;
    }
    node->Prev = tail;
    node->Snapshot = NULL;
    return node;
}

void Dmod_Hlp_FreeSearchPathList(Dmod_SearchNode_t* tail)
{
    if (tail != NULL && tail->Snapshot != NULL) ReleaseSnapshot(tail->Snapshot);
    else FreeNodes(tail);
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
    if (!ok) { FreeNodes(matches); return false; }
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

Dmod_SearchNode_t* Dmod_Hlp_PrepareSearchNodes(Dmod_SearchNode_t* tail, const char* paths)
{
    /* Build separately so allocation failure does not consume the caller's list. */
    Dmod_SearchNode_t* added = NULL;
    if (!PreparePaths(&added, paths)) { FreeNodes(added); return tail; }
    if (added == NULL) return tail;
    Dmod_SearchNode_t* last = added;
    while (last->Prev != NULL) last = last->Prev;
    last->Prev = tail;
    return added;
}

static bool Matches(const SearchSnapshot* snapshot, const char* const* sources)
{
    for (size_t i = 0; i < SEARCH_SOURCES; ++i)
        if (strcmp(snapshot->sources[i], sources[i]) != 0) return false;
    return true;
}

Dmod_SearchNode_t* Dmod_Hlp_PrepareModulesSearchNodes(void)
{
    /* Preserve the existing precedence: the last entry added is searched first.
     * Compare VALUES, not environment/context addresses: contexts can be reused. */
    const char* sources[SEARCH_SOURCES] = {
        Dmod_GetEnv("PATH"),
#ifdef DMOD_DMF_DIR
        DMOD_DMF_DIR,
#else
        NULL,
#endif
#ifdef DMOD_DMFC_DIR
        DMOD_DMFC_DIR,
#else
        NULL,
#endif
        DMOD_REPO_PATHS, Dmod_GetRepoDir(), Dmod_GetEnv("DMOD_REPO_PATHS"),
        Dmod_GetEnv("DMOD_DMFC_DIR"), Dmod_GetEnv("DMOD_DMF_DIR")
    };
    for (size_t i = 0; i < SEARCH_SOURCES; ++i)
        if (sources[i] == NULL) sources[i] = "";
    LOCK();
    unsigned long generation = g_generation;
    for (size_t i = 0; i < SEARCH_CACHE_SLOTS; ++i)
    {
        SearchSnapshot* found = g_searchCache[i];
        if (found != NULL && Matches(found, sources))
        {
            Dmod_SearchNode_t* tail = found->tail;
            if (tail != NULL) ++found->references;
            UNLOCK();
            return tail;
        }
    }
    UNLOCK();

    SearchSnapshot* built = Dmod_Malloc(sizeof(*built));
    if (built == NULL) return NULL;
    memset(built, 0, sizeof(*built));
    built->references = 1;
    for (size_t i = 0; i < SEARCH_SOURCES; ++i)
    {
        built->sources[i] = Dmod_StrDup(sources[i]);
        if (built->sources[i] == NULL || !PreparePaths(&built->tail, built->sources[i]))
        {
            ReleaseSnapshot(built); /* never publish a partial expansion */
            return NULL;
        }
    }
    if (built->tail != NULL) built->tail->Snapshot = built;

    SearchSnapshot* retired = NULL;
    LOCK();
    if (generation == g_generation)
    {
        /* Another builder may already have published the same configuration. */
        for (size_t i = 0; i < SEARCH_CACHE_SLOTS; ++i)
        {
            SearchSnapshot* found = g_searchCache[i];
            if (found != NULL && Matches(found, (const char* const*)built->sources))
            {
                Dmod_SearchNode_t* tail = found->tail;
                if (tail != NULL) ++found->references;
                UNLOCK();
                ReleaseSnapshot(built);
                return tail;
            }
        }
        size_t slot = g_nextSlot++ % SEARCH_CACHE_SLOTS;
        retired = g_searchCache[slot];
        g_searchCache[slot] = built;
        ++built->references;
    }
    UNLOCK();
    ReleaseSnapshot(retired);
    Dmod_SearchNode_t* tail = built->tail;
    if (tail == NULL) ReleaseSnapshot(built);
    return tail;
}
