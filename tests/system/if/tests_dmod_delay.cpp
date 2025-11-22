#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <chrono>
#include "dmod_sal.h"

// ===============================================================
//                  Tests for Delay and Sleep Functions
// ===============================================================

class DmodDelayTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
    
    // Helper function to measure elapsed time in microseconds
    template<typename Func>
    uint64_t measureTime(Func func)
    {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }
};

/**
 * @brief Test for Dmod_DelayUs with zero delay
 * 
 * The test checks if the function handles zero delay correctly.
 */
TEST_F(DmodDelayTest, DelayUsZero)
{
    bool result = Dmod_DelayUs(0);
    ASSERT_TRUE(result);
}

/**
 * @brief Test for Dmod_DelayUs with small delay
 * 
 * The test checks if the function can delay for a small amount of time.
 */
TEST_F(DmodDelayTest, DelayUsSmall)
{
    uint64_t delayUs = 1000; // 1 millisecond
    
    uint64_t elapsed = measureTime([delayUs]() {
        bool result = Dmod_DelayUs(delayUs);
        ASSERT_TRUE(result);
    });
    
    // Allow 50% tolerance due to system scheduling and timer precision
    ASSERT_GE(elapsed, delayUs / 2);
    // Upper bound should be reasonable (less than 10x expected)
    ASSERT_LE(elapsed, delayUs * 10);
}

/**
 * @brief Test for Dmod_DelayUs with larger delay
 * 
 * The test checks if the function can delay for a larger amount of time.
 */
TEST_F(DmodDelayTest, DelayUsLarge)
{
    uint64_t delayUs = 10000; // 10 milliseconds
    
    uint64_t elapsed = measureTime([delayUs]() {
        bool result = Dmod_DelayUs(delayUs);
        ASSERT_TRUE(result);
    });
    
    // Allow 30% tolerance for larger delays
    ASSERT_GE(elapsed, delayUs * 0.7);
    ASSERT_LE(elapsed, delayUs * 1.5);
}

/**
 * @brief Test for Dmod_SleepMs with zero sleep
 * 
 * The test checks if the function handles zero sleep correctly.
 */
TEST_F(DmodDelayTest, SleepMsZero)
{
    bool result = Dmod_SleepMs(0);
    ASSERT_TRUE(result);
}

/**
 * @brief Test for Dmod_SleepMs with small sleep
 * 
 * The test checks if the function can sleep for a small amount of time.
 */
TEST_F(DmodDelayTest, SleepMsSmall)
{
    uint64_t sleepMs = 10; // 10 milliseconds
    
    uint64_t elapsed = measureTime([sleepMs]() {
        bool result = Dmod_SleepMs(sleepMs);
        ASSERT_TRUE(result);
    });
    
    // Convert to microseconds for comparison
    uint64_t expectedUs = sleepMs * 1000;
    
    // Allow 50% tolerance due to system scheduling
    ASSERT_GE(elapsed, expectedUs / 2);
    ASSERT_LE(elapsed, expectedUs * 3);
}

/**
 * @brief Test for Dmod_SleepMs with larger sleep
 * 
 * The test checks if the function can sleep for a larger amount of time.
 */
TEST_F(DmodDelayTest, SleepMsLarge)
{
    uint64_t sleepMs = 50; // 50 milliseconds
    
    uint64_t elapsed = measureTime([sleepMs]() {
        bool result = Dmod_SleepMs(sleepMs);
        ASSERT_TRUE(result);
    });
    
    // Convert to microseconds for comparison
    uint64_t expectedUs = sleepMs * 1000;
    
    // Allow 30% tolerance for larger sleeps
    ASSERT_GE(elapsed, expectedUs * 0.7);
    ASSERT_LE(elapsed, expectedUs * 1.5);
}

/**
 * @brief Test that DelayUs and SleepMs produce consistent results
 * 
 * The test checks if both functions produce similar delays.
 */
TEST_F(DmodDelayTest, DelayVsSleepConsistency)
{
    uint64_t delayMs = 20;
    
    uint64_t delayElapsed = measureTime([delayMs]() {
        Dmod_DelayUs(delayMs * 1000);
    });
    
    uint64_t sleepElapsed = measureTime([delayMs]() {
        Dmod_SleepMs(delayMs);
    });
    
    // Both should be reasonably close (within 3x of each other)
    // Ensure neither value is zero to avoid division by zero
    ASSERT_GT(delayElapsed, 0u);
    ASSERT_GT(sleepElapsed, 0u);
    
    uint64_t ratio = (delayElapsed > sleepElapsed) ? 
                     (delayElapsed / sleepElapsed) : 
                     (sleepElapsed / delayElapsed);
    
    ASSERT_LE(ratio, 3); // Allow up to 3x difference
}
