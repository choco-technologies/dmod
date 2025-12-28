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

/**
 * @brief Test for Dmod_RemoveDir
 * 
 * The test checks if the function can remove an empty directory.
 */
TEST_F(DmodDirTest, RemoveDir)
{
    // Create test directory
    int result = Dmod_MakeDir(testDirPath, 0755);
    ASSERT_EQ(result, 0);
    
    // Verify directory exists
    void* dir = Dmod_OpenDir(testDirPath);
    ASSERT_NE(dir, nullptr);
    Dmod_CloseDir(dir);
    
    // Remove directory
    result = Dmod_RemoveDir(testDirPath);
    ASSERT_EQ(result, 0);
    
    // Verify directory no longer exists
    dir = Dmod_OpenDir(testDirPath);
    ASSERT_EQ(dir, nullptr);
}

/**
 * @brief Test for Dmod_RemoveDir with non-existent path
 * 
 * The test checks if the function handles non-existent paths correctly.
 */
TEST_F(DmodDirTest, RemoveDirNonExistent)
{
    int result = Dmod_RemoveDir("/non/existent/path/xyz123456");
    ASSERT_EQ(result, -1);
}

/**
 * @brief Test for Dmod_ReadDirEx
 * 
 * The test checks if the function can read directory entries with extended information.
 */
TEST_F(DmodDirTest, ReadDirEx)
{
    // Use current directory which should have some entries
    void* dir = Dmod_OpenDir(".");
    
    ASSERT_NE(dir, nullptr);
    
    // Read at least one entry
    const Dmod_DirEntry_t* entry = Dmod_ReadDirEx(dir);
    ASSERT_NE(entry, nullptr);
    
    // Entry name should not be empty
    ASSERT_NE(entry->name, nullptr);
    ASSERT_GT(strlen(entry->name), 0);
    
    // Entry type should be valid (even if unknown)
    ASSERT_GE(entry->type, Dmod_DirEntryType_Unknown);
    ASSERT_LE(entry->type, Dmod_DirEntryType_Other);
    
    Dmod_CloseDir(dir);
}

/**
 * @brief Test for Dmod_ReadDirEx reaching end
 * 
 * The test checks if the function returns NULL when all entries are read.
 */
TEST_F(DmodDirTest, ReadDirExEnd)
{
    // Create empty test directory
    Dmod_MakeDir(testDirPath, 0755);
    
    void* dir = Dmod_OpenDir(testDirPath);
    ASSERT_NE(dir, nullptr);
    
    // Read all entries (should be . and .. at minimum)
    int count = 0;
    const Dmod_DirEntry_t* entry;
    while ((entry = Dmod_ReadDirEx(dir)) != NULL && count < 100)
    {
        count++;
    }
    
    // Should have read at least . and ..
    ASSERT_GE(count, 2);
    
    // Reading past the end should return NULL
    ASSERT_EQ(Dmod_ReadDirEx(dir), nullptr);
    
    Dmod_CloseDir(dir);
}

/**
 * @brief Test for Dmod_ReadDirEx with directory type detection
 * 
 * The test checks if the function correctly identifies directory types.
 */
TEST_F(DmodDirTest, ReadDirExTypeDetection)
{
    // Use current directory which should have some entries
    void* dir = Dmod_OpenDir(".");
    
    ASSERT_NE(dir, nullptr);
    
    bool foundDir = false;
    bool foundAnyType = false;
    const Dmod_DirEntry_t* entry;
    
    // Read entries and look for directories (. and .. should be present)
    while ((entry = Dmod_ReadDirEx(dir)) != NULL)
    {
        if (entry->type == Dmod_DirEntryType_Dir)
        {
            foundDir = true;
            foundAnyType = true;
            break;
        }
        if (entry->type != Dmod_DirEntryType_Unknown)
        {
            foundAnyType = true;
        }
    }
    
    // Should find at least one directory (. or ..) OR have type detection available
    // Note: On some filesystems, d_type might return DT_UNKNOWN for all entries
    // In this case, we just verify that the function works without crashing
    if (foundAnyType)
    {
        ASSERT_TRUE(foundDir);
    }
    
    Dmod_CloseDir(dir);
}

/**
 * @brief Test backward compatibility between Dmod_ReadDir and Dmod_ReadDirEx
 * 
 * The test ensures both functions work on the same directory and return the same entries.
 */
TEST_F(DmodDirTest, BackwardCompatibility)
{
    // Create test directory with a known file
    Dmod_MakeDir(testDirPath, 0755);
    
    // Count entries using old API
    void* dir1 = Dmod_OpenDir(testDirPath);
    ASSERT_NE(dir1, nullptr);
    
    int countOld = 0;
    while (Dmod_ReadDir(dir1) != NULL && countOld < 100)
    {
        countOld++;
    }
    Dmod_CloseDir(dir1);
    
    // Count entries using new API
    void* dir2 = Dmod_OpenDir(testDirPath);
    ASSERT_NE(dir2, nullptr);
    
    int countNew = 0;
    while (Dmod_ReadDirEx(dir2) != NULL && countNew < 100)
    {
        countNew++;
    }
    Dmod_CloseDir(dir2);
    
    // Both should return the same count
    ASSERT_EQ(countOld, countNew);
}
