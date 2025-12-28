#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <string.h>
#include "dmod_sal.h"

// ===============================================================
//                  Tests for Dmod_SnPrintf and Dmod_VSnPrintf
// ===============================================================

class DmodSnPrintfTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

/**
 * @brief Test for Dmod_SnPrintf with normal buffer
 * 
 * The test checks if the function can format a string into a buffer.
 */
TEST_F(DmodSnPrintfTest, SnPrintfNormalBuffer)
{
    char buffer[32];
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "Hello %s!", "World");
    
    ASSERT_EQ(result, 12); // "Hello World!" is 12 characters
    ASSERT_STREQ(buffer, "Hello World!");
}

/**
 * @brief Test for Dmod_SnPrintf with NULL buffer
 * 
 * The test checks if the function returns the required buffer size when Buffer is NULL.
 */
TEST_F(DmodSnPrintfTest, SnPrintfNullBuffer)
{
    int result = Dmod_SnPrintf(NULL, 0, "Hello %s!", "World");
    
    ASSERT_EQ(result, 12); // Should return the required size
}

/**
 * @brief Test for Dmod_SnPrintf with buffer too small
 * 
 * The test checks if the function truncates properly when the buffer is too small.
 */
TEST_F(DmodSnPrintfTest, SnPrintfSmallBuffer)
{
    char buffer[8];
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "Hello %s!", "World");
    
    ASSERT_EQ(result, 12); // Should return the number that would have been written
    ASSERT_EQ(strlen(buffer), 7); // Buffer should contain 7 characters + null terminator
}

/**
 * @brief Test for Dmod_VSnPrintf with normal buffer
 * 
 * The test checks if the function can format a string with va_list into a buffer.
 */
static int TestVSnPrintfHelper(char* buffer, size_t size, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int result = Dmod_VSnPrintf(buffer, size, format, args);
    va_end(args);
    return result;
}

TEST_F(DmodSnPrintfTest, VSnPrintfNormalBuffer)
{
    char buffer[32];
    int result = TestVSnPrintfHelper(buffer, sizeof(buffer), "Number: %d", 42);
    
    ASSERT_EQ(result, 10); // "Number: 42" is 10 characters
    ASSERT_STREQ(buffer, "Number: 42");
}

/**
 * @brief Test for Dmod_VSnPrintf with NULL buffer
 * 
 * The test checks if the function returns the required buffer size when Buffer is NULL.
 */
TEST_F(DmodSnPrintfTest, VSnPrintfNullBuffer)
{
    int result = TestVSnPrintfHelper(NULL, 0, "Number: %d", 42);
    
    ASSERT_EQ(result, 10); // Should return the required size
}

/**
 * @brief Test for Dmod_SnPrintf with complex format
 * 
 * The test checks if the function handles multiple format specifiers.
 */
TEST_F(DmodSnPrintfTest, SnPrintfComplexFormat)
{
    char buffer[64];
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "Int: %d, String: %s, Hex: 0x%X", 
                                123, "test", 0xABCD);
    
    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "Int: 123, String: test, Hex: 0xABCD");
}

/**
 * @brief Test for Dmod_SnPrintf with zero-sized buffer
 * 
 * The test checks if the function handles zero-sized buffer correctly.
 */
TEST_F(DmodSnPrintfTest, SnPrintfZeroBuffer)
{
    char buffer[1] = {0};
    int result = Dmod_SnPrintf(buffer, 0, "Hello");
    
    ASSERT_EQ(result, 5); // Should return the required size
    ASSERT_EQ(buffer[0], 0); // Buffer should not be modified
}

/**
 * @brief Test for Dmod_SnPrintf with left-aligned string (%-30s)
 * 
 * The test checks if the function handles left-aligned strings with width.
 */
TEST_F(DmodSnPrintfTest, SnPrintfLeftAlignedString)
{
    char buffer[64];
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%-30s", "test");
    
    ASSERT_EQ(result, 30); // Should be padded to 30 characters
    ASSERT_STREQ(buffer, "test                          ");
    ASSERT_EQ(strlen(buffer), 30);
}

/**
 * @brief Test for Dmod_SnPrintf with right-aligned string (%30s)
 * 
 * The test checks if the function handles right-aligned strings with width.
 */
TEST_F(DmodSnPrintfTest, SnPrintfRightAlignedString)
{
    char buffer[64];
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%30s", "test");
    
    ASSERT_EQ(result, 30); // Should be padded to 30 characters
    ASSERT_STREQ(buffer, "                          test");
    ASSERT_EQ(strlen(buffer), 30);
}

/**
 * @brief Test for Dmod_SnPrintf with multiple width-formatted strings
 * 
 * The test checks if the function handles multiple width-formatted strings like in module list.
 */
TEST_F(DmodSnPrintfTest, SnPrintfMultipleWidthStrings)
{
    char buffer[128];
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%-30s %-15s %-40s", 
                                "Module Name", "Version", "Description");
    
    ASSERT_EQ(result, 30 + 1 + 15 + 1 + 40); // 30 + space + 15 + space + 40 = 87
    ASSERT_STREQ(buffer, "Module Name                    Version         Description                             ");
}

/**
 * @brief Test for Dmod_SnPrintf with string longer than width
 * 
 * The test checks if the function handles strings longer than the specified width.
 */
TEST_F(DmodSnPrintfTest, SnPrintfStringLongerThanWidth)
{
    char buffer[64];
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%-10s", "This is a long string");
    
    ASSERT_EQ(result, 21); // String length is 21, no padding needed
    ASSERT_STREQ(buffer, "This is a long string");
}

/**
 * @brief Test for Dmod_SnPrintf with mixed format specifiers and widths
 * 
 * The test checks if the function handles both width-formatted and regular specifiers.
 */
TEST_F(DmodSnPrintfTest, SnPrintfMixedWidthFormats)
{
    char buffer[128];
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%-20s: %d", "Count", 42);
    
    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "Count               : 42");
}

/**
 * @brief Test for Dmod_SnPrintf with zero width (should work like normal)
 * 
 * The test checks if the function handles zero width correctly.
 */
TEST_F(DmodSnPrintfTest, SnPrintfZeroWidth)
{
    char buffer[64];
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%-0s", "test");
    
    ASSERT_EQ(result, 4);
    ASSERT_STREQ(buffer, "test");
}

