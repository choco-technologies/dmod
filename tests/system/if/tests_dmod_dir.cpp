#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <string.h>
#include <unistd.h>
#include "dmod_sal.h"

// ===============================================================
//                  Tests for Directory Operations
// ===============================================================

class DmodDirTest : public ::testing::Test
{
protected:
    const char* testDirPath = "/tmp/dmod_test_dir";
    
    void SetUp() override
    {
        // Clean up any existing test directory
        rmdir(testDirPath);
    }

    void TearDown() override
    {
        // Clean up test directory
        rmdir(testDirPath);
    }
};

/**
 * @brief Test for Dmod_MakeDir
 * 
 * The test checks if the function can create a directory.
 */
TEST_F(DmodDirTest, MakeDir)
{
    int result = Dmod_MakeDir(testDirPath, 0755);
    
    ASSERT_EQ(result, 0);
    
    // Verify directory exists by trying to open it
    void* dir = Dmod_OpenDir(testDirPath);
    ASSERT_NE(dir, nullptr);
    Dmod_CloseDir(dir);
}

/**
 * @brief Test for Dmod_OpenDir with valid path
 * 
 * The test checks if the function can open an existing directory.
 */
TEST_F(DmodDirTest, OpenDirValid)
{
    // Create test directory first
    Dmod_MakeDir(testDirPath, 0755);
    
    void* dir = Dmod_OpenDir(testDirPath);
    
    ASSERT_NE(dir, nullptr);
    Dmod_CloseDir(dir);
}

/**
 * @brief Test for Dmod_OpenDir with invalid path
 * 
 * The test checks if the function handles invalid paths correctly.
 */
TEST_F(DmodDirTest, OpenDirInvalid)
{
    void* dir = Dmod_OpenDir("/non/existent/path/xyz123");
    
    ASSERT_EQ(dir, nullptr);
}

/**
 * @brief Test for Dmod_ReadDir
 * 
 * The test checks if the function can read directory entries.
 */
TEST_F(DmodDirTest, ReadDir)
{
    // Use current directory which should have some entries
    void* dir = Dmod_OpenDir(".");
    
    ASSERT_NE(dir, nullptr);
    
    // Read at least one entry
    const char* entry = Dmod_ReadDir(dir);
    ASSERT_NE(entry, nullptr);
    
    // Entry name should not be empty
    ASSERT_GT(strlen(entry), 0);
    
    Dmod_CloseDir(dir);
}

/**
 * @brief Test for Dmod_ReadDir reaching end
 * 
 * The test checks if the function returns NULL when all entries are read.
 */
TEST_F(DmodDirTest, ReadDirEnd)
{
    // Create empty test directory
    Dmod_MakeDir(testDirPath, 0755);
    
    void* dir = Dmod_OpenDir(testDirPath);
    ASSERT_NE(dir, nullptr);
    
    // Read all entries (should be . and .. at minimum)
    int count = 0;
    const char* entry;
    while ((entry = Dmod_ReadDir(dir)) != NULL && count < 100)
    {
        count++;
    }
    
    // Should have read at least . and ..
    ASSERT_GE(count, 2);
    
    // Reading past the end should return NULL
    ASSERT_EQ(Dmod_ReadDir(dir), nullptr);
    
    Dmod_CloseDir(dir);
}

/**
 * @brief Test for Dmod_CloseDir
 * 
 * The test checks if the function can close a directory handle.
 */
TEST_F(DmodDirTest, CloseDir)
{
    void* dir = Dmod_OpenDir(".");
    ASSERT_NE(dir, nullptr);
    
    // CloseDir should not crash
    Dmod_CloseDir(dir);
    
    // This test passes if no crash occurs
    ASSERT_TRUE(true);
}

/**
 * @brief Test for full directory iteration workflow
 * 
 * The test checks the complete workflow of opening, reading, and closing a directory.
 */
TEST_F(DmodDirTest, FullWorkflow)
{
    // Create test directory
    int result = Dmod_MakeDir(testDirPath, 0755);
    ASSERT_EQ(result, 0);
    
    // Open directory
    void* dir = Dmod_OpenDir(testDirPath);
    ASSERT_NE(dir, nullptr);
    
    // Count entries
    int entryCount = 0;
    const char* entry;
    while ((entry = Dmod_ReadDir(dir)) != NULL)
    {
        entryCount++;
    }
    
    // Should have at least . and ..
    ASSERT_GE(entryCount, 2);
    
    // Close directory
    Dmod_CloseDir(dir);
    
    ASSERT_TRUE(true);
}
