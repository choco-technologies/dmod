#ifndef DMOD_SCREEN_H
#define DMOD_SCREEN_H

#include <stdbool.h>

/* Opaque, case-sensitive variant IDs; no interpretation of pixel dimensions.
 * Portable single-component names also work as asset-directory names. */
#define DMOD_SCREEN_MAX_LEN 128
static inline bool Dmod_Screen_IsValid(const char* screen)
{
    if (!screen || !*screen) return false;
    unsigned length = 0;
    for (const unsigned char* p = (const unsigned char*)screen; *p; p++) {
        bool alnum = (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
                     (*p >= '0' && *p <= '9');
        if ((!alnum && *p != '_' && (length == 0 || (*p != '-' && *p != '.'))) ||
            ++length >= DMOD_SCREEN_MAX_LEN) return false;
    }
    return true;
}
#endif
