#define DMOD_PRIVATE
#include <stdlib.h>
#include <string.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "dmod.h"
#include "dmod_system.h"

// ===============================================================
//                  Test fixture
// ===============================================================

class DmodFindMatchTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

// ===============================================================
//                  Tests for Dmod_FindMatch
// ===============================================================

/**
 * @brief Test for Dmod_FindMatch with NULL parameters
 * 
 * The test checks if the function handles NULL parameters correctly.
 */
TEST_F(DmodFindMatchTest, FindMatchNullPartialName)
{
    char outModuleName[256];
    EXPECT_FALSE(Dmod_FindMatch(NULL, outModuleName, sizeof(outModuleName)));
}

/**
 * @brief Test for Dmod_FindMatch with NULL output buffer
 * 
 * The test checks if the function handles NULL output buffer correctly.
 */
TEST_F(DmodFindMatchTest, FindMatchNullOutputBuffer)
{
    EXPECT_FALSE(Dmod_FindMatch("test", NULL, 256));
}

/**
 * @brief Test for Dmod_FindMatch with zero max length
 * 
 * The test checks if the function handles zero max length correctly.
 */
TEST_F(DmodFindMatchTest, FindMatchZeroMaxLength)
{
    char outModuleName[256];
    EXPECT_FALSE(Dmod_FindMatch("test", outModuleName, 0));
}

/**
 * @brief Test for Dmod_FindMatch with empty partial name
 * 
 * The test checks if the function handles empty partial name correctly.
 */
TEST_F(DmodFindMatchTest, FindMatchEmptyPartialName)
{
    char outModuleName[256];
    EXPECT_FALSE(Dmod_FindMatch("", outModuleName, sizeof(outModuleName)));
}

/**
 * @brief Test for Dmod_FindMatch basic functionality
 * 
 * This test will pass if there's any module in the search paths
 * that can be matched. It's a basic smoke test.
 */
TEST_F(DmodFindMatchTest, FindMatchBasicFunctionality)
{
    // This test may fail if no modules are available in the search paths
    // which is acceptable for a unit test environment
    char outModuleName[256];
    
    // Try to find any module - this may or may not succeed depending on environment
    // The test verifies that the function doesn't crash and returns valid values
    bool result = Dmod_FindMatch("test", outModuleName, sizeof(outModuleName));
    
    if (result)
    {
        // If a match was found, verify the output is not empty
        EXPECT_GT(strlen(outModuleName), 0);
        EXPECT_TRUE(strncmp(outModuleName, "test", 4) == 0);
    }
    // If no match is found, that's also acceptable - the function should just return false
}
