#include <gtest/gtest.h>
#include "dmod.h"

// ===============================================================
//   Tests for DMOD_LOG_STEP_* macros and Dmod_GetStepBar helper
// ===============================================================

class DmodLogStepTest : public ::testing::Test
{
};

/**
 * @brief DMOD_LOG_STEP_BEGIN does not crash
 */
TEST_F(DmodLogStepTest, LogStepBegin_NoCrash)
{
    DMOD_LOG_STEP_BEGIN("starting operation\n");
}

/**
 * @brief DMOD_LOG_STEP_PROGRESS does not crash at 0%
 */
TEST_F(DmodLogStepTest, LogStepProgress_Zero_NoCrash)
{
    DMOD_LOG_STEP_PROGRESS(0, "operation in progress\n");
}

/**
 * @brief DMOD_LOG_STEP_PROGRESS does not crash at 50%
 */
TEST_F(DmodLogStepTest, LogStepProgress_Mid_NoCrash)
{
    DMOD_LOG_STEP_PROGRESS(50, "operation in progress\n");
}

/**
 * @brief DMOD_LOG_STEP_PROGRESS does not crash at 100%
 */
TEST_F(DmodLogStepTest, LogStepProgress_Full_NoCrash)
{
    DMOD_LOG_STEP_PROGRESS(100, "operation in progress\n");
}

/**
 * @brief DMOD_LOG_STEP with result 0 (success) does not crash
 */
TEST_F(DmodLogStepTest, LogStep_Success_NoCrash)
{
    DMOD_LOG_STEP(0, "operation result\n");
}

/**
 * @brief DMOD_LOG_STEP with non-zero result (failure) does not crash
 */
TEST_F(DmodLogStepTest, LogStep_Failure_NoCrash)
{
    DMOD_LOG_STEP(1, "operation result\n");
}

/**
 * @brief Full sequence (begin, progress, step) does not crash
 */
TEST_F(DmodLogStepTest, FullSequence_NoCrash)
{
    DMOD_LOG_STEP_BEGIN("loading module\n");
    DMOD_LOG_STEP_PROGRESS(25, "loading module\n");
    DMOD_LOG_STEP_PROGRESS(75, "loading module\n");
    DMOD_LOG_STEP(0, "loading module\n");
}

/**
 * @brief Dmod_GetStepBar returns non-NULL for 0%
 */
TEST_F(DmodLogStepTest, GetStepBar_Zero_NotNull)
{
    EXPECT_NE(Dmod_GetStepBar(0), nullptr);
}

/**
 * @brief Dmod_GetStepBar returns non-NULL for 50%
 */
TEST_F(DmodLogStepTest, GetStepBar_Mid_NotNull)
{
    EXPECT_NE(Dmod_GetStepBar(50), nullptr);
}

/**
 * @brief Dmod_GetStepBar returns non-NULL for 100%
 */
TEST_F(DmodLogStepTest, GetStepBar_Full_NotNull)
{
    EXPECT_NE(Dmod_GetStepBar(100), nullptr);
}

/**
 * @brief Dmod_GetStepBar clamps negative percentages without crashing
 */
TEST_F(DmodLogStepTest, GetStepBar_Negative_NoCrash)
{
    const char* bar = Dmod_GetStepBar(-10);
    EXPECT_NE(bar, nullptr);
    /* negative should clamp to the same bar as 0% */
    EXPECT_STREQ(bar, Dmod_GetStepBar(0));
}

/**
 * @brief Dmod_GetStepBar clamps percentages above 100 without crashing
 */
TEST_F(DmodLogStepTest, GetStepBar_Over100_NoCrash)
{
    const char* bar = Dmod_GetStepBar(110);
    EXPECT_NE(bar, nullptr);
    /* values > 100 should clamp to the same bar as 100% */
    EXPECT_STREQ(bar, Dmod_GetStepBar(100));
}

/* Count the number of filled-block UTF-8 sequences (U+2588, encoded as
 * \xe2\x96\x88) in a bar string.  Each filled segment adds one such sequence.
 */
static int CountFilledBlocks(const char* bar)
{
    int count = 0;
    while (bar && *bar)
    {
        if ((unsigned char)bar[0] == 0xe2 &&
            (unsigned char)bar[1] == 0x96 &&
            (unsigned char)bar[2] == 0x88)
        {
            count++;
            bar += 3;
        }
        else
        {
            bar++;
        }
    }
    return count;
}

/**
 * @brief Dmod_GetStepBar returns bars with non-decreasing fill as percentage rises
 *
 * The seven discrete states (index 0..6, one per ~17%) should never regress:
 * each representative percentage must have at least as many filled blocks as
 * the previous one.
 */
TEST_F(DmodLogStepTest, GetStepBar_Monotone)
{
    int prevFill = CountFilledBlocks(Dmod_GetStepBar(0));

    /* Walk through representative percentages and ensure fill never decreases */
    int percentages[] = { 17, 34, 50, 67, 84, 100 };
    for (int i = 0; i < (int)(sizeof(percentages) / sizeof(percentages[0])); i++)
    {
        const char* cur = Dmod_GetStepBar(percentages[i]);
        ASSERT_NE(cur, nullptr);
        int curFill = CountFilledBlocks(cur);
        EXPECT_GE(curFill, prevFill);
        prevFill = curFill;
    }
}
