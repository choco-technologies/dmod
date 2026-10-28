/**
 * @file dmcomp_strerror.c
 * @brief Names of the results (a file of its own: a firmware that does not log them does not carry the strings)
 */
#include "dmcomp.h"

const char* dmcomp_strerror( int result )
{
    switch( result )
    {
        case DMCOMP_OK:             return "ok";
        case DMCOMP_ERR_PARAM:      return "invalid argument";
        case DMCOMP_ERR_DATA:       return "invalid compressed data";
        case DMCOMP_ERR_TRUNCATED:  return "compressed data truncated";
        case DMCOMP_ERR_OVERFLOW:   return "output buffer too small";
        case DMCOMP_ERR_MEMORY:     return "out of memory";
        default:                    return "unknown error";
    }
}
