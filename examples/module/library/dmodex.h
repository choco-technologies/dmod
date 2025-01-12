#ifndef EXAMPLE_LIB_H
#define EXAMPLE_LIB_H

#include "dmod.h"
#include "dmodex_defs.h"

dmod_dmodex_api( 1.0, void, _foo, (const char* message) );
dmod_dmodex_global_api( 1.0, void, _print, (const char* message) );
dmod_dmodex_mal( 1.0, void, _bar, (const char* message) );
dmod_dmodex_global_mal( 1.0, void, _example, (const char* message) );

#endif // EXAMPLE_LIB_H
