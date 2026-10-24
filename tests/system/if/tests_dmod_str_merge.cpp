/**
 * MIT License
 * 
 * Copyright (c) 2025 patryk.kubiak90@gmail.com
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * @brief Tests for Dmod_StrMerge and Dmod_StrMergeEx functions
 * @date 24 Oct 2026
 * 
 * @file tests_dmod_str_merge.cpp
 * @version 0.1
 */
#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <string.h>
#include "dmod_sal.h"

/**
 * @brief Test for Dmod_StrMerge with two strings
 */
TEST(DmodStrMergeTest, MergeTwoStrings)
{
    char* result = Dmod_StrMerge(2, "Hello", "World");
    
    ASSERT_NE(result, nullptr);
    ASSERT_STREQ(result, "HelloWorld");
    Dmod_Free(result);
}

/**
 * @brief Test for Dmod_StrMerge with three strings
 */
TEST(DmodStrMergeTest, MergeThreeStrings)
{
    char* result = Dmod_StrMerge(3, "One", "Two", "Three");
    
    ASSERT_NE(result, nullptr);
    ASSERT_STREQ(result, "OneTwoThree");
    Dmod_Free(result);
}

/**
 * @brief Test for Dmod_StrMerge with NULL strings
 */
TEST(DmodStrMergeTest, MergeWithNullString)
{
    char* result = Dmod_StrMerge(3, "Hello", NULL, "World");
    
    ASSERT_NE(result, nullptr);
    ASSERT_STREQ(result, "HelloWorld");
    Dmod_Free(result);
}

/**
 * @brief Test for Dmod_StrMerge with empty strings
 */
TEST(DmodStrMergeTest, MergeWithEmptyStrings)
{
    char* result = Dmod_StrMerge(3, "", "Hello", "");
    
    ASSERT_NE(result, nullptr);
    ASSERT_STREQ(result, "Hello");
    Dmod_Free(result);
}

/**
 * @brief Test for Dmod_StrMerge with single string
 */
TEST(DmodStrMergeTest, MergeSingleString)
{
    char* result = Dmod_StrMerge(1, "Solo");
    
    ASSERT_NE(result, nullptr);
    ASSERT_STREQ(result, "Solo");
    Dmod_Free(result);
}

/**
 * @brief Test for Dmod_StrMerge with count <= 0
 */
TEST(DmodStrMergeTest, MergeInvalidCount)
{
    char* result1 = Dmod_StrMerge(0, "Hello");
    char* result2 = Dmod_StrMerge(-1, "Hello");
    
    ASSERT_EQ(result1, nullptr);
    ASSERT_EQ(result2, nullptr);
}

/**
 * @brief Test for Dmod_StrMerge result length
 */
TEST(DmodStrMergeTest, MergeResultLength)
{
    char* result = Dmod_StrMerge(2, "Test", "String");
    
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(strlen(result), 10); // "TestString" = 10 chars
    Dmod_Free(result);
}

/**
 * @brief Test for Dmod_StrMerge with many strings
 */
TEST(DmodStrMergeTest, MergeManyStrings)
{
    char* result = Dmod_StrMerge(5, "A", "B", "C", "D", "E");
    
    ASSERT_NE(result, nullptr);
    ASSERT_STREQ(result, "ABCDE");
    Dmod_Free(result);
}

/**
 * @brief Test for Dmod_StrMerge with long strings
 */
TEST(DmodStrMergeTest, MergeLongStrings)
{
    const char* str1 = "This is a very long string that should be merged ";
    const char* str2 = "with another long string to form a complete result ";
    const char* str3 = "that is longer than any individual input string.";
    
    char* result = Dmod_StrMerge(3, str1, str2, str3);
    
    ASSERT_NE(result, nullptr);
    ASSERT_STREQ(result, "This is a very long string that should be merged with another long string to form a complete result that is longer than any individual input string.");
    Dmod_Free(result);
}

/**
 * @brief Test for Dmod_StrMergeEx with custom allocator name
 */
TEST(DmodStrMergeTest, MergeExWithCustomAllocator)
{
    char* result = Dmod_StrMergeEx(2, "test_allocator", "Hello", "World");
    
    ASSERT_NE(result, nullptr);
    ASSERT_STREQ(result, "HelloWorld");
    Dmod_Free(result);
}
