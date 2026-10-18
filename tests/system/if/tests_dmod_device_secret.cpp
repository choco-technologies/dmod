#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <errno.h>
#include "dmod_sal.h"

TEST(DmodDeviceSecretTest, DefaultImplementationHasNoSecret)
{
    unsigned char buffer[32];

    EXPECT_EQ(Dmod_TakeDeviceSecret(buffer, sizeof(buffer)), -ENOSYS);
    // Repeated attempts must also be harmless on hosts without an override.
    EXPECT_EQ(Dmod_TakeDeviceSecret(buffer, sizeof(buffer)), -ENOSYS);
}
