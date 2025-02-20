#define DMOD_PRIVATE
#include <string.h>
#include <gtest/gtest.h>
#include "dmod.h"
#include "private/dmod_ctx.h"

class DmodDmfcTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

// ===============================================================
//                  Tests for Dmod_IsDMFC
// ===============================================================

/**
 * @brief Test for Dmod_IsDMFC
 * 
 * The test checks if the function returns true for a DMFC data.
 */
TEST(DmodDmfcTest, IsDMFC)
{
    Dmod_DmfcHeader_t header;
    memset(&header, 0, sizeof(header));
    header.Signature = DMOD_DMFC_SIGNATURE;
    header.HeaderSize = sizeof(Dmod_DmfcHeader_t);
    bool result = Dmod_IsDMFC(&header, sizeof(header));
    ASSERT_TRUE(result);
}

/**
 * @brief Test for Dmod_IsDMFC
 * 
 * The test checks if the function returns false for a non-DMFC data.
 */
TEST(DmodDmfcTest, IsNotDMFC)
{
    Dmod_DmfcHeader_t header;
    memset(&header, 0, sizeof(header));
    header.Signature = DMOD_HEADER_SIGNATURE;
    header.HeaderSize = sizeof(Dmod_DmfcHeader_t);
    bool result = Dmod_IsDMFC(&header, sizeof(header));
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_IsDMFC
 * 
 * The test checks if the function returns false for a NULL data.
 */
TEST(DmodDmfcTest, IsNullData)
{
    bool result = Dmod_IsDMFC(NULL, 0);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_IsDMFC
 * 
 * The test checks if the function returns false for a small data.
 */
TEST(DmodDmfcTest, IsSmallData)
{
    Dmod_DmfcHeader_t header;
    memset(&header, 0, sizeof(header));
    header.Signature = DMOD_DMFC_SIGNATURE;
    header.HeaderSize = sizeof(Dmod_DmfcHeader_t);
    bool result = Dmod_IsDMFC(&header, sizeof(Dmod_DmfcHeader_t) - 1);
    ASSERT_FALSE(result);
}

// ===============================================================
//                  Tests for Dmod_IsDMFCFile
// ===============================================================
/**
 * @brief Test for Dmod_IsDMFCFile
 * 
 * The test checks if the function returns true for a DMFC file.
 */
TEST(DmodDmfcTest, IsDMFCFile)
{
    #ifdef DMOD_TEST_DMFC_FILE
    bool result = Dmod_IsDMFCFile(DMOD_TEST_DMFC_FILE);
    ASSERT_TRUE(result);
    #else 
    #   warning DMOD_TEST_DMFC_FILE is not defined, test skipped
    #endif
}

/**
 * @brief Test for Dmod_IsDMFCFile
 * 
 * The test checks if the function returns false for a non-DMFC file.
 */
TEST(DmodDmfcTest, IsNotDMFCFile)
{
    #ifdef DMOD_TEST_DMF_FILE
    bool result = Dmod_IsDMFCFile(DMOD_TEST_DMF_FILE);
    ASSERT_FALSE(result);
    #else 
    #   warning DMOD_TEST_DMF_FILE is not defined, test skipped
    #endif
}

/**
 * @brief Test for Dmod_IsDMFCFile
 * 
 * The test checks if the function returns false for a NULL file.
 */
TEST(DmodDmfcTest, IsNullFile)
{
    bool result = Dmod_IsDMFCFile(NULL);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_IsDMFCFile
 * 
 * The test checks if the function returns false for an empty file.
 */
TEST(DmodDmfcTest, IsEmptyFile)
{
    bool result = Dmod_IsDMFCFile("");
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_IsDMFCFile
 * 
 * The test checks if the function returns false for a non-existing file.
 */
TEST(DmodDmfcTest, IsNotExistingFile)
{
    bool result = Dmod_IsDMFCFile("non-existing-file");
    ASSERT_FALSE(result);
}

// ===============================================================
//                  Tests for Dmod_ToDMFC
// ===============================================================

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns true for a valid DMF data.
 */
TEST(DmodDmfcTest, ToDMFC)
{
    #if defined(DMOD_TEST_DMF_FILE) && DMOD_USE_FASTLZ
    Dmod_Context_t* context = Dmod_LoadFile(DMOD_TEST_DMF_FILE);
    ASSERT_NE(context, nullptr);
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    bool result = Dmod_ToDMFC("fastlz", 1, context->Data, context->Size, &dmfcData, &dmfcSize);
    ASSERT_TRUE(result);
    ASSERT_NE(dmfcData, nullptr);
    ASSERT_GT(dmfcSize, 0);
    Dmod_Free(dmfcData);
    Dmod_Context_Delete(context);
    #else 
    #   warning DMOD_TEST_DMF_FILE is not defined, test skipped
    #endif
}

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns false for a NULL data.
 */
TEST(DmodDmfcTest, ToDMFCNullData)
{
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    bool result = Dmod_ToDMFC("fastlz", 1, NULL, 0, &dmfcData, &dmfcSize);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns false for a small data.
 */
TEST(DmodDmfcTest, ToDMFCSmallData)
{
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    bool result = Dmod_ToDMFC("fastlz", 1, NULL, 1, &dmfcData, &dmfcSize);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns false for a NULL output data.
 */
TEST(DmodDmfcTest, ToDMFCNullOutputData)
{
    bool result = Dmod_ToDMFC("fastlz", 1, NULL, 0, NULL, NULL);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns false for an empty compression name.
 */
TEST(DmodDmfcTest, ToDMFCEmptyCompressionName)
{
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    bool result = Dmod_ToDMFC("", 1, NULL, 0, &dmfcData, &dmfcSize);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns false for an invalid compression level.
 */
TEST(DmodDmfcTest, ToDMFCInvalidLevel)
{
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    bool result = Dmod_ToDMFC("fastlz", 0, NULL, 0, &dmfcData, &dmfcSize);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns false for an invalid compression algorithm name.
 */
TEST(DmodDmfcTest, ToDMFCInvalidAlgorithm)
{
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    bool result = Dmod_ToDMFC("invalid", 1, NULL, 0, &dmfcData, &dmfcSize);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns false for a NULL compression algorithm.
 */
TEST(DmodDmfcTest, ToDMFCNullAlgorithm)
{
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    bool result = Dmod_ToDMFC(NULL, 1, NULL, 0, &dmfcData, &dmfcSize);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns false for a NULL DMF data.
 */
TEST(DmodDmfcTest, ToDMFCInvalidData)
{
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    bool result = Dmod_ToDMFC("fastlz", 1, NULL, 0, &dmfcData, &dmfcSize);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns false for too long compression algorithm name.
 */
TEST(DmodDmfcTest, ToDMFCInvalidAlgorithmName)
{
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    bool result = Dmod_ToDMFC("this-is-a-very-long-compression-algorithm-name", 1, NULL, 0, &dmfcData, &dmfcSize);
    ASSERT_FALSE(result);
}
