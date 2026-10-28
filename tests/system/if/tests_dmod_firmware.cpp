#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include "dmod_sal.h"

// The host has no boot loader and no persistent memory: the default (weak) implementations say so and do nothing.
// A platform overrides them (dmod-boot does, with the selector in the first sector of the flash).

TEST(DmodFirmwareTest, FirmwareReadyIsUnsupportedByDefault)
{
    EXPECT_EQ(Dmod_FirmwareReady(), -ENOSYS);
    // Repeated confirmations must be harmless as well.
    EXPECT_EQ(Dmod_FirmwareReady(), -ENOSYS);
}

TEST(DmodFirmwareTest, WritePersistencyIsUnsupportedByDefault)
{
    const char data[] = "settings";

    EXPECT_EQ(Dmod_WritePersistency(1, data, sizeof(data)), -ENOSYS);
    // Removing an item is a write of nothing
    EXPECT_EQ(Dmod_WritePersistency(1, nullptr, 0), -ENOSYS);
}

TEST(DmodFirmwareTest, ReadPersistencyIsUnsupportedByDefault)
{
    char buffer[16] = { 'x' };

    EXPECT_EQ(Dmod_ReadPersistency(1, buffer, sizeof(buffer)), -ENOSYS);
    // Asking for the size only
    EXPECT_EQ(Dmod_ReadPersistency(1, nullptr, 0), -ENOSYS);
    // Nothing was copied
    EXPECT_EQ(buffer[0], 'x');
}

TEST(DmodFirmwareTest, BanksAreUnsupportedByDefault)
{
    Dmod_FirmwareBankInfo_t info;

    memset(&info, 0x5A, sizeof(info));
    info.StructSize = sizeof(info);

    EXPECT_EQ(Dmod_FirmwareGetBankCount(), -ENOSYS);
    EXPECT_EQ(Dmod_FirmwareGetRunningBank(), -ENOSYS);
    EXPECT_EQ(Dmod_FirmwareGetBankInfo(0, &info), -ENOSYS);
    EXPECT_EQ(Dmod_FirmwareVerifyBank(0), -ENOSYS);
    EXPECT_EQ(Dmod_FirmwareSelectBank(0, true), -ENOSYS);
    EXPECT_EQ(Dmod_FirmwareSelectBank(1, false), -ENOSYS);

    // The default does not touch what it was given
    EXPECT_EQ(info.State, 0x5A5A5A5Au);
}

TEST(DmodFirmwareTest, BankInfoLayoutIsFixed)
{
    // The structure is handed to the platform (the boot loader): its layout is an interface, no padding in it
    EXPECT_EQ(sizeof(Dmod_FirmwareBankInfo_t), 40u);
    EXPECT_EQ(offsetof(Dmod_FirmwareBankInfo_t, Address), 24u);
    EXPECT_EQ(offsetof(Dmod_FirmwareBankInfo_t, Capacity), 32u);
    EXPECT_EQ(DMOD_FIRMWARE_BANK_NONE, -1);
    EXPECT_EQ((int)Dmod_FirmwareBankState_Empty, 0);
    EXPECT_EQ((int)Dmod_FirmwareBankState_Confirmed, 4);
}
