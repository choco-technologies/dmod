#ifndef DMOD_SCREEN_H
#define DMOD_SCREEN_H

#include <stdbool.h>

/* Canonical pixel dimensions, each fitting the DMV header's uint16_t. */
#define DMOD_SCREEN_MAX_LEN 12
static inline bool Dmod_Screen_IsValid(const char* screen)
{
    if (!screen) return false;
    const char* p = screen;
    for (unsigned dimension = 0; dimension < 2; dimension++) {
        if (*p < '1' || *p > '9') return false;
        unsigned value = 0;
        do {
            value = value * 10 + (unsigned)(*p++ - '0');
            if (value > 65535) return false;
        } while (*p >= '0' && *p <= '9');
        if (dimension == 0 && *p++ != 'x') return false;
    }
    return *p == '\0';
}
#endif
