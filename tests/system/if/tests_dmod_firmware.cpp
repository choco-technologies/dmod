#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <errno.h>
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
