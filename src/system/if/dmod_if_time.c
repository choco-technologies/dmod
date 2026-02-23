/**
 * MIT License
 * 
 * Copyright (c) 2024 Patryk Kubiak
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * @brief Time interface for DMOD SAL
 * @date 2024-12-20
 * 
 */

#define _POSIX_C_SOURCE 200809L
#include "dmod.h"
#if DMOD_USE_TIME_H
#   include <time.h>
#endif

//==============================================================================
//                              FUNCTIONS DECLARATIONS
//==============================================================================

/**
 * @brief Get the time elapsed since system startup (uptime) in milliseconds
 * 
 * Returns the number of milliseconds elapsed since the system started.
 * On Linux/POSIX systems this uses CLOCK_BOOTTIME (or CLOCK_MONOTONIC as
 * a fallback). On unsupported platforms, 0 is returned.
 * 
 * @return Uptime in milliseconds as Dmod_Timestamp_t (uint64_t)
 */
DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, Dmod_Timestamp_t, _GetUptime, ( void ))
{
#if DMOD_USE_TIME_H
    struct timespec ts;
#   if defined(CLOCK_BOOTTIME)
    clockid_t clockId = CLOCK_BOOTTIME;
#   else
    clockid_t clockId = CLOCK_MONOTONIC;
#   endif
    if( clock_gettime( clockId, &ts ) != 0 )
    {
        return (Dmod_Timestamp_t)0;
    }
    return (Dmod_Timestamp_t)ts.tv_sec * 1000U + (Dmod_Timestamp_t)ts.tv_nsec / 1000000U;
#else
    return (Dmod_Timestamp_t)0;
#endif
}
