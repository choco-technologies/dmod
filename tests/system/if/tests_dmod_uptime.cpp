#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <stdint.h>
#include "dmod_sal.h"

// ===============================================================
//                  Tests for Dmod_GetUptime
// ===============================================================

class DmodUptimeTest : public ::testing::Test
{
};

/**
 * @brief Test that Dmod_GetUptime returns a non-zero value on supported platforms
 *
 * On Linux/POSIX platforms, the uptime should be greater than 0 because the
 * system has been running for at least some time before this test executes.
 */
TEST_F(DmodUptimeTest, ReturnsNonZeroOnSupportedPlatform)
{
#if defined(__linux__) || defined(__unix__)
    Dmod_Timestamp_t uptime = Dmod_GetUptime();
    ASSERT_GT(uptime, (Dmod_Timestamp_t)0);
#else
    GTEST_SKIP() << "Dmod_GetUptime not implemented on this platform";
#endif
}

/**
 * @brief Test that Dmod_GetUptime returns a value of type uint64_t (Dmod_Timestamp_t)
 */
TEST_F(DmodUptimeTest, ReturnTypeIsUint64)
{
    Dmod_Timestamp_t uptime = Dmod_GetUptime();
    static_assert(sizeof(uptime) == sizeof(uint64_t), "Dmod_Timestamp_t must be uint64_t");
    (void)uptime;
}

/**
 * @brief Test that successive calls to Dmod_GetUptime return non-decreasing values
 *
 * The uptime counter must be monotonically non-decreasing.
 */
TEST_F(DmodUptimeTest, IsMonotonicallyNonDecreasing)
{
#if defined(__linux__) || defined(__unix__)
    Dmod_Timestamp_t first  = Dmod_GetUptime();
    Dmod_Timestamp_t second = Dmod_GetUptime();

    ASSERT_LE(first, second);
#else
    GTEST_SKIP() << "Dmod_GetUptime not implemented on this platform";
#endif
}
