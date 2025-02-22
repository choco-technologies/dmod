#define DMOD_PRIVATE
#include <string.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "dmod.h"
#include "private/dmod_ctx.h"
#include "private/dmod_vars.h"
#include "fastlz.h"


class MockMemory 
{
public:
    MOCK_METHOD(void*, Malloc, (size_t Size));
    MOCK_METHOD(void*, Realloc, (void* ptr, size_t Size));
    MOCK_METHOD(void*, AlignedMalloc, (size_t Size, size_t Alignment));
    MOCK_METHOD(void, Free, (void* ptr));
};

class MockCompression
{
public:
    MOCK_METHOD(size_t, GetMaxSize, (const char* Name, int Level, size_t SrcSize));
    MOCK_METHOD(size_t, Pack, (const char* Name, int Level, void* Dest, size_t DestSize, const void* Src, size_t SrcSize));
    MOCK_METHOD(size_t, Unpack, (const char* Name, void* Dest, size_t DestSize, const void* Src, size_t SrcSize));
    MOCK_METHOD(bool, IsSupported, (const char* Name));
};

MockMemory* mockMemory = nullptr;
MockCompression* mockCompression = nullptr;

extern "C"
{
    /**
     * @brief Allocate memory
     * 
     * @param Size Size of memory to allocate
     * 
     * @return Pointer to allocated memory
     */
    void* Dmod_Malloc(size_t Size)
    {
        if( mockMemory != nullptr )
        {
            return mockMemory->Malloc(Size);
        }
        return malloc(Size);
    }

    /**
     * @brief Reallocate memory
     * 
     * @param ptr Pointer to memory to reallocate
     * @param Size Size of memory to allocate
     * 
     * @return Pointer to reallocated memory
     */
    void* Dmod_Realloc(void* ptr, size_t Size)
    {
        if( mockMemory != nullptr )
        {
            return mockMemory->Realloc(ptr, Size);
        }
        return realloc(ptr, Size);
    }

    /**
     * @brief Allocate aligned memory
     * 
     * @param Size Size of memory to allocate
     * @param Alignment Alignment of memory
     * 
     * @return Pointer to allocated memory
     * 
     * @note Optional - set to NULL if not supported
     */
    void* Dmod_AlignedMalloc(size_t Size, size_t Alignment)
    {
        if( mockMemory != nullptr )
        {
            return mockMemory->AlignedMalloc(Size, Alignment);
        }
        return aligned_alloc(Alignment, Size);
    }

    /**
     * @brief Free memory
     * 
     * @param ptr Pointer to memory to free
     */
    void Dmod_Free(void *ptr)
    {
        if( mockMemory != nullptr )
        {
            mockMemory->Free(ptr);
            return;
        }
        free(ptr);
    }

    /**
     * @brief Get maximum size of compressed data
     * 
     * @param Name Compression algorithm name
     * @param Level Compression level
     * @param SrcSize Source buffer size
     * 
     * @return Maximum size of compressed data
     */
    size_t Dmod_Compression_GetMaxSize(const char* Name, int Level, size_t SrcSize)
    {
        if( mockCompression != nullptr )
        {
            return mockCompression->GetMaxSize(Name, Level, SrcSize);
        }
        size_t max_output_size = 0;
        if( SrcSize == 0 )
        {
            DMOD_LOG_ERROR("Cannot compress data with size 0\n");
            return 0;
        }
        #if DMOD_USE_FASTLZ
        else if( strcmp( Name, "fastlz" ) == 0 )
        {
            max_output_size = SrcSize + (SrcSize / 16) + 64 + 3;
        }
        #endif
        else 
        {
            DMOD_LOG_ERROR("Compression algorithm '%s' is not supported\n", Name);
        }

        return max_output_size;
    }

    /**
     * @brief Compress data
     * 
     * @param Name Compression algorithm name
     * @param Level Compression level
     * @param Dest Destination buffer
     * @param DestSize Destination buffer size
     * @param Src Source buffer
     * @param SrcSize Source buffer size
     * 
     * @return Compressed data size
     */
    size_t Dmod_Compression_Pack(const char* Name, int Level, void* Dest, size_t DestSize, const void* Src, size_t SrcSize)
    {
        if( mockCompression != nullptr )
        {
            return mockCompression->Pack(Name, Level, Dest, DestSize, Src, SrcSize);
        }
        size_t Ret = 0;
        if( SrcSize == 0 )
        {
            DMOD_LOG_ERROR("Cannot compress data with size 0\n");
            return 0;
        }
        #if DMOD_USE_FASTLZ
        else if( strcmp( Name, "fastlz" ) == 0 )
        {
            Ret = fastlz_compress_level(Level, Src, SrcSize, Dest );
        }
        #endif
        else 
        {
            DMOD_LOG_ERROR("Compression algorithm '%s' is not supported\n", Name);
        }

        return Ret;
    }

    /**
     * @brief Decompress data
     * 
     * @param Name Compression algorithm name
     * @param Dest Destination buffer
     * @param DestSize Destination buffer size
     * @param Src Source buffer
     * @param SrcSize Source buffer size
     * 
     * @return Decompressed data size
     */
    size_t Dmod_Compression_Unpack(const char* Name, void* Dest, size_t DestSize, const void* Src, size_t SrcSize)
    {
        if( mockCompression != nullptr )
        {
            return mockCompression->Unpack(Name, Dest, DestSize, Src, SrcSize);
        }
        size_t Ret = 0;
        if( SrcSize == 0 )
        {
            DMOD_LOG_ERROR("Cannot decompress data with size 0\n");
            return 0;
        }
        #if DMOD_USE_FASTLZ
        if(  strcmp( Name, "fastlz" ) == 0 )
        {
            Ret = fastlz_decompress( Src, SrcSize, Dest, DestSize );
        }
        #endif
        else 
        {
            DMOD_LOG_ERROR("Compression algorithm '%s' is not supported\n", Name);
        }
        return Ret;
    }

    /**
     * @brief Check if the compression algorithm is supported
     * 
     * @param Name Compression algorithm name
     * 
     * @return true if the compression algorithm is supported, false otherwise
     */
    bool Dmod_Compression_IsSupported(const char* Name)
    {
        if( mockCompression != nullptr )
        {
            return mockCompression->IsSupported(Name);
        }
        bool Ret = false;
        #if DMOD_USE_FASTLZ
        if( strcmp( Name, "fastlz" ) == 0)
        {
            Ret = true;
        }
        #endif
        return Ret;
    }
}
class DmodDmfcTest : public ::testing::Test
{
protected:
#if defined(DMOD_TEST_DMFC_FILE)
    const char* m_DmfcFile = DMOD_TEST_DMFC_FILE;
#elif defined(DMOD_TEST_DMF_FILE)
    const char* m_DmfcFile = DMOD_BUILD_DIR "/test.dmfc";
#else
#   error DMOD_TEST_DMFC_FILE or DMOD_TEST_DMF_FILE must be defined
    const char* m_DmfcFile = nullptr;
#endif

    const char* m_EmptyFile = "empty.dmfc";
    const char* m_ShortFile = "short.dmfc";

    void SetUp() override
    {
        memset(Dmod_Contexts, 0, sizeof(Dmod_Contexts));
        PrepareDmfc();
        PrepareEmptyFile();
        PrepareShortFile();
    }

    void TearDown() override
    {
        remove(m_EmptyFile);
        remove(m_ShortFile);

        DisableMockMemory();
        DisableMockCompression();
    }

    bool ConvertToDmfc(const char* path, void** dmfcData, size_t* dmfcSize)
    {
        void* buffer = nullptr;
        size_t size = 0;
        FILE* file = fopen(path, "rb");
        if(!file)
        {
            return false;
        }
        fseek(file, 0, SEEK_END);
        size = ftell(file);
        fseek(file, 0, SEEK_SET);

        buffer = malloc(size);
        if(!buffer)
        {
            fclose(file);
            return false;
        }
        fread(buffer, 1, size, file);
        fclose(file);

        bool result = Dmod_ToDMFC("fastlz", 1, buffer, size, dmfcData, dmfcSize);
        free(buffer);
        return result;
    }

    bool ConvertToDmfcFile( const char* inputPath, const char* outputPath )
    {
        void* dmfcData = nullptr;
        size_t dmfcSize = 0;
        bool result = ConvertToDmfc(inputPath, &dmfcData, &dmfcSize);
        if(result)
        {
            FILE* file = fopen(outputPath, "wb");
            if(file != nullptr)
            {
                fwrite(dmfcData, 1, dmfcSize, file);
                fclose(file);
            }
            Dmod_Free(dmfcData);
        }
        return result;
    }

    bool PrepareDmfc()
    {
    #if defined(DMOD_TEST_DMFC_FILE)
        return true;
    #elif defined(DMOD_TEST_DMF_FILE)
        return ConvertToDmfcFile(DMOD_TEST_DMF_FILE, m_DmfcFile);
    #else 
    #   error DMOD_TEST_DMFC_FILE or DMOD_TEST_DMF_FILE must be defined
        return false;
    #endif
    }

    bool CreateEmptyFile(const char* path)
    {
        FILE* file = fopen(path, "wb");
        if(file != nullptr)
        {
            fclose(file);
            return true;
        }
        return false;
    }

    bool PrepareEmptyFile()
    {
        return CreateEmptyFile(m_EmptyFile);
    }

    bool CreateShortFile(const char* path)
    {
        FILE* file = fopen(path, "wb");
        if(file != nullptr)
        {
            fwrite("short", 1, 5, file);
            fclose(file);
            return true;
        }
        return false;
    }

    bool PrepareShortFile()
    {
        return CreateShortFile(m_ShortFile);
    }

    void EnableMockMemory()
    {
        mockMemory = new MockMemory();
    }

    void EnableMockCompression()
    {
        mockCompression = new MockCompression();
    }

    void DisableMockMemory()
    {
        if(mockMemory != nullptr)
        {
            testing::Mock::VerifyAndClearExpectations(&mockMemory);
            delete mockMemory;
            mockMemory = nullptr;
        }
    }

    void DisableMockCompression()
    {
        if(mockCompression != nullptr)
        {
            testing::Mock::VerifyAndClearExpectations(&mockCompression);
            delete mockCompression;
            mockCompression = nullptr;
        }
    }

    void PrepareTestDmfc( void** outDmfcData, size_t* outDmfcSize )
    {
        FILE* file = fopen(m_DmfcFile, "rb");

        if(file != nullptr)
        {
            fseek(file, 0, SEEK_END);
            size_t size = ftell(file);
            fseek(file, 0, SEEK_SET);

            void* data = malloc(size);
            if(data != nullptr)
            {
                fread(data, 1, size, file);
                *outDmfcData = data;
                *outDmfcSize = size;
            }
            fclose(file);
        }
    }

    size_t GetDmfcOriginalSize( void* dmfcData, size_t dmfcSize )
    {
        Dmod_DmfcHeader_t* header = (Dmod_DmfcHeader_t*)dmfcData;
        return header != nullptr && header->OriginalSize;
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
TEST_F(DmodDmfcTest, IsDMFC)
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
TEST_F(DmodDmfcTest, IsNotDMFC)
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
TEST_F(DmodDmfcTest, IsNullData)
{
    bool result = Dmod_IsDMFC(NULL, 0);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_IsDMFC
 * 
 * The test checks if the function returns false for a small data.
 */
TEST_F(DmodDmfcTest, IsSmallData)
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
TEST_F(DmodDmfcTest, IsDMFCFile)
{
    bool result = Dmod_IsDMFCFile(m_DmfcFile);
    ASSERT_TRUE(result);
}

/**
 * @brief Test for Dmod_IsDMFCFile
 * 
 * The test checks if the function returns false for a non-DMFC file.
 */
TEST_F(DmodDmfcTest, IsNotDMFCFile)
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
TEST_F(DmodDmfcTest, IsNullFile)
{
    bool result = Dmod_IsDMFCFile(NULL);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_IsDMFCFile
 * 
 * The test checks if the function returns false for an empty file.
 */
TEST_F(DmodDmfcTest, IsEmptyFile)
{
    bool result = Dmod_IsDMFCFile(m_EmptyFile);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_IsDMFCFile
 * 
 * The test checks if the function returns false for a short file.
 */
TEST_F(DmodDmfcTest, IsShortFile)
{
    bool result = Dmod_IsDMFCFile(m_ShortFile);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_IsDMFCFile
 * 
 * The test checks if the function returns false for a non-existing file.
 */
TEST_F(DmodDmfcTest, IsNotExistingFile)
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
TEST_F(DmodDmfcTest, ToDMFC)
{
    #if DMOD_USE_FASTLZ
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
TEST_F(DmodDmfcTest, ToDMFCNullData)
{
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    bool result = Dmod_ToDMFC("fastlz", 1, NULL, 0, &dmfcData, &dmfcSize);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns false for a NULL output data.
 */
TEST_F(DmodDmfcTest, ToDMFCNullOutputData)
{
    bool result = Dmod_ToDMFC("fastlz", 1, NULL, 0, NULL, NULL);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns false for an empty compression name.
 */
TEST_F(DmodDmfcTest, ToDMFCEmptyCompressionName)
{
    Dmod_Context_t* context = Dmod_LoadFile(DMOD_TEST_DMF_FILE);
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    bool result = Dmod_ToDMFC("", 1, context->Data, context->Size, &dmfcData, &dmfcSize);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns false for long compression name.
 */
TEST_F(DmodDmfcTest, ToDMFCLongCompressionName)
{
    Dmod_Context_t* context = Dmod_LoadFile(DMOD_TEST_DMF_FILE);
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    bool result = Dmod_ToDMFC("this-is-a-very-long-compression-algorithm-name", 1, context->Data, context->Size, &dmfcData, &dmfcSize);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns false for an invalid compression level.
 */
TEST_F(DmodDmfcTest, ToDMFCInvalidLevel)
{
    Dmod_Context_t* context = Dmod_LoadFile(DMOD_TEST_DMF_FILE);
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    bool result = Dmod_ToDMFC("fastlz", 0, context->Data, context->Size, &dmfcData, &dmfcSize);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns false for an invalid compression algorithm name.
 */
TEST_F(DmodDmfcTest, ToDMFCInvalidAlgorithm)
{
    Dmod_Context_t* context = Dmod_LoadFile(DMOD_TEST_DMF_FILE);
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    bool result = Dmod_ToDMFC("invalid", 1, context->Data, context->Size, &dmfcData, &dmfcSize);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns false for a NULL compression algorithm.
 */
TEST_F(DmodDmfcTest, ToDMFCNullAlgorithm)
{
    Dmod_Context_t* context = Dmod_LoadFile(DMOD_TEST_DMF_FILE);
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    bool result = Dmod_ToDMFC(NULL, 1, context->Data, context->Size, &dmfcData, &dmfcSize);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns false for a NULL DMF data.
 */
TEST_F(DmodDmfcTest, ToDMFCInvalidData)
{
    Dmod_ModuleHeader_t header;
    memset(&header, 0, sizeof(header));

    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    bool result = Dmod_ToDMFC("fastlz", 1, &header, sizeof(header), &dmfcData, &dmfcSize);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns false if Dmod_Compression_GetMaxSize fails.
 */
TEST_F(DmodDmfcTest, ToDMFCGetMaxSizeFail)
{
    EnableMockCompression();
    EXPECT_CALL(*mockCompression, IsSupported(testing::_))
        .WillOnce(testing::Return(true));
    EXPECT_CALL(*mockCompression, GetMaxSize(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(0));

    Dmod_Context_t* context = Dmod_LoadFile(DMOD_TEST_DMF_FILE);
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    bool result = Dmod_ToDMFC("fastlz", 1, context->Data, context->Size, &dmfcData, &dmfcSize);
    ASSERT_FALSE(result);
    ASSERT_EQ(dmfcData, nullptr);
    ASSERT_EQ(dmfcSize, 0);
    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns false if Dmod_Malloc fails.
 */
TEST_F(DmodDmfcTest, ToDMFCMallocFail)
{
    
    Dmod_Context_t* context = Dmod_LoadFile(DMOD_TEST_DMF_FILE);
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;

    EnableMockMemory();
    EXPECT_CALL(*mockMemory, Malloc(testing::_))
        .WillOnce(testing::Return(nullptr));

    bool result = Dmod_ToDMFC("fastlz", 1, context->Data, context->Size, &dmfcData, &dmfcSize);
    ASSERT_FALSE(result);
    ASSERT_EQ(dmfcData, nullptr);
    ASSERT_EQ(dmfcSize, 0);
    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns false if Dmod_Realloc fails.
 */
TEST_F(DmodDmfcTest, ToDMFCReallocFail)
{
    Dmod_Context_t* context = Dmod_LoadFile(DMOD_TEST_DMF_FILE);
    ASSERT_NE(context, nullptr);
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    void* testBuffer = malloc(context->Size);

    EnableMockMemory();
    EXPECT_CALL(*mockMemory, Malloc(testing::_))
        .WillOnce(testing::Return(testBuffer));
    EXPECT_CALL(*mockMemory, Realloc(testBuffer, testing::_))
        .WillOnce(testing::Return(nullptr));

    bool result = Dmod_ToDMFC("fastlz", 1, context->Data, context->Size, &dmfcData, &dmfcSize);
    ASSERT_FALSE(result);
    ASSERT_EQ(dmfcData, nullptr);
    ASSERT_EQ(dmfcSize, 0);

    DisableMockMemory();

    Dmod_Context_Delete(context);
    free(testBuffer);
}

/**
 * @brief Test for Dmod_ToDMFC
 * 
 * The test checks if the function returns false if Dmod_Compression_Pack fails.
 */
TEST_F(DmodDmfcTest, ToDMFCPackFail)
{
    Dmod_Context_t* context = Dmod_LoadFile(DMOD_TEST_DMF_FILE);
    ASSERT_NE(context, nullptr);
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;

    EnableMockCompression();
    EXPECT_CALL(*mockCompression, IsSupported(testing::_))
        .WillOnce(testing::Return(true));
    EXPECT_CALL(*mockCompression, GetMaxSize(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(context->Size));
    EXPECT_CALL(*mockCompression, Pack(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(0));

    bool result = Dmod_ToDMFC("fastlz", 1, context->Data, context->Size, &dmfcData, &dmfcSize);
    ASSERT_FALSE(result);
    ASSERT_EQ(dmfcData, nullptr);
    ASSERT_EQ(dmfcSize, 0);

    Dmod_Context_Delete(context);
}

// ===============================================================
//                  Tests for Dmod_FromDMFC
// ===============================================================

/**
 * @brief Test for Dmod_FromDMFC
 * 
 * The test checks if the function returns true for a valid DMFC data.
 */
TEST_F(DmodDmfcTest, FromDMFC)
{
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    PrepareTestDmfc(&dmfcData, &dmfcSize);

    void* dmfData = nullptr;
    size_t dmfSize = 0;
    bool result = Dmod_FromDMFC(dmfcData, dmfcSize, &dmfData, &dmfSize);
    ASSERT_TRUE(result);
    ASSERT_NE(dmfData, nullptr);
    ASSERT_GT(dmfSize, 0);

    // Test if the file is valid DMF
    Dmod_Context_t* context = Dmod_Load(dmfData, dmfSize);
    ASSERT_NE(context, nullptr);
    ASSERT_NE(context->Header, nullptr);
    Dmod_Context_Delete(context);

    Dmod_Free(dmfData);
    Dmod_Free(dmfcData);
}

/**
 * @brief Test for Dmod_FromDMFC
 * 
 * The test checks if the function returns false for a NULL data.
 */
TEST_F(DmodDmfcTest, FromDMFCNullData)
{
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    PrepareTestDmfc(&dmfcData, &dmfcSize);
    ASSERT_NE(dmfcData, nullptr);
    ASSERT_NE(dmfcSize, 0);

    void* dmfData = nullptr;
    size_t dmfSize = 0;
    bool result = Dmod_FromDMFC(NULL, dmfcSize, &dmfData, &dmfSize);
    ASSERT_FALSE(result);

    result = Dmod_FromDMFC(dmfcData, dmfcSize, NULL, &dmfSize);
    ASSERT_FALSE(result);

    result = Dmod_FromDMFC(dmfcData, dmfcSize, &dmfData, NULL);
    ASSERT_FALSE(result);

    Dmod_Free(dmfcData);
}

/**
 * @brief Test for Dmod_FromDMFC
 * 
 * The test checks if the function returns false for a 0 size
 */
TEST_F(DmodDmfcTest, FromDMFCZeroSize)
{
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    PrepareTestDmfc(&dmfcData, &dmfcSize);
    ASSERT_NE(dmfcData, nullptr);
    ASSERT_NE(dmfcSize, 0);

    void* dmfData = nullptr;
    size_t dmfSize = 0;
    bool result = Dmod_FromDMFC(dmfcData, 0, &dmfData, &dmfSize);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_FromDMFC
 * 
 * The test checks if the function returns false for a small data.
 */
TEST_F(DmodDmfcTest, FromDMFCSmallData)
{
    Dmod_DmfcHeader_t header;
    memset(&header, 0, sizeof(header));
    header.Signature = DMOD_DMFC_SIGNATURE;
    header.HeaderSize = sizeof(Dmod_DmfcHeader_t);
    void* dmfcData = &header;
    size_t dmfcSize = sizeof(Dmod_DmfcHeader_t);

    void* dmfData = nullptr;
    size_t dmfSize = 0;
    bool result = Dmod_FromDMFC(dmfcData, dmfcSize, &dmfData, &dmfSize);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_FromDMFC
 * 
 * The test checks if the function returns false for an invalid signature.
 */
TEST_F(DmodDmfcTest, FromDMFCInvalidSignature)
{
    size_t dmfcSize = sizeof(Dmod_DmfcHeader_t) + 1;
    Dmod_DmfcHeader_t* header = (Dmod_DmfcHeader_t*)malloc(dmfcSize);
    memset(header, 0, sizeof(Dmod_DmfcHeader_t) + 1);   
    header->Signature = 0;
    header->HeaderSize = sizeof(Dmod_DmfcHeader_t);
    void* dmfcData = header;

    void* dmfData = nullptr;
    size_t dmfSize = 0;
    bool result = Dmod_FromDMFC(dmfcData, dmfcSize, &dmfData, &dmfSize);
    ASSERT_FALSE(result);

    free(header);
}

/**
 * @brief Test for Dmod_FromDMFC
 * 
 * The test checks if the function returns false for an invalid header size.
 */
TEST_F(DmodDmfcTest, FromDMFCInvalidHeaderSize)
{
    size_t dmfcSize = sizeof(Dmod_DmfcHeader_t) + 1;
    Dmod_DmfcHeader_t* header = (Dmod_DmfcHeader_t*)malloc(dmfcSize);
    memset(header, 0, sizeof(Dmod_DmfcHeader_t) + 1);   
    header->Signature = DMOD_DMFC_SIGNATURE;
    header->HeaderSize = sizeof(Dmod_DmfcHeader_t) - 1;
    void* dmfcData = header;

    void* dmfData = nullptr;
    size_t dmfSize = 0;
    bool result = Dmod_FromDMFC(dmfcData, dmfcSize, &dmfData, &dmfSize);
    ASSERT_FALSE(result);

    free(header);
}

/**
 * @brief Test for Dmod_FromDMFC
 * 
 * The test checks if the function returns false for incompatible header version.
 */
TEST_F(DmodDmfcTest, FromDMFCInvalidHeaderVersion)
{
    size_t dmfcSize = sizeof(Dmod_DmfcHeader_t) + 1;
    Dmod_DmfcHeader_t* header = (Dmod_DmfcHeader_t*)malloc(dmfcSize);
    memset(header, 0, sizeof(Dmod_DmfcHeader_t) + 1);   
    header->Signature = DMOD_DMFC_SIGNATURE;
    header->HeaderSize = sizeof(Dmod_DmfcHeader_t);
    header->HeaderVersion = 0xff00;
    void* dmfcData = header;

    void* dmfData = nullptr;
    size_t dmfSize = 0;
    bool result = Dmod_FromDMFC(dmfcData, dmfcSize, &dmfData, &dmfSize);
    ASSERT_FALSE(result);

    free(header);
}

/**
 * @brief Test for Dmod_FromDMFC
 * 
 * The test checks if the function returns false for an invalid original size.
 */
TEST_F(DmodDmfcTest, FromDMFCInvalidOriginalSize)
{
    size_t dmfcSize = sizeof(Dmod_DmfcHeader_t) + 10;
    Dmod_DmfcHeader_t* header = (Dmod_DmfcHeader_t*)malloc(dmfcSize);
    memset(header, 0, sizeof(Dmod_DmfcHeader_t) + 1);   
    header->Signature = DMOD_DMFC_SIGNATURE;
    header->HeaderSize = sizeof(Dmod_DmfcHeader_t);
    header->HeaderVersion = DMOD_DMFC_VERSION;
    header->OriginalSize = 0;
    void* dmfcData = header;

    void* dmfData = nullptr;
    size_t dmfSize = 0;
    bool result = Dmod_FromDMFC(dmfcData, dmfcSize, &dmfData, &dmfSize);
    ASSERT_FALSE(result);

    header->OriginalSize = 1;
    result = Dmod_FromDMFC(dmfcData, dmfcSize, &dmfData, &dmfSize);
    ASSERT_FALSE(result);

    free(header);
}

/**
 * @brief Test for Dmod_FromDMFC
 * 
 * The test checks if the function returns false for a too long compression name. 
 */
TEST_F(DmodDmfcTest, FromDMFCLongCompressionName)
{
    size_t dmfcSize = sizeof(Dmod_DmfcHeader_t) + 10;
    Dmod_DmfcHeader_t* header = (Dmod_DmfcHeader_t*)malloc(dmfcSize);
    memset(header, 0, sizeof(Dmod_DmfcHeader_t) + 1);   
    header->Signature = DMOD_DMFC_SIGNATURE;
    header->HeaderSize = sizeof(Dmod_DmfcHeader_t);
    header->HeaderVersion = DMOD_DMFC_VERSION;
    header->OriginalSize = 10;
    memset(header->Compression, 'a', sizeof(header->Compression) + 10);
    void* dmfcData = header;

    void* dmfData = nullptr;
    size_t dmfSize = 0;
    bool result = Dmod_FromDMFC(dmfcData, dmfcSize, &dmfData, &dmfSize);
    ASSERT_FALSE(result);

    free(header);
}

/**
 * @brief Test for Dmod_FromDMFC
 * 
 * The test checks if the function returns false for an invalid compression name.
 */
TEST_F(DmodDmfcTest, FromDMFCInvalidCompressionName)
{
    size_t dmfcSize = sizeof(Dmod_DmfcHeader_t) + 10;
    Dmod_DmfcHeader_t* header = (Dmod_DmfcHeader_t*)malloc(dmfcSize);
    memset(header, 0, sizeof(Dmod_DmfcHeader_t) + 1);   
    header->Signature = DMOD_DMFC_SIGNATURE;
    header->HeaderSize = sizeof(Dmod_DmfcHeader_t);
    header->HeaderVersion = DMOD_DMFC_VERSION;
    header->OriginalSize = 10;
    memset(header->Compression, 0, sizeof(header->Compression));
    void* dmfcData = header;

    void* dmfData = nullptr;
    size_t dmfSize = 0;
    bool result = Dmod_FromDMFC(dmfcData, dmfcSize, &dmfData, &dmfSize);
    ASSERT_FALSE(result);

    free(header);
}

/**
 * @brief Test for Dmod_FromDMFC
 * 
 * The test checks if the function returns false when Dmod_Malloc fails.
 */
TEST_F(DmodDmfcTest, FromDMFCMallocFail)
{
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    PrepareTestDmfc(&dmfcData, &dmfcSize);
    ASSERT_NE(dmfcData, nullptr);
    ASSERT_NE(dmfcSize, 0);

    void* dmfData = nullptr;
    size_t dmfSize = 0;

    EnableMockMemory();
    EXPECT_CALL(*mockMemory, Malloc(testing::_))
        .WillOnce(testing::Return(nullptr));

    bool result = Dmod_FromDMFC(dmfcData, dmfcSize, &dmfData, &dmfSize);
    ASSERT_FALSE(result);
    ASSERT_EQ(dmfData, nullptr);
    ASSERT_EQ(dmfSize, 0);

    DisableMockMemory();
    Dmod_Free(dmfcData);
}

/**
 * @brief Test for Dmod_FromDMFC
 * 
 * The test checks if the function returns false when Dmod_Compression_Unpack fails.
 */
TEST_F(DmodDmfcTest, FromDMFCUnpackFail)
{
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    PrepareTestDmfc(&dmfcData, &dmfcSize);
    ASSERT_NE(dmfcData, nullptr);
    ASSERT_NE(dmfcSize, 0);

    void* dmfData = nullptr;
    size_t dmfSize = 0;

    EnableMockCompression();
    EXPECT_CALL(*mockCompression, IsSupported(testing::_))
        .WillOnce(testing::Return(true));
    EXPECT_CALL(*mockCompression, Unpack(testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(0));

    bool result = Dmod_FromDMFC(dmfcData, dmfcSize, &dmfData, &dmfSize);
    ASSERT_FALSE(result);
    ASSERT_EQ(dmfData, nullptr);
    ASSERT_EQ(dmfSize, 0);

    Dmod_Free(dmfcData);
}

/**
 * @brief Test for Dmod_FromDMFC
 * 
 * The test checks if the function returns true when decompressed data size is different than the original size.
 */
TEST_F(DmodDmfcTest, FromDMFCInvalidDecompressedSize)
{
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    PrepareTestDmfc(&dmfcData, &dmfcSize);
    ASSERT_NE(dmfcData, nullptr);
    ASSERT_NE(dmfcSize, 0);

    void* dmfData = nullptr;
    size_t dmfSize = GetDmfcOriginalSize(dmfcData, dmfcSize);
    void* testBuffer = malloc(dmfcSize);
    size_t decompressedSize = 1;

    EnableMockMemory();
    EXPECT_CALL(*mockMemory, Malloc(testing::_))
        .WillOnce(testing::Return(testBuffer));
    EnableMockCompression();
    EXPECT_CALL(*mockCompression, IsSupported(testing::_))
        .WillOnce(testing::Return(true));
    EXPECT_CALL(*mockCompression, Unpack(testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(decompressedSize));
    EXPECT_CALL(*mockMemory, Realloc(testBuffer, decompressedSize))
        .WillOnce(testing::Return(testBuffer));

    dmfSize = 0;
    bool result = Dmod_FromDMFC(dmfcData, dmfcSize, &dmfData, &dmfSize);
    ASSERT_TRUE(result);

    DisableMockMemory();
    Dmod_Free(dmfcData);
    free(testBuffer);
}

/**
 * @brief Test for Dmod_FromDMFC
 * 
 * The test checks if the function returns false when Dmod_Realloc fails.
 */
TEST_F(DmodDmfcTest, FromDMFCReallocFail)
{
    void* dmfcData = nullptr;
    size_t dmfcSize = 0;
    PrepareTestDmfc(&dmfcData, &dmfcSize);
    ASSERT_NE(dmfcData, nullptr);
    ASSERT_NE(dmfcSize, 0);

    void* dmfData = nullptr;
    size_t dmfSize = GetDmfcOriginalSize(dmfcData, dmfcSize);
    void* testBuffer = malloc(dmfcSize);
    size_t decompressedSize = 1;

    EnableMockMemory();
    EXPECT_CALL(*mockMemory, Malloc(testing::_))
        .WillOnce(testing::Return(testBuffer));
    EnableMockCompression();
    EXPECT_CALL(*mockCompression, IsSupported(testing::_))
        .WillOnce(testing::Return(true));
    EXPECT_CALL(*mockCompression, Unpack(testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(decompressedSize));
    EXPECT_CALL(*mockMemory, Realloc(testBuffer, decompressedSize))
        .WillOnce(testing::Return(nullptr));

    dmfSize = 0;
    bool result = Dmod_FromDMFC(dmfcData, dmfcSize, &dmfData, &dmfSize);
    ASSERT_FALSE(result);
    ASSERT_EQ(dmfData, nullptr);
    ASSERT_EQ(dmfSize, 0);

    DisableMockMemory();
    Dmod_Free(dmfcData);
    free(testBuffer);
}