#ifndef EXAMPLE_LIB_H
#define EXAMPLE_LIB_H

#include "dmod.h"
#include "example_lib_defs.h"

dmod_api_example_lib( 1.0, void, _foo, (const char* message) );
dmod_api_example_lib_gen( 1.0, void, _print, (const char* message) );
dmod_mal_example_lib( 1.0, void, _bar, (const char* message) );

#endif // EXAMPLE_LIB_H
