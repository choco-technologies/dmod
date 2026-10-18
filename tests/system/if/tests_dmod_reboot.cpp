#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <errno.h>
#include "dmod_sal.h"

TEST(DmodRebootTest, DefaultImplementationIsUnsupported)
{
    EXPECT_EQ(Dmod_Reboot(), -ENOSYS);
    // Repeated attempts must also be harmless on hosts without an override.
    EXPECT_EQ(Dmod_Reboot(), -ENOSYS);
}
