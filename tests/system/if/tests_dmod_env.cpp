#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <string.h>
#include "dmod_sal.h"

// ===============================================================
//                  Tests for Dmod_GetEnv and Dmod_SetEnv
// ===============================================================

class DmodEnvTest : public ::testing::Test
{
};

/**
 * @brief Test for Dmod_SetEnv with a new environment variable
 * 
 * The test checks if the function can set a new environment variable.
 */
TEST_F(DmodEnvTest, SetEnvNewVariable)
{
    const char* testName = "DMOD_TEST_VAR_NEW";
    const char* testValue = "test_value";
    
    int result = Dmod_SetEnv(testName, testValue, 1);
    
    ASSERT_EQ(result, 0); // Should succeed
    
    const char* value = Dmod_GetEnv(testName);
    ASSERT_NE(value, nullptr);
    ASSERT_STREQ(value, testValue);
}

/**
 * @brief Test for Dmod_SetEnv with overwrite enabled
 * 
 * The test checks if the function can overwrite an existing environment variable.
 */
TEST_F(DmodEnvTest, SetEnvOverwriteEnabled)
{
    const char* testName = "DMOD_TEST_VAR_OVERWRITE";
    const char* initialValue = "initial";
    const char* newValue = "overwritten";
    
    // Set initial value
    int result1 = Dmod_SetEnv(testName, initialValue, 1);
    ASSERT_EQ(result1, 0);
    
    const char* value1 = Dmod_GetEnv(testName);
    ASSERT_NE(value1, nullptr);
    ASSERT_STREQ(value1, initialValue);
    
    // Overwrite with new value
    int result2 = Dmod_SetEnv(testName, newValue, 1);
    ASSERT_EQ(result2, 0);
    
    const char* value2 = Dmod_GetEnv(testName);
    ASSERT_NE(value2, nullptr);
    ASSERT_STREQ(value2, newValue);
}

/**
 * @brief Test for Dmod_SetEnv with overwrite disabled
 * 
 * The test checks if the function preserves the existing value when overwrite is disabled.
 */
TEST_F(DmodEnvTest, SetEnvOverwriteDisabled)
{
    const char* testName = "DMOD_TEST_VAR_NO_OVERWRITE";
    const char* initialValue = "keep_this";
    const char* newValue = "should_not_set";
    
    // Set initial value
    int result1 = Dmod_SetEnv(testName, initialValue, 1);
    ASSERT_EQ(result1, 0);
    
    const char* value1 = Dmod_GetEnv(testName);
    ASSERT_NE(value1, nullptr);
    ASSERT_STREQ(value1, initialValue);
    
    // Try to overwrite with overwrite=0
    int result2 = Dmod_SetEnv(testName, newValue, 0);
    ASSERT_EQ(result2, 0); // Should succeed
    
    // Value should remain unchanged
    const char* value2 = Dmod_GetEnv(testName);
    ASSERT_NE(value2, nullptr);
    ASSERT_STREQ(value2, initialValue);
}

/**
 * @brief Test for Dmod_GetEnv with non-existent variable
 * 
 * The test checks if the function returns NULL for non-existent variables.
 */
TEST_F(DmodEnvTest, GetEnvNonExistent)
{
    const char* value = Dmod_GetEnv("DMOD_TEST_VAR_NONEXISTENT_12345");
    ASSERT_EQ(value, nullptr);
}

/**
 * @brief Test for Dmod_GetEnv with DMOD_REPO_DIR
 * 
 * The test checks if the function returns the default DMOD_REPO_DIR value.
 */
TEST_F(DmodEnvTest, GetEnvDmodRepoDir)
{
    const char* value = Dmod_GetEnv("DMOD_REPO_DIR");
    ASSERT_NE(value, nullptr);
    // We just check that it's not null, as the actual value depends on configuration
}

/**
 * @brief Test for Dmod_SetEnv with empty value
 * 
 * The test checks if the function can set an empty value.
 */
TEST_F(DmodEnvTest, SetEnvEmptyValue)
{
    const char* testName = "DMOD_TEST_VAR_EMPTY";
    const char* emptyValue = "";
    
    int result = Dmod_SetEnv(testName, emptyValue, 1);
    ASSERT_EQ(result, 0);
    
    const char* value = Dmod_GetEnv(testName);
    ASSERT_NE(value, nullptr);
    ASSERT_STREQ(value, emptyValue);
}

/**
 * @brief Test for Dmod_Unsetenv with existing variable
 * 
 * The test checks if the function can unset an existing environment variable.
 */
TEST_F(DmodEnvTest, UnsetenvExistingVariable)
{
    const char* testName = "DMOD_TEST_VAR_UNSET";
    const char* testValue = "to_be_removed";
    
    // Set the variable first
    int result = Dmod_SetEnv(testName, testValue, 1);
    ASSERT_EQ(result, 0);
    
    // Verify it exists
    const char* value = Dmod_GetEnv(testName);
    ASSERT_NE(value, nullptr);
    ASSERT_STREQ(value, testValue);
    
    // Unset the variable
    result = Dmod_Unsetenv(testName);
    ASSERT_EQ(result, 0);
    
    // Verify it's gone
    value = Dmod_GetEnv(testName);
    ASSERT_EQ(value, nullptr);
}

/**
 * @brief Test for Dmod_Unsetenv with non-existent variable
 * 
 * The test checks if the function handles non-existent variables correctly.
 */
TEST_F(DmodEnvTest, UnsetenvNonExistent)
{
    // Unsetting a non-existent variable should succeed (POSIX behavior)
    int result = Dmod_Unsetenv("DMOD_TEST_VAR_DOES_NOT_EXIST_XYZ123");
    ASSERT_EQ(result, 0);
}

/**
 * @brief Test for Dmod_GetNextEnvName iteration
 * 
 * The test checks if the function can iterate through environment variables.
 */
TEST_F(DmodEnvTest, GetNextEnvNameIteration)
{
    // Get the first environment variable
    const char* first = Dmod_GetNextEnvName(NULL);
    ASSERT_NE(first, nullptr);
    
    // Should have at least one more environment variable
    const char* second = Dmod_GetNextEnvName(first);
    // Second could be null if there's only one env var, but typically there are more
    if (second != nullptr)
    {
        // Names should be different
        ASSERT_STRNE(first, second);
    }
}

/**
 * @brief Test for Dmod_GetNextEnvName with custom variable
 * 
 * The test verifies that a newly set variable appears in the iteration.
 */
TEST_F(DmodEnvTest, GetNextEnvNameFindsCustomVar)
{
    const char* testName = "DMOD_TEST_ITERATE_VAR";
    const char* testValue = "iterate_value";
    
    // Set a custom variable
    int result = Dmod_SetEnv(testName, testValue, 1);
    ASSERT_EQ(result, 0);
    
    // Iterate and look for our variable
    bool found = false;
    const char* envName = Dmod_GetNextEnvName(NULL);
    int count = 0;
    const int maxIterations = 10000; // Safety limit
    
    while (envName != NULL && count < maxIterations)
    {
        if (strcmp(envName, testName) == 0)
        {
            found = true;
            break;
        }
        envName = Dmod_GetNextEnvName(envName);
        count++;
    }
    
    ASSERT_TRUE(found);
    
    // Clean up
    Dmod_Unsetenv(testName);
}
