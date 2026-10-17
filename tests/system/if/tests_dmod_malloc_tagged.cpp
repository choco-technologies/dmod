#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <string.h>
#include "dmod_sal.h"

// ===============================================================
//                  Tests for Dmod_MallocTaggedEx
// ===============================================================

/**
 * @brief Without heaps of kinds (dmheap gives them on a device) memory of any
 *        kind is the default memory - a module asking for one works anywhere
 */
TEST(DmodMallocTaggedTest, WithoutHeapsOfKindsTheDefaultMemory)
{
    void* fast = Dmod_MallocTaggedEx("fast,dma", 64, "tests_dmod_malloc_tagged");
    ASSERT_NE(fast, nullptr);
    memset(fast, 0xA5, 64);
    Dmod_FreeEx(fast, false);

    void* any = Dmod_MallocTaggedEx(NULL, 32, "tests_dmod_malloc_tagged");
    ASSERT_NE(any, nullptr);
    Dmod_FreeEx(any, false);

    any = Dmod_MallocTaggedEx("", 32, "tests_dmod_malloc_tagged");
    ASSERT_NE(any, nullptr);
    Dmod_FreeEx(any, false);
}
