#ifndef DMOD_MGR_H
#define DMOD_MGR_H

#ifndef DMOD_PRIVATE
#   error "This is private DMOD header. Don't include this outside DMOD library"
#endif

extern bool Dmod_Mgr_IsSystemModule( const char* ModuleName );
extern bool Dmod_Mgr_IsLoaded( const char* ModuleName );
extern bool Dmod_Mgr_IsEnabled( const char* ModuleName );

#endif // DMOD_MGR_H