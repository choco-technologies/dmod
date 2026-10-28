#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include "dmod_sal.h"

// The heap of this test: counts what the compression takes and gives back, and can refuse to give anything
static int  g_LiveAllocations = 0;
static bool g_FailAllocations = false;

extern "C"
{
    void* Dmod_MallocEx(size_t Size, const char* ModuleName)
    {
        (void)ModuleName;
        if( g_FailAllocations )
        {
            return nullptr;
        }
        void* ptr = malloc(Size);
        if( ptr != nullptr )
        {
            g_LiveAllocations++;
        }
        return ptr;
    }

    void Dmod_FreeEx(void* ptr, bool Concatenate)
    {
        (void)Concatenate;
        if( ptr != nullptr )
        {
            g_LiveAllocations--;
            free(ptr);
        }
    }
}

class DmodCompressionTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_LiveAllocations = 0;
        g_FailAllocations = false;
    }

    // Something that compresses like a module does: structured, repetitive, with some noise
    static std::vector<uint8_t> Sample(size_t size)
    {
        static const char* words[] = { "module", "loader", "flash", "driver", "device", "memory", "thread", "mount" };
        std::vector<uint8_t> data;
        uint32_t state = 12345;

        while( data.size() < size )
        {
            state = state * 1103515245u + 12345u;
            const char* word = words[(state >> 16) % 8];
            data.insert(data.end(), word, word + strlen(word));
            data.push_back((state >> 8) % 11 == 0 ? '\n' : ' ');
            if( (state >> 20) % 5 == 0 )
            {
                data.push_back((uint8_t)(state >> 3));
            }
        }
        data.resize(size);
        return data;
    }

    static size_t Pack(const char* name, int level, const std::vector<uint8_t>& data, std::vector<uint8_t>& packed)
    {
        size_t max = Dmod_Compression_GetMaxSize(name, level, data.size());
        packed.assign(max, 0);
        size_t size = Dmod_Compression_Pack(name, level, packed.data(), packed.size(), data.data(), data.size());
        packed.resize(size);
        return size;
    }
};

TEST_F(DmodCompressionTest, SupportedMethods)
{
    EXPECT_TRUE(Dmod_Compression_IsSupported("fastlz"));
    EXPECT_TRUE(Dmod_Compression_IsSupported("deflate"));
    EXPECT_FALSE(Dmod_Compression_IsSupported("lzma"));

    // fastlz stays the first: it is the default of todmfc, and what the loaders without deflate can read
    ASSERT_NE(Dmod_Compression_GetNextSupported(nullptr), nullptr);
    EXPECT_STREQ(Dmod_Compression_GetNextSupported(nullptr), "fastlz");
    EXPECT_STREQ(Dmod_Compression_GetNextSupported("fastlz"), "deflate");
    EXPECT_EQ(Dmod_Compression_GetNextSupported("deflate"), nullptr);
}

TEST_F(DmodCompressionTest, DeflateRoundTripOnEveryLevel)
{
    const std::vector<uint8_t> data = Sample(20000);

    for( int level = 0; level <= 9; level++ )
    {
        std::vector<uint8_t> packed;
        size_t max = Dmod_Compression_GetMaxSize("deflate", level, data.size());

        ASSERT_GE(max, data.size()) << "level " << level;
        ASSERT_GT(Pack("deflate", level, data, packed), 0u) << "level " << level;
        EXPECT_LE(packed.size(), max);
        if( level > 0 )
        {
            EXPECT_LT(packed.size(), data.size() / 2) << "level " << level;
        }

        std::vector<uint8_t> unpacked(data.size());
        EXPECT_EQ(Dmod_Compression_Unpack("deflate", unpacked.data(), unpacked.size(), packed.data(), packed.size()),
                  data.size()) << "level " << level;
        EXPECT_EQ(unpacked, data) << "level " << level;
    }
    // The tables of the decoder come from the heap and go back to it
    EXPECT_EQ(g_LiveAllocations, 0);
}

TEST_F(DmodCompressionTest, DeflateIsSmallerThanFastlz)
{
    const std::vector<uint8_t> data = Sample(60000);
    std::vector<uint8_t> deflate, fastlz;

    ASSERT_GT(Pack("deflate", 9, data, deflate), 0u);
    ASSERT_GT(Pack("fastlz", 2, data, fastlz), 0u);
    EXPECT_LT(deflate.size(), fastlz.size());
}

TEST_F(DmodCompressionTest, FastlzStillWorks)
{
    const std::vector<uint8_t> data = Sample(10000);
    std::vector<uint8_t> packed;

    ASSERT_GT(Pack("fastlz", 2, data, packed), 0u);
    std::vector<uint8_t> unpacked(data.size());
    EXPECT_EQ(Dmod_Compression_Unpack("fastlz", unpacked.data(), unpacked.size(), packed.data(), packed.size()), data.size());
    EXPECT_EQ(unpacked, data);
}

TEST_F(DmodCompressionTest, DeflateRefusesWhatIsNotDeflate)
{
    const std::vector<uint8_t> data = Sample(5000);
    std::vector<uint8_t> packed;
    std::vector<uint8_t> out(data.size());

    ASSERT_GT(Pack("deflate", 6, data, packed), 0u);

    // Garbage
    std::vector<uint8_t> garbage(300);
    for( size_t i = 0; i < garbage.size(); i++ ) garbage[i] = (uint8_t)(i * 7 + 3);
    EXPECT_EQ(Dmod_Compression_Unpack("deflate", out.data(), out.size(), garbage.data(), garbage.size()), 0u);

    // Cut short
    EXPECT_EQ(Dmod_Compression_Unpack("deflate", out.data(), out.size(), packed.data(), packed.size() / 2), 0u);

    // An output buffer that is too small
    EXPECT_EQ(Dmod_Compression_Unpack("deflate", out.data(), out.size() - 1, packed.data(), packed.size()), 0u);

    EXPECT_EQ(g_LiveAllocations, 0);
}

TEST_F(DmodCompressionTest, DeflateDecoderWithoutMemory)
{
    const std::vector<uint8_t> data = Sample(5000);
    std::vector<uint8_t> packed;
    std::vector<uint8_t> out(data.size());

    ASSERT_GT(Pack("deflate", 6, data, packed), 0u);

    g_FailAllocations = true;
    EXPECT_EQ(Dmod_Compression_Unpack("deflate", out.data(), out.size(), packed.data(), packed.size()), 0u);
    g_FailAllocations = false;
    EXPECT_EQ(Dmod_Compression_Unpack("deflate", out.data(), out.size(), packed.data(), packed.size()), data.size());
}

TEST_F(DmodCompressionTest, NothingToCompress)
{
    uint8_t out[64];
    uint8_t in = 0;

    EXPECT_EQ(Dmod_Compression_Pack("deflate", 6, out, sizeof(out), &in, 0), 0u);
    EXPECT_EQ(Dmod_Compression_Unpack("deflate", out, sizeof(out), &in, 0), 0u);
    EXPECT_EQ(Dmod_Compression_GetMaxSize("deflate", 6, 0), 0u);
}
