/* The table of the relocations (DMOD_USE_RELDYN) of the module that is linked first, to find out what goes into the table:
   there is none yet, only the symbol that the link of the module asks for. */
#include <stdint.h>

const uint32_t dmod_reldyn[0] __attribute__((section(".got"), used, aligned(4)));
