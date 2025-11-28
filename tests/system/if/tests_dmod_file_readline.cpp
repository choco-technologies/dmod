#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <string.h>
#include <unistd.h>
#include "dmod_sal.h"

// ===============================================================
//                  Tests for Dmod_FileReadLine
// ===============================================================

class DmodFileReadLineTest : public ::testing::Test
{
protected:
    const char* testFilePath = "/tmp/dmod_test_readline.txt";
    
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
    
    void CreateTestFile(const char* content)
    {
        void* file = Dmod_FileOpen(testFilePath, "w");
        ASSERT_NE(file, nullptr);
        Dmod_FileWrite(content, 1, strlen(content), file);
        Dmod_FileClose(file);
    }
};

/**
 * @brief Test for Dmod_FileReadLine with a single line
 * 
 * The test checks if the function can read a single line from a file.
 */
TEST_F(DmodFileReadLineTest, ReadSingleLine)
{
    const char* testContent = "Hello World\n";
    CreateTestFile(testContent);
    
    void* file = Dmod_FileOpen(testFilePath, "r");
    ASSERT_NE(file, nullptr);
    
    char buffer[256];
    char* result = Dmod_FileReadLine(buffer, sizeof(buffer), file);
    
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result, buffer);
    ASSERT_STREQ(buffer, "Hello World\n");
    
    Dmod_FileClose(file);
}

/**
 * @brief Test for Dmod_FileReadLine with multiple lines
 * 
 * The test checks if the function reads only one line at a time.
 */
TEST_F(DmodFileReadLineTest, ReadMultipleLines)
{
    const char* testContent = "Line 1\nLine 2\nLine 3\n";
    CreateTestFile(testContent);
    
    void* file = Dmod_FileOpen(testFilePath, "r");
    ASSERT_NE(file, nullptr);
    
    char buffer[256];
    
    // Read first line
    char* result1 = Dmod_FileReadLine(buffer, sizeof(buffer), file);
    ASSERT_NE(result1, nullptr);
    ASSERT_STREQ(buffer, "Line 1\n");
    
    // Read second line
    char* result2 = Dmod_FileReadLine(buffer, sizeof(buffer), file);
    ASSERT_NE(result2, nullptr);
    ASSERT_STREQ(buffer, "Line 2\n");
    
    // Read third line
    char* result3 = Dmod_FileReadLine(buffer, sizeof(buffer), file);
    ASSERT_NE(result3, nullptr);
    ASSERT_STREQ(buffer, "Line 3\n");
    
    // Read past end of file
    char* result4 = Dmod_FileReadLine(buffer, sizeof(buffer), file);
    ASSERT_EQ(result4, nullptr);
    
    Dmod_FileClose(file);
}

/**
 * @brief Test for Dmod_FileReadLine with line longer than buffer
 * 
 * The test checks if the function handles lines longer than buffer correctly.
 */
TEST_F(DmodFileReadLineTest, ReadLongLine)
{
    const char* testContent = "This is a very long line that should be truncated\n";
    CreateTestFile(testContent);
    
    void* file = Dmod_FileOpen(testFilePath, "r");
    ASSERT_NE(file, nullptr);
    
    char buffer[20];
    char* result = Dmod_FileReadLine(buffer, sizeof(buffer), file);
    
    ASSERT_NE(result, nullptr);
    // Buffer should contain only 19 chars + null terminator
    ASSERT_EQ(strlen(buffer), 19);
    ASSERT_STREQ(buffer, "This is a very long");
    
    Dmod_FileClose(file);
}

/**
 * @brief Test for Dmod_FileReadLine with NULL buffer
 * 
 * The test checks if the function handles NULL buffer correctly.
 */
TEST_F(DmodFileReadLineTest, NullBuffer)
{
    const char* testContent = "Test\n";
    CreateTestFile(testContent);
    
    void* file = Dmod_FileOpen(testFilePath, "r");
    ASSERT_NE(file, nullptr);
    
    char* result = Dmod_FileReadLine(NULL, 256, file);
    
    ASSERT_EQ(result, nullptr);
    
    Dmod_FileClose(file);
}

/**
 * @brief Test for Dmod_FileReadLine with NULL file
 * 
 * The test checks if the function handles NULL file handle correctly.
 */
TEST_F(DmodFileReadLineTest, NullFile)
{
    char buffer[256];
    char* result = Dmod_FileReadLine(buffer, sizeof(buffer), NULL);
    
    ASSERT_EQ(result, nullptr);
}

/**
 * @brief Test for Dmod_FileReadLine with zero size
 * 
 * The test checks if the function handles zero size correctly.
 */
TEST_F(DmodFileReadLineTest, ZeroSize)
{
    const char* testContent = "Test\n";
    CreateTestFile(testContent);
    
    void* file = Dmod_FileOpen(testFilePath, "r");
    ASSERT_NE(file, nullptr);
    
    char buffer[256];
    char* result = Dmod_FileReadLine(buffer, 0, file);
    
    ASSERT_EQ(result, nullptr);
    
    Dmod_FileClose(file);
}

/**
 * @brief Test for Dmod_FileReadLine with negative size
 * 
 * The test checks if the function handles negative size correctly.
 */
TEST_F(DmodFileReadLineTest, NegativeSize)
{
    const char* testContent = "Test\n";
    CreateTestFile(testContent);
    
    void* file = Dmod_FileOpen(testFilePath, "r");
    ASSERT_NE(file, nullptr);
    
    char buffer[256];
    char* result = Dmod_FileReadLine(buffer, -1, file);
    
    ASSERT_EQ(result, nullptr);
    
    Dmod_FileClose(file);
}

/**
 * @brief Test for Dmod_FileReadLine with line without newline
 * 
 * The test checks if the function handles lines without trailing newline.
 */
TEST_F(DmodFileReadLineTest, LineWithoutNewline)
{
    const char* testContent = "No newline at end";
    CreateTestFile(testContent);
    
    void* file = Dmod_FileOpen(testFilePath, "r");
    ASSERT_NE(file, nullptr);
    
    char buffer[256];
    char* result = Dmod_FileReadLine(buffer, sizeof(buffer), file);
    
    ASSERT_NE(result, nullptr);
    ASSERT_STREQ(buffer, "No newline at end");
    
    // Next read should return NULL (EOF)
    char* result2 = Dmod_FileReadLine(buffer, sizeof(buffer), file);
    ASSERT_EQ(result2, nullptr);
    
    Dmod_FileClose(file);
}

/**
 * @brief Test for Dmod_FileReadLine with empty file
 * 
 * The test checks if the function handles empty file correctly.
 */
TEST_F(DmodFileReadLineTest, EmptyFile)
{
    CreateTestFile("");
    
    void* file = Dmod_FileOpen(testFilePath, "r");
    ASSERT_NE(file, nullptr);
    
    char buffer[256];
    char* result = Dmod_FileReadLine(buffer, sizeof(buffer), file);
    
    ASSERT_EQ(result, nullptr);
    
    Dmod_FileClose(file);
}

/**
 * @brief Test for Dmod_FileReadLine with empty lines
 * 
 * The test checks if the function handles empty lines correctly.
 */
TEST_F(DmodFileReadLineTest, EmptyLines)
{
    const char* testContent = "\n\n\n";
    CreateTestFile(testContent);
    
    void* file = Dmod_FileOpen(testFilePath, "r");
    ASSERT_NE(file, nullptr);
    
    char buffer[256];
    
    // Read first empty line
    char* result1 = Dmod_FileReadLine(buffer, sizeof(buffer), file);
    ASSERT_NE(result1, nullptr);
    ASSERT_STREQ(buffer, "\n");
    
    // Read second empty line
    char* result2 = Dmod_FileReadLine(buffer, sizeof(buffer), file);
    ASSERT_NE(result2, nullptr);
    ASSERT_STREQ(buffer, "\n");
    
    // Read third empty line
    char* result3 = Dmod_FileReadLine(buffer, sizeof(buffer), file);
    ASSERT_NE(result3, nullptr);
    ASSERT_STREQ(buffer, "\n");
    
    // Read past end of file
    char* result4 = Dmod_FileReadLine(buffer, sizeof(buffer), file);
    ASSERT_EQ(result4, nullptr);
    
    Dmod_FileClose(file);
}
