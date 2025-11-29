#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <string.h>
#include <unistd.h>
#include "dmod_sal.h"

// ===============================================================
//                  Tests for Dmod_FileRemove
// ===============================================================

class DmodFileRemoveTest : public ::testing::Test
{
protected:
    const char* testFilePath = "/tmp/dmod_test_file_remove.txt";
    
    void SetUp() override
    {
        // Clean up any existing test file
        unlink(testFilePath);
    }

    void TearDown() override
    {
        // Clean up test file
        unlink(testFilePath);
    }
    
    void CreateTestFile()
    {
        void* file = Dmod_FileOpen(testFilePath, "w");
        if (file)
        {
            Dmod_FileWrite("test content", 1, 12, file);
            Dmod_FileClose(file);
        }
    }
};

/**
 * @brief Test for Dmod_FileRemove with existing file
 * 
 * The test checks if the function can remove an existing file.
 */
TEST_F(DmodFileRemoveTest, RemoveExistingFile)
{
    // Create test file
    CreateTestFile();
    
    // Verify file exists
    ASSERT_TRUE(Dmod_FileAvailable(testFilePath));
    
    // Remove file
    int result = Dmod_FileRemove(testFilePath);
    ASSERT_EQ(result, 0);
    
    // Verify file no longer exists
    ASSERT_FALSE(Dmod_FileAvailable(testFilePath));
}

/**
 * @brief Test for Dmod_FileRemove with non-existent file
 * 
 * The test checks if the function handles non-existent files correctly.
 */
TEST_F(DmodFileRemoveTest, RemoveNonExistentFile)
{
    int result = Dmod_FileRemove("/non/existent/path/xyz123456.txt");
    ASSERT_EQ(result, -1);
}

/**
 * @brief Test for Dmod_FileRemove workflow
 * 
 * The test checks the complete workflow of creating and removing a file.
 */
TEST_F(DmodFileRemoveTest, FullWorkflow)
{
    // Create file
    void* file = Dmod_FileOpen(testFilePath, "w");
    ASSERT_NE(file, nullptr);
    
    const char* content = "Hello, DMOD!";
    size_t written = Dmod_FileWrite(content, 1, strlen(content), file);
    ASSERT_EQ(written, strlen(content));
    
    Dmod_FileClose(file);
    
    // Verify file exists
    ASSERT_TRUE(Dmod_FileAvailable(testFilePath));
    
    // Remove file
    int result = Dmod_FileRemove(testFilePath);
    ASSERT_EQ(result, 0);
    
    // Verify file is gone
    ASSERT_FALSE(Dmod_FileAvailable(testFilePath));
}
