#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <stdint.h>
#include <limits.h>
#include "dmod_sal.h"

// ===============================================================
//                  Tests for Dmod_GetLeftStackSize
// ===============================================================

class DmodStackTest : public ::testing::Test
{
};

/**
 * @brief Test that Dmod_GetLeftStackSize returns a non-zero value
 *
 * The function should return either a positive stack size (when the
 * implementation is available) or SIZE_MAX as a fallback.
 */
TEST_F(DmodStackTest, ReturnsNonZero)
{
    size_t leftStack = Dmod_GetLeftStackSize();

    ASSERT_NE(leftStack, (size_t)0);
}

/**
 * @brief Test that Dmod_GetLeftStackSize returns a reasonable value
 *
 * The returned value should either be SIZE_MAX (when the implementation
 * is unavailable) or a positive number less than a plausible maximum
 * stack size (e.g. 1 GiB).
 */
TEST_F(DmodStackTest, ReturnsReasonableValue)
{
    size_t leftStack = Dmod_GetLeftStackSize();
    const size_t maxReasonableStack = (size_t)1 * 1024 * 1024 * 1024; /* 1 GiB */

    /* Either unknown (SIZE_MAX) or a value within a sane range */
    ASSERT_TRUE(leftStack == SIZE_MAX || leftStack < maxReasonableStack);
}

/**
 * @brief Test that nested calls return decreasing values
 *
 * Since each function call uses some stack space, calling
 * Dmod_GetLeftStackSize from a deeper call frame should return
 * a smaller value than from a shallower one (when the implementation
 * is available, i.e. not SIZE_MAX).
 */
static size_t GetLeftStackInner(void)
{
    return Dmod_GetLeftStackSize();
}

TEST_F(DmodStackTest, DeeperCallReturnsLessStack)
{
    size_t outerStack = Dmod_GetLeftStackSize();

    if( outerStack == SIZE_MAX )
    {
        GTEST_SKIP() << "Dmod_GetLeftStackSize not implemented on this platform";
    }

    size_t innerStack = GetLeftStackInner();

    ASSERT_LT(innerStack, outerStack);
}
