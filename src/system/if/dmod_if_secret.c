/**
 * @brief Platform-overridable device secret interface.
 */
#include "dmod.h"
#include <errno.h>

DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _TakeDeviceSecret, ( void* Buffer, size_t Size ))
{
    (void)Buffer;
    (void)Size;
    return -ENOSYS;
}
