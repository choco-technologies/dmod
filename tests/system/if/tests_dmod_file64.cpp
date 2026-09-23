#define DMOD_PRIVATE
#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <type_traits>

#include <unistd.h>

#include "dmod_sal.h"

static_assert(sizeof(Dmod_FileOffset_t) == sizeof(int64_t),
              "file offsets must be exactly 64-bit");
static_assert(sizeof(Dmod_FileSize_t) == sizeof(uint64_t),
              "file sizes must be exactly 64-bit");
static_assert(std::is_same<decltype(&Dmod_FileSeek),
                           int (*)(void*, Dmod_FileOffset_t, int)>::value,
              "Dmod_FileSeek must expose the 2.0 offset type");
static_assert(std::is_same<decltype(&Dmod_FileTell),
                           Dmod_FileOffset_t (*)(void*)>::value,
              "Dmod_FileTell must expose the 2.0 offset type");

class DmodFile64Test : public ::testing::Test
{
protected:
    void SetUp() override
    {
        char pathTemplate[] = "/tmp/dmod64XXXXXX";
        int descriptor = mkstemp(pathTemplate);
        ASSERT_GE(descriptor, 0);
        ASSERT_EQ(close(descriptor), 0);

        path = pathTemplate;
        file = Dmod_FileOpen(path.c_str(), "w+b");
        ASSERT_NE(file, nullptr);
    }

    void TearDown() override
    {
        if( file != nullptr )
        {
            Dmod_FileClose(file);
        }
        if( !path.empty() )
        {
            unlink(path.c_str());
        }
    }

    std::string path;
    void* file = nullptr;
};

TEST_F(DmodFile64Test, RoundTripsOffsetsAndSizesAboveFourGiB)
{
    const Dmod_FileOffset_t offset = (INT64_C(1) << 32) + INT64_C(123);
    ASSERT_EQ(Dmod_FileSeek(file, offset, DMOD_SEEK_SET), 0);
    ASSERT_EQ(Dmod_FileTell(file), offset);

    const char byte = 'x';
    ASSERT_EQ(Dmod_FileWrite(&byte, 1, 1, file), 1u);
    ASSERT_EQ(Dmod_FileTell(file), offset + 1);
    ASSERT_EQ(Dmod_FileSize(file), (Dmod_FileSize_t)(offset + 1));
    ASSERT_EQ(Dmod_FileTell(file), offset + 1);

    Dmod_FileStat_t stat = {};
    ASSERT_EQ(Dmod_FileStat(path.c_str(), &stat), 0);
    ASSERT_EQ(stat.Size, (Dmod_FileSize_t)(offset + 1));
}

TEST(DmodFile64ContractTest, UsesDocumentedFailureValues)
{
    ASSERT_EQ(Dmod_FileTell(nullptr), DMOD_FILE_OFFSET_ERROR);
    ASSERT_EQ(Dmod_FileSize(nullptr), 0u);
}

TEST(DmodFile64ContractTest, RegistersMajorVersionTwo)
{
    ASSERT_NE(std::strstr(Dmod_FileSeek_registration.Signature, ":2.0"), nullptr);
    ASSERT_NE(std::strstr(Dmod_FileTell_registration.Signature, ":2.0"), nullptr);
    ASSERT_NE(std::strstr(Dmod_FileSize_registration.Signature, ":2.0"), nullptr);
    ASSERT_NE(std::strstr(Dmod_FileStat_registration.Signature, ":2.0"), nullptr);
}
