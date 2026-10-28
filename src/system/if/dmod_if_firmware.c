/**
 * @brief Platform-overridable firmware interface: the banks of the firmware images and their choice, the confirmation
 *        of a firmware on trial and the persistent data.
 *
 * The platform with a boot loader that keeps more than one firmware (see DMOD_SAL_FIRMWARE in dmod_sal.h) overrides
 * these; everywhere else (the host included) there is nothing to confirm and nothing is kept.
 */
#include "dmod.h"
#include <errno.h>

DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _FirmwareReady, ( void ))
{
    return -ENOSYS;
}

DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _WritePersistency, ( uint32_t Id, const void* Data, size_t Size ))
{
    (void)Id;
    (void)Data;
    (void)Size;
    return -ENOSYS;
}

DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _ReadPersistency, ( uint32_t Id, void* Buffer, size_t BufferSize ))
{
    (void)Id;
    (void)Buffer;
    (void)BufferSize;
    return -ENOSYS;
}

DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _FirmwareGetBankCount, ( void ))
{
    return -ENOSYS;
}

DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _FirmwareGetRunningBank, ( void ))
{
    return -ENOSYS;
}

DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _FirmwareGetBankInfo, ( int Bank, Dmod_FirmwareBankInfo_t* Info ))
{
    (void)Bank;
    (void)Info;
    return -ENOSYS;
}

DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _FirmwareVerifyBank, ( int Bank ))
{
    (void)Bank;
    return -ENOSYS;
}

DMOD_INPUT_WEAK_API_DECLARATION(Dmod, 1.0, int, _FirmwareSelectBank, ( int Bank, bool Trial ))
{
    (void)Bank;
    (void)Trial;
    return -ENOSYS;
}
