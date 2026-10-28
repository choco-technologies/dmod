#define DMOD_PRIVATE
#include <string.h>
#include <vector>
#include <gtest/gtest.h>
#include "dmod.h"
#include "dmod_defs.h"
#include "dmod_types.h"

// DMF -> DMFC -> DMF with every compression method. A DMF for the conversion only has to start with the module header
// (the signature and the name); the rest is the module's code, which this is not - but it is as compressible.
static std::vector<uint8_t> FakeModule(size_t size)
{
    std::vector<uint8_t> dmf(size);
    uint32_t state = 777;

    for( size_t i = 0; i < size; i++ )
    {
        state = state * 1664525u + 1013904223u;
        // Words of a few kinds of values, like the instructions and the tables of a real module
        dmf[i] = (i % 4 == 3) ? 0xE0 : (uint8_t)((state >> 24) % 24);
    }
    Dmod_ModuleHeader_t* header = reinterpret_cast<Dmod_ModuleHeader_t*>(dmf.data());
    memset(header, 0, sizeof(*header));
    header->Signature = DMOD_HEADER_SIGNATURE;
    strncpy(header->Name, "fakemodule", sizeof(header->Name) - 1);
    return dmf;
}

class DmodDmfcMethodsTest : public ::testing::TestWithParam<const char*>
{
};

TEST_P(DmodDmfcMethodsTest, ToDmfcAndBack)
{
    const char* method = GetParam();
    const std::vector<uint8_t> dmf = FakeModule(12000);
    void* dmfc = nullptr;
    size_t dmfcSize = 0;

    ASSERT_TRUE(Dmod_ToDMFC(method, 2, dmf.data(), dmf.size(), &dmfc, &dmfcSize));
    ASSERT_NE(dmfc, nullptr);
    EXPECT_LT(dmfcSize, dmf.size());
    EXPECT_TRUE(Dmod_IsDMFC(dmfc, dmfcSize));

    // The method is named in the header: the loader decompresses with it
    const Dmod_DmfcHeader_t* header = static_cast<const Dmod_DmfcHeader_t*>(dmfc);
    EXPECT_STREQ(header->Compression, method);
    EXPECT_EQ(header->OriginalSize, dmf.size());

    void* back = nullptr;
    size_t backSize = 0;
    ASSERT_TRUE(Dmod_FromDMFC(dmfc, dmfcSize, &back, &backSize));
    ASSERT_EQ(backSize, dmf.size());
    EXPECT_EQ(memcmp(back, dmf.data(), dmf.size()), 0);

    Dmod_Free(back);
    Dmod_Free(dmfc);
}

// A DMFC with the compressed data cut short: deflate notices (the stream ends before its last block), so the loader
// does not get a half of a module. (fastlz gives what it has decoded, and Dmod_FromDMFC only warns about the size.)
TEST(DmodDmfcDeflateTest, DamagedDmfcIsRefused)
{
    const std::vector<uint8_t> dmf = FakeModule(12000);
    void* dmfc = nullptr;
    size_t dmfcSize = 0;

    ASSERT_TRUE(Dmod_ToDMFC("deflate", 2, dmf.data(), dmf.size(), &dmfc, &dmfcSize));

    void* back = nullptr;
    size_t backSize = 0;
    EXPECT_FALSE(Dmod_FromDMFC(dmfc, dmfcSize - dmfcSize / 3, &back, &backSize));
    Dmod_Free(dmfc);
}

INSTANTIATE_TEST_SUITE_P(Methods, DmodDmfcMethodsTest, ::testing::Values("fastlz", "deflate"));
