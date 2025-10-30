#ifndef DMOD_PCK_H
#define DMOD_PCK_H

#ifdef __cplusplus
extern "C" {
#endif
#ifndef DMOD_PRIVATE
#   error "This is private DMOD header. Don't include this outside DMOD library"
#endif

#include "dmod_types.h"

extern bool                     Dmod_Pck_IsSlotUsed( Dmod_PackageSlot_t* Slot );
extern void                     Dmod_Pck_ReleaseSlot( Dmod_PackageSlot_t* Slot );
extern const char*              Dmod_Pck_GetPackageName( Dmod_PackageSlot_t* Slot );
extern bool                     Dmod_Pck_IsValidSlot( Dmod_PackageSlot_t* Slot );
extern Dmod_PackageSlot_t*      Dmod_Pck_GetFreeSlot( uint32_t *outIndex );
extern Dmod_PackageSlot_t*      Dmod_Pck_GetSlotByIndex( uint32_t Index );
extern Dmod_PackageSlot_t*      Dmod_Pck_FindSlotByName( const char* PackageName, uint32_t *outIndex );
extern Dmod_PackageSlot_t*      Dmod_Pck_FindSlotByBuffer( const void* Buffer );
extern Dmod_PackageSlot_t*      Dmod_Pck_FindSlotByFilePath( const char* FilePath );
extern size_t                   Dmod_Pck_GetNumberOfPackages( void );
extern Dmod_DmpModuleEntry_t*   Dmod_Pck_FindModuleEntry( Dmod_PackageSlot_t* Slot, const char* ModuleName );
extern Dmod_DmpModuleEntry_t*   Dmod_Pck_FindModuleEntryInPackages( const char* ModuleName, Dmod_PackageSlot_t** outSlot );
extern const char*              Dmod_Pck_GetMainModuleName( Dmod_PackageSlot_t* Slot );

#ifdef __cplusplus
}
#endif
#endif // DMOD_PCK_H