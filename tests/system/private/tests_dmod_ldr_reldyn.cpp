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

// dmntp 0.4 for the ESP32-S3 (built on 2 November 2026, before the magic): two entries of the GOT that the linker made (the
// addresses 0x92c and 0xb00 of its functions) and then the table, all in .got without the magic. Taken as offsets, the entries
// would make the loader add the address of the load to two words of the code and leave the entries unrelocated: the module
// passed 0x92c to dmdhcp as a callback and the system faulted. It has to be refused, and nothing of the module changed.
TEST_F(DmodReldynTest, RejectsLegacyTableWithLinkerEntries)
{
    auto table = reinterpret_cast<size_t*>(data + footer.Got.SectionStart);
    footer.Got.SectionSize = 4 * sizeof(size_t);
    table[0] = 96;                              // a function's address (an entry of the linker's GOT)
    table[1] = 112;
    table[2] = 32;                              // the table
    table[3] = 48;
    auto code = reinterpret_cast<size_t*>(data + 96);
    auto literal = reinterpret_cast<size_t*>(data + 32);
    *code = 0x12345678;
    *literal = 64;
    EXPECT_FALSE(Dmod_Ldr_LoadGot(&context));
    EXPECT_EQ(*code, 0x12345678u);
    EXPECT_EQ(*literal, 64u);
    EXPECT_EQ(table[0], 96u);
}

TEST_F(DmodReldynTest, RejectsUnorderedTable)
{
    auto table = reinterpret_cast<size_t*>(data + footer.Got.SectionStart);
    footer.Got.SectionSize = 4 * sizeof(size_t);
    table[1] = 2;
    table[2] = 48;
    table[3] = 32;
    auto first = reinterpret_cast<size_t*>(data + 48);
    *first = 64;
    EXPECT_FALSE(Dmod_Ldr_LoadGot(&context));
    EXPECT_EQ(*first, 64u);                     // nothing was applied before the table was found wrong
}

// The current format: the entries of the linker's GOT before the magic are relocated, the table is applied
TEST_F(DmodReldynTest, RelocatesLinkerEntriesBeforeTheMagic)
{
    auto table = reinterpret_cast<size_t*>(data + footer.Got.SectionStart);
    footer.Got.SectionSize = 4 * sizeof(size_t);
    table[0] = 96;                              // an entry of the linker's GOT
    table[1] = DMOD_RELDYN_MAGIC;
    table[2] = 1;
    table[3] = 32;
    auto literal = reinterpret_cast<size_t*>(data + 32);
    *literal = 64;
    ASSERT_TRUE(Dmod_Ldr_LoadGot(&context));
    EXPECT_EQ(table[0], reinterpret_cast<size_t>(data) + 96);
    EXPECT_EQ(*literal, reinterpret_cast<size_t>(data) + 64);
}
