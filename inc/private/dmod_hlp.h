#ifndef DMOD_HLP_H
#define DMOD_HLP_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DMOD_PRIVATE
#   error "This is private DMOD header. Don't include this outside DMOD library"
#endif

#include "dmod_types.h"

extern void*              Dmod_Hlp_GetPointerCP ( Dmod_Context_t* Context, Dmod_CrossPtr_t Pointer, const char* PointerName );
extern bool               Dmod_Hlp_InitPointer( Dmod_Context_t* Context, void** PointerRef, const char* PointerName );

/* A pointer to the code of a module (an entry point, a function that it exports): the address that the code is executed from.
   With DMOD_USE_RELDYN that may be another address than the one that the module is written at (the Xtensa executes the RAM through
   the instruction bus, which sees it elsewhere than the data bus does - Dmod_Hlp_CodeAddress, see docs/reldyn.md). Without it,
   there is one address. */
#ifdef DMOD_USE_RELDYN
/** Starts the table of the relocations in the section .got: scripts/reldyn.cmake writes it, the loader looks for it. It is 1 because
    the tools that load a module on the host check every word of the section to be an offset in the module; no offset of the table
    (a multiple of 4, behind the header) and no address of the GOT is 1 */
#define DMOD_RELDYN_MAGIC           1u
extern void*               Dmod_Hlp_CodeAddress( void* Address );
extern bool                Dmod_Hlp_InitCodePointer( Dmod_Context_t* Context, void** PointerRef, const char* PointerName );
#else
#define Dmod_Hlp_InitCodePointer    Dmod_Hlp_InitPointer
#endif
extern Dmod_SearchNode_t* Dmod_Hlp_AddSearchNode( Dmod_SearchNode_t* Tail, const char* Path );
extern void               Dmod_Hlp_FreeSearchPathList( Dmod_SearchNode_t* Tail );
extern Dmod_SearchNode_t* Dmod_Hlp_PrepareSearchNodes( Dmod_SearchNode_t* Tail, const char* Paths );
extern void               Dmod_Hlp_InitSearchCache( void );
extern void               Dmod_Hlp_DeinitSearchCache( void );
/* Shared immutable snapshot: release with Dmod_Hlp_FreeSearchPathList().
 * Do not append to or modify the returned nodes. */
extern Dmod_SearchNode_t* Dmod_Hlp_PrepareModulesSearchNodes( void );

#ifdef __cplusplus
}
#endif
#endif // DMOD_HLP_H
