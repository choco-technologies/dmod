#include "dmod_test.h"

DMOD_TEST_STEP(arithmetic)
{
    DMOD_TEST_EXPECT_EQ(20 + 22, 42);
}
