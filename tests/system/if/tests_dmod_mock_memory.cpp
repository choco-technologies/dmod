#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <string.h>
#include "dmod_sal.h"

// ===============================================================
//                  Tests for Mock Memory
// ===============================================================

class DmodMockMemoryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

#ifdef DMOD_MEMORY_MOCK_ADDRESS
/**
 * @brief Test for Dmod_ReadMemory with mock memory
 * 
 * The test checks if the function can read from mock memory.
 */
TEST_F(DmodMockMemoryTest, ReadMemoryFromMock)
{
    uint8_t buffer[16] = {0};
    
    // Read from the beginning of mock memory
    size_t bytesRead = Dmod_ReadMemory(DMOD_MEMORY_MOCK_ADDRESS, buffer, sizeof(buffer));
    
    ASSERT_EQ(bytesRead, sizeof(buffer));
    
    // The test memory file contains bytes 0-255
    for (size_t i = 0; i < sizeof(buffer); i++) 
    {
        ASSERT_EQ(buffer[i], (uint8_t)i);
    }
}

/**
 * @brief Test for Dmod_WriteMemory with mock memory
 * 
 * The test checks if the function can write to mock memory and read it back.
 */
TEST_F(DmodMockMemoryTest, WriteMemoryToMock)
{
    uint8_t writeData[16] = {0xFF, 0xFE, 0xFD, 0xFC, 0xFB, 0xFA, 0xF9, 0xF8,
                              0xF7, 0xF6, 0xF5, 0xF4, 0xF3, 0xF2, 0xF1, 0xF0};
    uint8_t readData[16] = {0};
    
    // Write to mock memory
    size_t bytesWritten = Dmod_WriteMemory(DMOD_MEMORY_MOCK_ADDRESS + 128, writeData, sizeof(writeData));
    ASSERT_EQ(bytesWritten, sizeof(writeData));
    
    // Read back from mock memory
    size_t bytesRead = Dmod_ReadMemory(DMOD_MEMORY_MOCK_ADDRESS + 128, readData, sizeof(readData));
    ASSERT_EQ(bytesRead, sizeof(readData));
    
    // Verify data
    ASSERT_EQ(memcmp(readData, writeData, sizeof(readData)), 0);
}

/**
 * @brief Test for Dmod_ReadMemory with partial read from mock memory
 * 
 * The test checks if the function handles reading past the end of mock memory.
 */
TEST_F(DmodMockMemoryTest, ReadMemoryPartialMock)
{
    uint8_t buffer[64] = {0};
    
    // Try to read past the end of mock memory (256 bytes)
    // Reading from offset 240 with size 64 should only read 16 bytes
    size_t bytesRead = Dmod_ReadMemory(DMOD_MEMORY_MOCK_ADDRESS + 240, buffer, sizeof(buffer));
    
    ASSERT_EQ(bytesRead, 16); // Should only read to end of mock memory
    
    // Verify the first 16 bytes contain expected values
    for (size_t i = 0; i < 16; i++) 
    {
        ASSERT_EQ(buffer[i], (uint8_t)(240 + i));
    }
}

#else
TEST_F(DmodMockMemoryTest, MockMemoryNotEnabled)
{
    // This test just confirms that mock memory is not enabled
    GTEST_SKIP() << "Mock memory is not enabled (DMOD_MEMORY_MOCK_ADDRESS not defined)";
}
#endif
