#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <string.h>
#include "dmod_sal.h"

// ===============================================================
//                  Tests for Dmod_ReadMemory and Dmod_WriteMemory
// ===============================================================

class DmodRawMemoryTest : public ::testing::Test
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
 * @brief Test for Dmod_ReadMemory
 * 
 * The test checks if the function can read from a memory location.
 */
TEST_F(DmodRawMemoryTest, ReadMemory)
{
    uint8_t sourceData[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                               0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    uint8_t buffer[16] = {0};
    
    size_t bytesRead = Dmod_ReadMemory((uintptr_t)sourceData, buffer, sizeof(buffer));
    
    ASSERT_EQ(bytesRead, sizeof(buffer));
    ASSERT_EQ(memcmp(buffer, sourceData, sizeof(buffer)), 0);
}

/**
 * @brief Test for Dmod_WriteMemory
 * 
 * The test checks if the function can write to a memory location.
 */
TEST_F(DmodRawMemoryTest, WriteMemory)
{
    uint8_t destData[16] = {0};
    uint8_t sourceData[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                               0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    
    size_t bytesWritten = Dmod_WriteMemory((uintptr_t)destData, sourceData, sizeof(sourceData));
    
    ASSERT_EQ(bytesWritten, sizeof(sourceData));
    ASSERT_EQ(memcmp(destData, sourceData, sizeof(sourceData)), 0);
}

/**
 * @brief Test for Dmod_ReadMemory with NULL buffer
 * 
 * The test checks if the function handles NULL buffer correctly.
 */
TEST_F(DmodRawMemoryTest, ReadMemoryNullBuffer)
{
    uint8_t sourceData[16] = {0};
    
    size_t bytesRead = Dmod_ReadMemory((uintptr_t)sourceData, NULL, sizeof(sourceData));
    
    ASSERT_EQ(bytesRead, 0);
}

/**
 * @brief Test for Dmod_WriteMemory with NULL buffer
 * 
 * The test checks if the function handles NULL buffer correctly.
 */
TEST_F(DmodRawMemoryTest, WriteMemoryNullBuffer)
{
    uint8_t destData[16] = {0};
    
    size_t bytesWritten = Dmod_WriteMemory((uintptr_t)destData, NULL, sizeof(destData));
    
    ASSERT_EQ(bytesWritten, 0);
}

/**
 * @brief Test for Dmod_ReadMemory with zero size
 * 
 * The test checks if the function handles zero size correctly.
 */
TEST_F(DmodRawMemoryTest, ReadMemoryZeroSize)
{
    uint8_t sourceData[16] = {0};
    uint8_t buffer[16] = {0};
    
    size_t bytesRead = Dmod_ReadMemory((uintptr_t)sourceData, buffer, 0);
    
    ASSERT_EQ(bytesRead, 0);
}

/**
 * @brief Test for Dmod_WriteMemory with zero size
 * 
 * The test checks if the function handles zero size correctly.
 */
TEST_F(DmodRawMemoryTest, WriteMemoryZeroSize)
{
    uint8_t destData[16] = {0};
    uint8_t sourceData[16] = {0};
    
    size_t bytesWritten = Dmod_WriteMemory((uintptr_t)destData, sourceData, 0);
    
    ASSERT_EQ(bytesWritten, 0);
}

/**
 * @brief Test for Dmod_ReadMemory and Dmod_WriteMemory together
 * 
 * The test checks if data written can be read back correctly.
 */
TEST_F(DmodRawMemoryTest, ReadWriteMemoryRoundTrip)
{
    uint8_t memory[32] = {0};
    uint8_t writeData[16] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                              0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};
    uint8_t readData[16] = {0};
    
    // Write to memory
    size_t bytesWritten = Dmod_WriteMemory((uintptr_t)memory, writeData, sizeof(writeData));
    ASSERT_EQ(bytesWritten, sizeof(writeData));
    
    // Read back from memory
    size_t bytesRead = Dmod_ReadMemory((uintptr_t)memory, readData, sizeof(readData));
    ASSERT_EQ(bytesRead, sizeof(readData));
    
    // Verify data
    ASSERT_EQ(memcmp(readData, writeData, sizeof(readData)), 0);
}
