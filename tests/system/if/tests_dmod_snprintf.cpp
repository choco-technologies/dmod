#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <string.h>
#include "dmod_sal.h"
#include "private/dmod_prf.h"

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

/**
 * @brief Test for Dmod_SnPrintf with %llu (unsigned long long)
 * 
 * The test checks if the function handles unsigned long long format specifier.
 */
TEST_F(DmodSnPrintfTest, SnPrintfUnsignedLongLong)
{
    char buffer[64];
    uint64_t value = 18446744073709551615ULL; // Max uint64_t
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%llu", value);
    
    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "18446744073709551615");
}

/**
 * @brief Test for Dmod_SnPrintf with %lld (signed long long)
 * 
 * The test checks if the function handles signed long long format specifier.
 */
TEST_F(DmodSnPrintfTest, SnPrintfSignedLongLong)
{
    char buffer[64];
    int64_t value = -9223372036854775807LL - 1LL; // Min int64_t
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%lld", value);
    
    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "-9223372036854775808");
}

/**
 * @brief Test for Dmod_SnPrintf with positive %lld
 * 
 * The test checks if the function handles positive signed long long values.
 */
TEST_F(DmodSnPrintfTest, SnPrintfPositiveLongLong)
{
    char buffer[64];
    int64_t value = 9223372036854775807LL; // Max int64_t
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%lld", value);
    
    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "9223372036854775807");
}

/**
 * @brief Test for Dmod_SnPrintf with %llx (long long hex lowercase)
 * 
 * The test checks if the function handles long long hexadecimal format specifier.
 */
TEST_F(DmodSnPrintfTest, SnPrintfLongLongHexLowercase)
{
    char buffer[64];
    uint64_t value = 0xFEDCBA9876543210ULL;
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%llx", value);
    
    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "fedcba9876543210");
}

/**
 * @brief Test for Dmod_SnPrintf with %llX (long long hex uppercase)
 * 
 * The test checks if the function handles long long hexadecimal uppercase format specifier.
 */
TEST_F(DmodSnPrintfTest, SnPrintfLongLongHexUppercase)
{
    char buffer[64];
    uint64_t value = 0xFEDCBA9876543210ULL;
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%llX", value);
    
    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "FEDCBA9876543210");
}

/**
 * @brief Test for Dmod_SnPrintf with small %llu value
 * 
 * The test checks if the function handles small unsigned long long values correctly.
 */
TEST_F(DmodSnPrintfTest, SnPrintfSmallUnsignedLongLong)
{
    char buffer[64];
    uint64_t value = 42ULL;
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%llu", value);
    
    ASSERT_EQ(result, 2);
    ASSERT_STREQ(buffer, "42");
}

/**
 * @brief Test for Dmod_SnPrintf with zero %llu value
 * 
 * The test checks if the function handles zero unsigned long long value.
 */
TEST_F(DmodSnPrintfTest, SnPrintfZeroLongLong)
{
    char buffer[64];
    uint64_t value = 0ULL;
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%llu", value);
    
    ASSERT_EQ(result, 1);
    ASSERT_STREQ(buffer, "0");
}

/**
 * @brief Test for Dmod_SnPrintf with mixed format specifiers including %llu
 * 
 * The test checks if the function handles multiple format specifiers including long long.
 */
TEST_F(DmodSnPrintfTest, SnPrintfMixedWithLongLong)
{
    char buffer[128];
    uint64_t big_value = 1234567890123456789ULL;
    int small_value = 42;
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), 
                                "Small: %d, Big: %llu, Hex: %llx", 
                                small_value, big_value, big_value);
    
    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "Small: 42, Big: 1234567890123456789, Hex: 112210f47de98115");
}

/**
 * @brief Test for Dmod_SnPrintf with negative zero %lld
 * 
 * The test checks if the function handles zero signed long long value.
 */
TEST_F(DmodSnPrintfTest, SnPrintfNegativeZeroLongLong)
{
    char buffer[64];
    int64_t value = 0LL;
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%lld", value);
    
    ASSERT_EQ(result, 1);
    ASSERT_STREQ(buffer, "0");
}

/**
 * @brief Test for Dmod_SnPrintf with %lli (signed long long, 'i' variant)
 * 
 * The test checks if the function handles %lli format specifier.
 */
TEST_F(DmodSnPrintfTest, SnPrintfSignedLongLongI)
{
    char buffer[64];
    int64_t value = -1234567890123456789LL;
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%lli", value);
    
    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "-1234567890123456789");
}

/**
 * @brief Test for Dmod_SnPrintf with %lu (unsigned long)
 * 
 * The test checks if the function handles unsigned long format specifier.
 */
TEST_F(DmodSnPrintfTest, SnPrintfUnsignedLong)
{
    char buffer[64];
    unsigned long value = 4294967295UL;
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%lu", value);
    
    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "4294967295");
}

/**
 * @brief Test for Dmod_SnPrintf with %ld (signed long)
 * 
 * The test checks if the function handles signed long format specifier.
 */
TEST_F(DmodSnPrintfTest, SnPrintfSignedLong)
{
    char buffer[64];
    long value = -2147483648L;
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%ld", value);
    
    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "-2147483648");
}

/**
 * @brief Test for Dmod_SnPrintf with %lx (unsigned long hex)
 * 
 * The test checks if the function handles unsigned long hex format specifier.
 */
TEST_F(DmodSnPrintfTest, SnPrintfLongHex)
{
    char buffer[64];
    unsigned long value = 0xFFFFFFFFUL;
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%lx", value);
    
    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "ffffffff");
}

/**
 * @brief Test for Dmod_SnPrintf with %zu (size_t)
 * 
 * The test checks if the function handles size_t format specifier.
 */
TEST_F(DmodSnPrintfTest, SnPrintfSizeT)
{
    char buffer[64];
    size_t value = 12345;
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%zu", value);
    
    ASSERT_EQ(result, 5);
    ASSERT_STREQ(buffer, "12345");
}

/**
 * @brief Test for Dmod_SnPrintf with %hd (short)
 * 
 * The test checks if the function handles short format specifier.
 */
TEST_F(DmodSnPrintfTest, SnPrintfShort)
{
    char buffer[64];
    short value = -32768;
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%hd", value);
    
    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "-32768");
}

/**
 * @brief Test for Dmod_SnPrintf with %hu (unsigned short)
 * 
 * The test checks if the function handles unsigned short format specifier.
 */
TEST_F(DmodSnPrintfTest, SnPrintfUnsignedShort)
{
    char buffer[64];
    unsigned short value = 65535;
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%hu", value);
    
    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "65535");
}

/**
 * @brief Test for Dmod_SnPrintf with %hhd (signed char)
 * 
 * The test checks if the function handles signed char format specifier.
 */
TEST_F(DmodSnPrintfTest, SnPrintfSignedChar)
{
    char buffer[64];
    signed char value = -128;
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%hhd", value);
    
    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "-128");
}

/**
 * @brief Test for Dmod_SnPrintf with %hhu (unsigned char)
 * 
 * The test checks if the function handles unsigned char format specifier.
 */
TEST_F(DmodSnPrintfTest, SnPrintfUnsignedChar)
{
    char buffer[64];
    unsigned char value = 255;
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%hhu", value);
    
    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "255");
}

/**
 * @brief Test for Dmod_SnPrintf with %o (octal)
 * 
 * The test checks if the function handles octal format specifier.
 */
TEST_F(DmodSnPrintfTest, SnPrintfOctal)
{
    char buffer[64];
    unsigned int value = 0755;
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%o", value);
    
    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "755");
}

/**
 * @brief Test for Dmod_SnPrintf with %lo (long octal)
 * 
 * The test checks if the function handles long octal format specifier.
 */
TEST_F(DmodSnPrintfTest, SnPrintfLongOctal)
{
    char buffer[64];
    unsigned long value = 0xFFFFFFFFUL;
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%lo", value);
    
    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "37777777777");
}

/**
 * @brief Test for Dmod_SnPrintf with %llo (long long octal)
 * 
 * The test checks if the function handles long long octal format specifier.
 */
TEST_F(DmodSnPrintfTest, SnPrintfLongLongOctal)
{
    char buffer[64];
    uint64_t value = 0xFFFFFFFFFFFFFFFFULL;
    int result = Dmod_SnPrintf(buffer, sizeof(buffer), "%llo", value);
    
    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "1777777777777777777777");
}

/**
 * @brief Test for Dmod_SnPrintf with mixed new format specifiers
 * 
 * The test checks if the function handles multiple new format specifiers.
 */
TEST_F(DmodSnPrintfTest, SnPrintfMixedNewFormats)
{
    char buffer[128];
    unsigned long ul = 4294967295UL;
    short s = -32768;
    unsigned char uc = 255;
    unsigned int oct = 0755;
    int result = Dmod_SnPrintf(buffer, sizeof(buffer),
                                "Mixed: %lu, %hd, %hhu, %o", ul, s, uc, oct);

    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "Mixed: 4294967295, -32768, 255, 755");
}

/**
 * @brief Test for Dmod_SnPrintf with right-aligned unsigned integer (%10u)
 *
 * Regression test: integer conversions previously parsed the width flag but
 * silently ignored it, unlike %s which honored it correctly.
 */
TEST_F(DmodSnPrintfTest, SnPrintfRightAlignedUnsigned)
{
    char buffer[64];
    int result = Dmod_SnPrintf_Impl(buffer, sizeof(buffer), "%10u", 14u);

    ASSERT_EQ(result, 10);
    ASSERT_STREQ(buffer, "        14");
}

/**
 * @brief Test for Dmod_SnPrintf with right-aligned size_t (%14zu)
 */
TEST_F(DmodSnPrintfTest, SnPrintfRightAlignedSizeT)
{
    char buffer[64];
    int result = Dmod_SnPrintf_Impl(buffer, sizeof(buffer), "%14zu", (size_t)36924);

    ASSERT_EQ(result, 14);
    ASSERT_STREQ(buffer, "         36924");
}

/**
 * @brief Test for Dmod_SnPrintf with right-aligned signed integer, including sign (%5d)
 */
TEST_F(DmodSnPrintfTest, SnPrintfRightAlignedSignedInt)
{
    char buffer[64];
    int result = Dmod_SnPrintf_Impl(buffer, sizeof(buffer), "%5d", -42);

    ASSERT_EQ(result, 5);
    ASSERT_STREQ(buffer, "  -42");
}

/**
 * @brief Test for Dmod_SnPrintf with left-aligned signed integer (%-10d)
 */
TEST_F(DmodSnPrintfTest, SnPrintfLeftAlignedSignedInt)
{
    char buffer[64];
    int result = Dmod_SnPrintf_Impl(buffer, sizeof(buffer), "%-10d|", -42);

    ASSERT_EQ(result, 11);
    ASSERT_STREQ(buffer, "-42       |");
}

/**
 * @brief Test for Dmod_SnPrintf with a full tabular row: string + two width-padded
 * size_t columns, mirroring how modules print aligned tables (e.g. memory -m).
 */
TEST_F(DmodSnPrintfTest, SnPrintfTabularRowMatchesHeaderWidth)
{
    char header[128];
    char row[128];

    Dmod_SnPrintf_Impl(header, sizeof(header), "%-32s %10s %14s", "MODULE", "BLOCKS", "BYTES");
    Dmod_SnPrintf_Impl(row, sizeof(row), "%-32s %10zu %14zu", "dmell#0", (size_t)14, (size_t)36924);

    ASSERT_EQ(strlen(header), strlen(row));
}

/**
 * @brief Test for Dmod_SnPrintf with %f using the default precision (6)
 */
TEST_F(DmodSnPrintfTest, SnPrintfFloatDefaultPrecision)
{
    char buffer[64];
    int result = Dmod_SnPrintf_Impl(buffer, sizeof(buffer), "%f", 3.14);

    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "3.140000");
}

/**
 * @brief Test for Dmod_SnPrintf with %f and an explicit precision (%.2f)
 */
TEST_F(DmodSnPrintfTest, SnPrintfFloatExplicitPrecision)
{
    char buffer[64];
    int result = Dmod_SnPrintf_Impl(buffer, sizeof(buffer), "%.2f", 18.84);

    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "18.84");
}

/**
 * @brief Test for Dmod_SnPrintf with %.0f (no decimal point at all)
 */
TEST_F(DmodSnPrintfTest, SnPrintfFloatZeroPrecision)
{
    char buffer[64];
    int result = Dmod_SnPrintf_Impl(buffer, sizeof(buffer), "%.0f", 3.6);

    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "4");
}

/**
 * @brief Test for Dmod_SnPrintf with a negative %f value
 */
TEST_F(DmodSnPrintfTest, SnPrintfFloatNegative)
{
    char buffer[64];
    int result = Dmod_SnPrintf_Impl(buffer, sizeof(buffer), "%.2f", -3.14);

    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "-3.14");
}

/**
 * @brief Test for Dmod_SnPrintf with %f rounding that carries into the integer part
 */
TEST_F(DmodSnPrintfTest, SnPrintfFloatRoundingCarry)
{
    char buffer[64];
    int result = Dmod_SnPrintf_Impl(buffer, sizeof(buffer), "%f", 0.9999995);

    ASSERT_GT(result, 0);
    ASSERT_STREQ(buffer, "1.000000");
}

/**
 * @brief Test for Dmod_SnPrintf with %f combined with width (right-aligned)
 */
TEST_F(DmodSnPrintfTest, SnPrintfFloatWithWidth)
{
    char buffer[64];
    int result = Dmod_SnPrintf_Impl(buffer, sizeof(buffer), "%9.2f", 3.14);

    ASSERT_EQ(result, 9);
    ASSERT_STREQ(buffer, "     3.14");
}

/**
 * @brief Test for Dmod_SnPrintf with %f for NaN and +/-infinity
 */
TEST_F(DmodSnPrintfTest, SnPrintfFloatNanAndInf)
{
    char buffer[64];
    volatile double zero = 0.0;
    volatile double one = 1.0;

    Dmod_SnPrintf_Impl(buffer, sizeof(buffer), "%f", zero / zero);
    ASSERT_STREQ(buffer, "nan");

    Dmod_SnPrintf_Impl(buffer, sizeof(buffer), "%f", one / zero);
    ASSERT_STREQ(buffer, "inf");

    Dmod_SnPrintf_Impl(buffer, sizeof(buffer), "%f", -one / zero);
    ASSERT_STREQ(buffer, "-inf");
}


