#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <cstring>
#include "dmod.h"
#include "private/dmod_ldr.h"
#include "private/dmod_hlp.h"

class DmodReldynTest : public ::testing::Test
{
protected:
    alignas(size_t) unsigned char data[256] = {};
    Dmod_ModuleHeader_t header = {};
    Dmod_ModuleFooter_t footer = {};
    Dmod_Context_t context = {};

    void SetUp() override
    {
        header.PointerSize = sizeof(void*);
        context.Signature = DMOD_CONTEXT_SIGNATURE;
        strcpy(header.Name, "memory");
        context.Header = &header;
        context.Footer = &footer;
        context.Data = data;
        context.Size = sizeof(data);
        footer.Got.SectionStart = 128;
        footer.Got.SectionSize = 2 * sizeof(size_t);
        auto table = reinterpret_cast<size_t*>(data + footer.Got.SectionStart);
        table[0] = DMOD_RELDYN_MAGIC;
        table[1] = 0;
    }
};

// The published memory 0.9 Xtensa package has a nonzero GOT offset and zero
// size. Its unrelocated print_usage pointer (0x1a4) faults the ESP32-S3.
TEST_F(DmodReldynTest, RejectsEmptyRelocationSection)
{
    footer.Got.SectionSize = 0;
    EXPECT_FALSE(Dmod_Ldr_LoadGot(&context));
}

TEST_F(DmodReldynTest, RejectsMissingRelocationSection)
{
    footer.Got = {};
    EXPECT_FALSE(Dmod_Ldr_LoadGot(&context));
}

TEST_F(DmodReldynTest, AcceptsTableWithNoRelocations)
{
    EXPECT_TRUE(Dmod_Ldr_LoadGot(&context));
}

TEST_F(DmodReldynTest, RelocatesAddressFromTable)
{
    auto table = reinterpret_cast<size_t*>(data + footer.Got.SectionStart);
    footer.Got.SectionSize = 3 * sizeof(size_t);
    table[1] = 1;
    table[2] = 32;
    auto address = reinterpret_cast<size_t*>(data + 32);
    *address = 64;
    ASSERT_TRUE(Dmod_Ldr_LoadGot(&context));
    EXPECT_EQ(*address, reinterpret_cast<size_t>(data) + 64);
}

TEST_F(DmodReldynTest, AcceptsLegacyNonemptyRelocationTable)
{
    auto table = reinterpret_cast<size_t*>(data + footer.Got.SectionStart);
    footer.Got.SectionSize = sizeof(size_t);
    table[0] = 32;
    auto address = reinterpret_cast<size_t*>(data + 32);
    *address = 64;
    ASSERT_TRUE(Dmod_Ldr_LoadGot(&context));
    EXPECT_EQ(*address, reinterpret_cast<size_t>(data) + 64);
}
