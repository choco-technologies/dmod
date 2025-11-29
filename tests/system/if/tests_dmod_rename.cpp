#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "dmod_sal.h"

// ===============================================================
//                  Tests for Dmod_Rename
// ===============================================================

class DmodRenameTest : public ::testing::Test
{
protected:
    const char* testFilePath = "/tmp/dmod_test_rename_old.txt";
    const char* testNewPath = "/tmp/dmod_test_rename_new.txt";
    
    void SetUp() override
    {
        // Clean up any existing test files
        remove(testFilePath);
        remove(testNewPath);
    }

    void TearDown() override
    {
        // Clean up test files
        remove(testFilePath);
        remove(testNewPath);
    }
    
    void CreateTestFile(const char* path, const char* content)
    {
        void* file = Dmod_FileOpen(path, "wb");
        if (file != NULL)
        {
            Dmod_FileWrite(content, 1, strlen(content), file);
            Dmod_FileClose(file);
        }
    }
    
    bool FileExists(const char* path)
    {
        void* file = Dmod_FileOpen(path, "rb");
        if (file != NULL)
        {
            Dmod_FileClose(file);
            return true;
        }
        return false;
    }
};

/**
 * @brief Test for Dmod_Rename - successful rename
 * 
 * The test checks if the function can rename a file.
 */
TEST_F(DmodRenameTest, RenameSuccess)
{
    // Create test file
    const char* content = "test content";
    CreateTestFile(testFilePath, content);
    
    // Verify old file exists
    ASSERT_TRUE(FileExists(testFilePath));
    ASSERT_FALSE(FileExists(testNewPath));
    
    // Rename the file
    int result = Dmod_Rename(testFilePath, testNewPath);
    
    ASSERT_EQ(result, 0);
    
    // Verify old file no longer exists and new file exists
    ASSERT_FALSE(FileExists(testFilePath));
    ASSERT_TRUE(FileExists(testNewPath));
}

/**
 * @brief Test for Dmod_Rename - non-existent source file
 * 
 * The test checks if the function handles non-existent source file correctly.
 */
TEST_F(DmodRenameTest, RenameNonExistent)
{
    // Try to rename a non-existent file
    int result = Dmod_Rename("/tmp/non_existent_file_xyz123.txt", testNewPath);
    
    ASSERT_NE(result, 0);
}

/**
 * @brief Test for Dmod_Rename - preserves content
 * 
 * The test checks if the function preserves file content after rename.
 */
TEST_F(DmodRenameTest, RenamePreservesContent)
{
    // Create test file with specific content
    const char* content = "Hello, DMOD World!";
    CreateTestFile(testFilePath, content);
    
    // Rename the file
    int result = Dmod_Rename(testFilePath, testNewPath);
    ASSERT_EQ(result, 0);
    
    // Read content from the new file
    void* file = Dmod_FileOpen(testNewPath, "rb");
    ASSERT_NE(file, nullptr);
    
    char buffer[256] = {0};
    size_t bytesRead = Dmod_FileRead(buffer, 1, sizeof(buffer) - 1, file);
    Dmod_FileClose(file);
    
    ASSERT_EQ(bytesRead, strlen(content));
    ASSERT_STREQ(buffer, content);
}

/**
 * @brief Test for Dmod_Rename - rename to existing file (overwrite behavior)
 * 
 * On POSIX systems, rename should atomically replace the target if it exists.
 */
TEST_F(DmodRenameTest, RenameOverwrite)
{
    // Create source file
    const char* sourceContent = "source content";
    CreateTestFile(testFilePath, sourceContent);
    
    // Create target file
    const char* targetContent = "target content";
    CreateTestFile(testNewPath, targetContent);
    
    // Rename source to target (should overwrite)
    int result = Dmod_Rename(testFilePath, testNewPath);
    ASSERT_EQ(result, 0);
    
    // Verify only new file exists with source content
    ASSERT_FALSE(FileExists(testFilePath));
    ASSERT_TRUE(FileExists(testNewPath));
    
    // Read content from the new file
    void* file = Dmod_FileOpen(testNewPath, "rb");
    ASSERT_NE(file, nullptr);
    
    char buffer[256] = {0};
    size_t bytesRead = Dmod_FileRead(buffer, 1, sizeof(buffer) - 1, file);
    Dmod_FileClose(file);
    
    ASSERT_EQ(bytesRead, strlen(sourceContent));
    ASSERT_STREQ(buffer, sourceContent);
}
