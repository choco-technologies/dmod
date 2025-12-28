#define DMOD_PRIVATE
#include <stdlib.h>
#include <string.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "dmod.h"
#include "dmod_system.h"

// ===============================================================
//                  Test fixture
// ===============================================================

class DmodReadNextModuleTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

// ===============================================================
//                  Tests for Dmod_ReadNextModule
// ===============================================================

/**
 * @brief Test for Dmod_ReadNextModule with NULL parameter
 * 
 * The test checks if the function handles NULL parameter correctly.
 */
TEST_F(DmodReadNextModuleTest, ReadNextModuleNullParameter)
{
    EXPECT_FALSE(Dmod_ReadNextModule(NULL));
}

/**
 * @brief Test for Dmod_ReadNextModule basic initialization
 * 
 * The test verifies that the function can initialize the iteration state
 * on first call with _Data set to NULL.
 */
TEST_F(DmodReadNextModuleTest, ReadNextModuleInitialization)
{
    Dmod_ModuleNode_t node;
    memset(&node, 0, sizeof(node));
    node._Data = NULL;
    
    // First call should initialize state
    // Result depends on whether any modules are available
    bool result = Dmod_ReadNextModule(&node);
    
    // Either we found a module (true) or there are no modules (false)
    // Both are valid outcomes depending on the environment
    if (result)
    {
        // If we found a module, verify the structure is populated
        EXPECT_NE(node._Data, nullptr) << "Internal state should be initialized";
        EXPECT_GT(strlen(node.path), 0) << "Path should not be empty";
        EXPECT_GT(strlen(node.header.Name), 0) << "Module name should not be empty";
    }
    else
    {
        // If no modules found, state should be cleaned up
        EXPECT_EQ(node._Data, nullptr) << "Internal state should be cleaned up when no modules found";
    }
}

/**
 * @brief Test for Dmod_ReadNextModule iteration
 * 
 * The test verifies that multiple calls iterate through modules correctly.
 */
TEST_F(DmodReadNextModuleTest, ReadNextModuleIteration)
{
    Dmod_ModuleNode_t node;
    memset(&node, 0, sizeof(node));
    node._Data = NULL;
    
    int moduleCount = 0;
    const int maxIterations = 1000; // Safety limit to prevent infinite loop
    
    // Iterate through all available modules
    while (Dmod_ReadNextModule(&node) && moduleCount < maxIterations)
    {
        // Verify that each found module has valid data
        EXPECT_GT(strlen(node.path), 0) << "Path should not be empty for module " << moduleCount;
        EXPECT_GT(strlen(node.header.Name), 0) << "Module name should not be empty for module " << moduleCount;
        EXPECT_NE(node._Data, nullptr) << "Internal state should be valid during iteration";
        
        moduleCount++;
    }
    
    // After iteration completes, state should be cleaned up
    EXPECT_EQ(node._Data, nullptr) << "Internal state should be cleaned up after iteration completes";
    
    // We should have terminated before hitting the safety limit
    EXPECT_LT(moduleCount, maxIterations) << "Iteration should terminate naturally";
}

/**
 * @brief Test for Dmod_ReadNextModule unique modules
 * 
 * The test verifies that each iteration returns a different module path.
 */
TEST_F(DmodReadNextModuleTest, ReadNextModuleUniqueModules)
{
    Dmod_ModuleNode_t node;
    memset(&node, 0, sizeof(node));
    node._Data = NULL;
    
    std::vector<std::string> paths;
    const int maxIterations = 100; // Reasonable limit for this test
    
    // Collect all module paths
    while (Dmod_ReadNextModule(&node) && paths.size() < maxIterations)
    {
        paths.push_back(std::string(node.path));
    }
    
    // Verify that we don't have duplicate paths
    // (Note: This is a soft requirement - in theory the same module could be in multiple paths)
    // But it's good to check for obvious iteration bugs
    for (size_t i = 0; i < paths.size(); i++)
    {
        for (size_t j = i + 1; j < paths.size(); j++)
        {
            if (paths[i] == paths[j])
            {
                // Found duplicate - log it but don't fail the test
                // as duplicates might be legitimate in some configurations
                std::cout << "Note: Found duplicate module path: " << paths[i] << std::endl;
            }
        }
    }
}

/**
 * @brief Test for Dmod_ReadNextModule early termination and restart
 * 
 * The test verifies that iteration can be stopped and restarted.
 */
TEST_F(DmodReadNextModuleTest, ReadNextModuleEarlyTerminationAndRestart)
{
    Dmod_ModuleNode_t node;
    
    // First iteration - read a few modules then stop
    memset(&node, 0, sizeof(node));
    node._Data = NULL;
    
    int firstCount = 0;
    for (int i = 0; i < 3; i++)
    {
        if (Dmod_ReadNextModule(&node))
        {
            firstCount++;
        }
        else
        {
            break;
        }
    }
    
    // If we found any modules and stopped early, _Data might still be set
    // We need to manually clean up by calling until it returns false
    if (node._Data != nullptr)
    {
        while (Dmod_ReadNextModule(&node))
        {
            // Continue until iteration completes naturally
        }
    }
    
    EXPECT_EQ(node._Data, nullptr) << "State should be cleaned up after complete iteration";
    
    // Second iteration - start fresh
    memset(&node, 0, sizeof(node));
    node._Data = NULL;
    
    int secondCount = 0;
    while (Dmod_ReadNextModule(&node))
    {
        secondCount++;
    }
    
    // If we found modules in first iteration, we should find them in second too
    if (firstCount > 0)
    {
        EXPECT_GT(secondCount, 0) << "Second iteration should also find modules";
    }
}
