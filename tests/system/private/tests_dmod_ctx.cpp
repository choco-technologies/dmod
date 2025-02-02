#define DMOD_PRIVATE
#include <stdlib.h>
#include <gtest/gtest.h>
#include "private/dmod_ctx.h"
#include "private/dmod_vars.h"
#include "dmod.h"

// ===============================================================
//                  Tests for Dmod_Context_New
// ===============================================================

/**
 * @brief loads DMF test file for testing
 */
static bool LoadDmfTestFile( void** outData, size_t* outSize )
{
    Dmod_Printf("Loading DMF test file: %s\n", DMOD_TEST_DMF_FILE);
    FILE* file = fopen(DMOD_TEST_DMF_FILE, "rb");
    if( file == NULL )
    {
        return false;
    }

    fseek(file, 0, SEEK_END);
    *outSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    *outData = Dmod_AlignedMalloc(*outSize, DMOD_STACK_ALIGNMENT);
    if( *outData == NULL )
    {
        fclose(file);
        return false;
    }

    fread(*outData, 1, *outSize, file);
    fclose(file);

    return true;
}

class DmodContextTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        memset(Dmod_Contexts, 0, sizeof(Dmod_Contexts));
    }

    void TearDown() override
    {
    }
};

/**
 * @brief Test for Dmod_Context_New
 * 
 * The test checks if the function creates a new context with the given data, 
 * assuming that the data is not NULL.
 */
TEST_F(DmodContextTest, NewWithData) 
{
    size_t fileSize = 1024;
    void* data = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);
    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Context_New
 * 
 * The test checks if the function creates a new context with the given data, 
 * assuming that the data is NULL.
 */
TEST_F(DmodContextTest, NewWithoutData)
{
    size_t fileSize = 1024;
    Dmod_Context_t* context = Dmod_Context_New(NULL, fileSize);
    ASSERT_NE(context, nullptr);
    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Context_New
 * 
 * The test checks if the function fails to create a new context with the given data, 
 * assuming that the data is NULL and the file size is 0.
 */
TEST_F(DmodContextTest, NewWithoutDataAndZeroFileSize)
{
    Dmod_Context_t* context = Dmod_Context_New(NULL, 0);
    ASSERT_EQ(context, nullptr);
}

// ===============================================================
//                  Tests for Dmod_Context_IsValid
// ===============================================================
/**
 * @brief Test for Dmod_Context_IsValid
 * 
 * The test checks if the function returns true for a valid context.
 */
TEST_F(DmodContextTest, IsValidTrue)
{
    size_t fileSize = 1024;
    void* data = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_TRUE(Dmod_Context_IsValid(context));
    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Context_IsValid
 * 
 * The test checks if the function returns false for an invalid context.
 */
TEST_F(DmodContextTest, IsValidFalse)
{
    Dmod_Context_t* context = nullptr;
    ASSERT_FALSE(Dmod_Context_IsValid(context));
}

// ===============================================================
//                  Tests for Dmod_Context_Delete
// ===============================================================
/**
 * @brief Test for Dmod_Context_Delete
 * 
 * The test checks if the function deletes a valid context.
 */
TEST_F(DmodContextTest, DeleteValidContext)
{
    size_t fileSize = 1024;
    void* data = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Context_Delete
 * 
 * The test checks if the function deletes an invalid context.
 */
TEST_F(DmodContextTest, DeleteInvalidContext)
{
    Dmod_Context_t* context = nullptr;
    Dmod_Context_Delete(context);
}

// ===============================================================
//                  Tests for Dmod_Context_GetModuleName
// ===============================================================
/**
 * @brief Test for Dmod_Context_GetModuleName
 * 
 * The test checks if the function returns the module name for a valid context.
 */
TEST_F(DmodContextTest, GetModuleNameValidContext)
{
    size_t fileSize = 1024;
    void* data = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_STREQ(Dmod_Context_GetModuleName(context), "Unknown");
    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Context_GetModuleName
 * 
 * The test checks if the function returns "Invalid" for an invalid context.
 */
TEST_F(DmodContextTest, GetModuleNameInvalidContext)
{
    Dmod_Context_t* context = nullptr;
    ASSERT_STREQ(Dmod_Context_GetModuleName(context), "Invalid");
}

// ===============================================================
//                  Tests for Dmod_Context_Add
// ===============================================================
/**
 * @brief Test for Dmod_Context_Add
 * 
 * The test checks if the function adds a valid context.
 */
TEST_F(DmodContextTest, AddValidContext)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));

    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);

    // Get the module name
    ASSERT_NE(context, nullptr);
    context->Header = reinterpret_cast<Dmod_ModuleHeader_t*>(data);
    const char* moduleName = Dmod_Context_GetModuleName(context);
    EXPECT_NE(moduleName, nullptr);
    EXPECT_STREQ(moduleName, "example_app");

    ASSERT_TRUE(Dmod_Context_Add(context));

    // Check if the context is added to the list
    Dmod_Context_t* contextFromList = Dmod_Context_Get(moduleName);
    EXPECT_EQ(context, contextFromList);

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Context_Add
 * 
 * The test checks if the function fails to add an invalid context.
 */
TEST_F(DmodContextTest, AddInvalidContext)
{
    Dmod_Context_t* context = nullptr;
    ASSERT_FALSE(Dmod_Context_Add(context));
}

// ===============================================================
//                  Tests for Dmod_Context_Remove
// ===============================================================
/**
 * @brief Test for Dmod_Context_Remove
 * 
 * The test checks if the function removes a valid context.
 */
TEST_F(DmodContextTest, RemoveValidContext)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));

    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);

    // Get the module name
    ASSERT_NE(context, nullptr);
    context->Header = reinterpret_cast<Dmod_ModuleHeader_t*>(data);
    const char* moduleName = Dmod_Context_GetModuleName(context);
    EXPECT_NE(moduleName, nullptr);
    EXPECT_STREQ(moduleName, "example_app");

    ASSERT_TRUE(Dmod_Context_Add(context));

    // Check if the context is added to the list
    Dmod_Context_t* contextFromList = Dmod_Context_Get(moduleName);
    EXPECT_EQ(context, contextFromList);

    ASSERT_TRUE(Dmod_Context_Remove(context));

    // Check if the context is removed from the list
    contextFromList = Dmod_Context_Get(moduleName);
    EXPECT_EQ(contextFromList, nullptr);

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Context_Remove
 * 
 * The test checks if the function removes an invalid context.
 */
TEST_F(DmodContextTest, RemoveInvalidContext)
{
    Dmod_Context_t* context = nullptr;
    ASSERT_FALSE(Dmod_Context_Remove(context));
}

// ===============================================================
//                  Tests for Dmod_Context_Get
// ===============================================================
/**
 * @brief Test for Dmod_Context_Get
 * 
 * The test checks if the function returns a valid context.
 */
TEST_F(DmodContextTest, GetValidContext)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));

    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);

    // Set the module name
    ASSERT_NE(context, nullptr);
    context->Header = reinterpret_cast<Dmod_ModuleHeader_t*>(data);
    const char* moduleName = Dmod_Context_GetModuleName(context);
    EXPECT_NE(moduleName, nullptr);
    EXPECT_STREQ(moduleName, "example_app");

    // Add the context to the list
    EXPECT_TRUE(Dmod_Context_Add(context));

    ASSERT_EQ(Dmod_Context_Get(moduleName), context);
    EXPECT_TRUE(Dmod_Context_Remove(context));
    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Context_Get
 * 
 * The test checks if the function returns NULL for an invalid context.
 */
TEST_F(DmodContextTest, GetInvalidContext)
{
    Dmod_Context_t* context = nullptr;
    ASSERT_EQ(Dmod_Context_Get("Unknown"), context);
}
