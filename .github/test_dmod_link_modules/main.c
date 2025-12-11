#include <dmod.h>

// Test that we can include dmini headers
// This file should exist if dmod_link_modules worked correctly
#ifdef __has_include
#  if __has_include(<dmini.h>)
#    include <dmini.h>
#    define HAVE_DMINI_H 1
#  endif
#endif

int main(int argc, char** argv)
{
    Dmod_Printf("Test dmod_link_modules with dmini\n");
    
#ifdef HAVE_DMINI_H
    Dmod_Printf("SUCCESS: dmini.h header is available\n");
#else
    Dmod_Printf("WARNING: dmini.h header not found\n");
#endif
    
    return 0;
}
