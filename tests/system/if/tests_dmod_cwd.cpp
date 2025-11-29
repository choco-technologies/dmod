#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <string.h>
#include <unistd.h>
#include "dmod_sal.h"

// ===============================================================
//                  Tests for Dmod_ChDir and Dmod_GetCwd
// ===============================================================

class DmodCwdTest : public ::testing::Test
{
protected:
    char originalCwd[DMOD_MAX_PATH_LENGTH];
    const char* testDirPath = "/tmp/dmod_test_chdir";
    
    void SetUp() override
    {
        // Save original working directory
        Dmod_GetCwd(originalCwd, sizeof(originalCwd));
        
        // Clean up any existing test directory and recreate it
        rmdir(testDirPath);
        Dmod_MakeDir(testDirPath, 0755);
    }

    void TearDown() override
    {
        // Restore original working directory
        Dmod_ChDir(originalCwd);
        
        // Clean up test directory
        rmdir(testDirPath);
    }
};

/**
 * @brief Test for Dmod_GetCwd
 * 
 * The test checks if the function can get the current working directory.
 */
TEST_F(DmodCwdTest, GetCwd)
{
    char buffer[DMOD_MAX_PATH_LENGTH];
    
    char* result = Dmod_GetCwd(buffer, sizeof(buffer));
    
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result, buffer);
    ASSERT_GT(strlen(buffer), 0);
}

/**
 * @brief Test for Dmod_GetCwd with NULL buffer
 * 
 * The test checks if the function handles NULL buffer correctly.
 */
TEST_F(DmodCwdTest, GetCwdNullBuffer)
{
    char* result = Dmod_GetCwd(nullptr, 0);
    
    // When buffer is NULL, getcwd behavior varies by implementation
    // On some systems it returns NULL, on others it allocates memory
    // Just verify it doesn't crash
    ASSERT_TRUE(true);
    (void)result;
}

/**
 * @brief Test for Dmod_ChDir with valid path
 * 
 * The test checks if the function can change to a valid directory.
 */
TEST_F(DmodCwdTest, ChDirValid)
{
    int result = Dmod_ChDir(testDirPath);
    
    ASSERT_EQ(result, 0);
    
    // Verify we are now in the test directory
    char buffer[DMOD_MAX_PATH_LENGTH];
    Dmod_GetCwd(buffer, sizeof(buffer));
    ASSERT_STREQ(buffer, testDirPath);
}

/**
 * @brief Test for Dmod_ChDir with invalid path
 * 
 * The test checks if the function handles invalid paths correctly.
 */
TEST_F(DmodCwdTest, ChDirInvalid)
{
    int result = Dmod_ChDir("/non/existent/path/xyz123");
    
    ASSERT_NE(result, 0);
}

/**
 * @brief Test for Dmod_ChDir round-trip
 * 
 * The test checks if we can change directory and return to original.
 */
TEST_F(DmodCwdTest, ChDirRoundTrip)
{
    char buffer[DMOD_MAX_PATH_LENGTH];
    
    // Get original directory
    char* originalDir = Dmod_GetCwd(buffer, sizeof(buffer));
    ASSERT_NE(originalDir, nullptr);
    
    // Change to test directory
    int result1 = Dmod_ChDir(testDirPath);
    ASSERT_EQ(result1, 0);
    
    // Verify we changed
    char newBuffer[DMOD_MAX_PATH_LENGTH];
    Dmod_GetCwd(newBuffer, sizeof(newBuffer));
    ASSERT_STREQ(newBuffer, testDirPath);
    
    // Change back to original
    int result2 = Dmod_ChDir(originalDir);
    ASSERT_EQ(result2, 0);
    
    // Verify we're back
    char finalBuffer[DMOD_MAX_PATH_LENGTH];
    Dmod_GetCwd(finalBuffer, sizeof(finalBuffer));
    ASSERT_STREQ(finalBuffer, originalDir);
}

// ===============================================================
//                  Tests for Dmod_EnvCtx_Push and Dmod_EnvCtx_Pop
// ===============================================================

class DmodEnvCtxTest : public ::testing::Test
{
};

/**
 * @brief Test for Dmod_EnvCtx_Push
 * 
 * The test checks if the function returns success (default empty implementation).
 */
TEST_F(DmodEnvCtxTest, EnvCtxPush)
{
    int result = Dmod_EnvCtx_Push();
    
    // Default implementation should return 0 (success)
    ASSERT_EQ(result, 0);
}

/**
 * @brief Test for Dmod_EnvCtx_Pop
 * 
 * The test checks if the function returns success (default empty implementation).
 */
TEST_F(DmodEnvCtxTest, EnvCtxPop)
{
    int result = Dmod_EnvCtx_Pop();
    
    // Default implementation should return 0 (success)
    ASSERT_EQ(result, 0);
}

/**
 * @brief Test for Dmod_EnvCtx_Push and Pop sequence
 * 
 * The test checks if the push/pop sequence works correctly.
 */
TEST_F(DmodEnvCtxTest, EnvCtxPushPopSequence)
{
    // Multiple pushes and pops should work without errors
    int result1 = Dmod_EnvCtx_Push();
    ASSERT_EQ(result1, 0);
    
    int result2 = Dmod_EnvCtx_Push();
    ASSERT_EQ(result2, 0);
    
    int result3 = Dmod_EnvCtx_Pop();
    ASSERT_EQ(result3, 0);
    
    int result4 = Dmod_EnvCtx_Pop();
    ASSERT_EQ(result4, 0);
}
