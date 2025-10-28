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
