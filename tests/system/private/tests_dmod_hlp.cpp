#define DMOD_PRIVATE
#include <stdlib.h>
#include <string.h>
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

/**
 * @brief Test for Dmod_Hlp_InitPointer
 * 
 * The test checks if the function fails to initialize a pointer with the given offset.
 */
TEST_F(DmodHlpTest, InitPointerFail)
{
    size_t fileSize = 1024;
    void* data = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    ASSERT_FALSE(Dmod_Hlp_InitPointer(context, nullptr, "Pointer"));

    void* pointer = reinterpret_cast<void*>(0);
    ASSERT_TRUE(Dmod_Hlp_InitPointer(context, &pointer, "Pointer")); // pointer is NULL and should not be changed

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Hlp_InitPointer
 * 
 * The test checks if the function fails to initialize a pointer with the given offset.
 */
TEST_F(DmodHlpTest, InitPointerFailOffset)
{
    size_t fileSize = 1024;
    void* data = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    uint32_t offset = 2000;
    void* pointer = reinterpret_cast<void*>(offset);
    ASSERT_FALSE(Dmod_Hlp_InitPointer(context, &pointer, "Pointer"));

    Dmod_Context_Delete(context);
}

// ===============================================================
//                  Tests for Dmod_Hlp_PrepareModulesSearchNodes
// ===============================================================

/**
 * @brief Helper function to check if a path exists in search nodes
 * 
 * @param tail Tail of the search node list
 * @param path Path to search for
 * 
 * @return true if path is found, false otherwise
 */
static bool SearchNodesContainPath(Dmod_SearchNode_t* tail, const char* path)
{
    Dmod_SearchNode_t* current = tail;
    while (current != NULL)
    {
        if (strcmp(current->Path, path) == 0)
        {
            return true;
        }
        current = current->Prev;
    }
    return false;
}

/**
 * @brief Test for Dmod_Hlp_PrepareModulesSearchNodes with PATH support
 * 
 * The test checks if the function includes PATH environment variable paths.
 */
TEST_F(DmodHlpTest, PrepareModulesSearchNodesIncludesPATH)
{
    // Set a custom PATH with a unique test path
    const char* testPath = "/tmp/dmod_test_path";
    const char* originalPath = Dmod_GetEnv("PATH");
    
    // Create a new PATH with our test path
    char newPath[4096];
    if (originalPath != NULL)
    {
        snprintf(newPath, sizeof(newPath), "%s%s%s", testPath, DMOD_ARRAY_SEP, originalPath);
    }
    else
    {
        snprintf(newPath, sizeof(newPath), "%s", testPath);
    }
    
    // Set the new PATH
    Dmod_SetEnv("PATH", newPath, 1);
    
    // Get search nodes
    Dmod_SearchNode_t* tail = Dmod_Hlp_PrepareModulesSearchNodes();
    ASSERT_NE(tail, nullptr);
    
    // Check if our test path is included
    bool found = SearchNodesContainPath(tail, testPath);
    ASSERT_TRUE(found) << "PATH directories should be included in module search paths";
    
    // Clean up
    Dmod_Hlp_FreeSearchPathList(tail);
    
    // Restore original PATH
    if (originalPath != NULL)
    {
        Dmod_SetEnv("PATH", originalPath, 1);
    }
}

/**
 * @brief Test for Dmod_Hlp_PrepareModulesSearchNodes 
 * 
 * The test checks that the function returns non-null search nodes.
 */
TEST_F(DmodHlpTest, PrepareModulesSearchNodesNotNull)
{
    Dmod_SearchNode_t* tail = Dmod_Hlp_PrepareModulesSearchNodes();
    ASSERT_NE(tail, nullptr);
    
    // Clean up
    Dmod_Hlp_FreeSearchPathList(tail);
}
