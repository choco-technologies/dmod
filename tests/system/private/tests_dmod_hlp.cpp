#define DMOD_PRIVATE
#include <stdlib.h>
#include <gtest/gtest.h>
#include "private/dmod_ctx.h"
#include "private/dmod_hlp.h"
#include "dmod.h"

// ===============================================================
//                  Tests for Dmod_Hlp_InitPointer
// ===============================================================

class DmodHlpTest : public ::testing::Test
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
 * @brief Test for Dmod_Hlp_InitPointer
 * 
 * The test checks if the function initializes a pointer with the given offset.
 */
TEST_F(DmodHlpTest, InitPointer)
{
    size_t fileSize = 1024;
    void* data = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    uint32_t offset = 100;
    void* expectedPointer = &(reinterpret_cast<uint8_t*>(data)[offset]);
    void* pointer = reinterpret_cast<void*>(offset);
    ASSERT_TRUE(Dmod_Hlp_InitPointer(context, &pointer, "Pointer"));
    ASSERT_EQ(pointer, expectedPointer);

    Dmod_Context_Delete(context);
}