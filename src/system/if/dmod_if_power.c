/**
 * @brief Platform-overridable system power interface.
 */
#include "dmod.h"
#include <errno.h>

DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _Reboot, ( void ))
{
    return -ENOSYS;
}
