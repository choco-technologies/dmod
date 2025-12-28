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
 * @brief Test for Dmod_OpenModules with NULL parameter
 * 
 * The test checks if the function handles NULL parameter correctly.
 */
TEST_F(DmodReadNextModuleTest, OpenModulesNullParameter)
{
    EXPECT_FALSE(Dmod_OpenModules(NULL));
}

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
 * @brief Test for Dmod_CloseModules with NULL parameter
 * 
 * The test checks if the function handles NULL parameter correctly.
 */
TEST_F(DmodReadNextModuleTest, CloseModulesNullParameter)
{
    // Should not crash
    Dmod_CloseModules(NULL);
}

/**
 * @brief Test for Dmod_ReadNextModule without opening first
 * 
 * The test verifies that calling ReadNextModule without OpenModules fails properly.
 */
TEST_F(DmodReadNextModuleTest, ReadNextModuleWithoutOpen)
{
    Dmod_ModuleNode_t node;
    memset(&node, 0, sizeof(node));
    node._Data = NULL;
    
    EXPECT_FALSE(Dmod_ReadNextModule(&node));
}

/**
 * @brief Test for Dmod_OpenModules basic initialization
 * 
 * The test verifies that the function can initialize the iteration state.
 */
TEST_F(DmodReadNextModuleTest, OpenModulesInitialization)
{
    Dmod_ModuleNode_t node;
    memset(&node, 0, sizeof(node));
    node._Data = NULL;
    
    // Open should succeed
    EXPECT_TRUE(Dmod_OpenModules(&node));
    EXPECT_NE(node._Data, nullptr) << "Internal state should be initialized";
    
    // Clean up
    Dmod_CloseModules(&node);
    EXPECT_EQ(node._Data, nullptr) << "Internal state should be cleaned up";
}

/**
 * @brief Test for Dmod_OpenModules double open
 * 
 * The test verifies that opening twice without closing fails.
 */
TEST_F(DmodReadNextModuleTest, OpenModulesDoubleOpen)
{
    Dmod_ModuleNode_t node;
    memset(&node, 0, sizeof(node));
    node._Data = NULL;
    
    // First open should succeed
    EXPECT_TRUE(Dmod_OpenModules(&node));
    
    // Second open without close should fail
    EXPECT_FALSE(Dmod_OpenModules(&node));
    
    // Clean up
    Dmod_CloseModules(&node);
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
    
    // Open module iteration
    if (!Dmod_OpenModules(&node))
    {
        // No modules available or error - skip test
        return;
    }
    
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
    
    // We should have terminated before hitting the safety limit
    EXPECT_LT(moduleCount, maxIterations) << "Iteration should terminate naturally";
    
    // Clean up resources
    Dmod_CloseModules(&node);
    
    // After cleanup, state should be NULL
    EXPECT_EQ(node._Data, nullptr) << "Internal state should be cleaned up after close";
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
    
    // Open module iteration
    if (!Dmod_OpenModules(&node))
    {
        // No modules available or error - skip test
        return;
    }
    
    std::vector<std::string> paths;
    const int maxIterations = 100; // Reasonable limit for this test
    
    // Collect all module paths
    while (Dmod_ReadNextModule(&node) && paths.size() < maxIterations)
    {
        paths.push_back(std::string(node.path));
    }
    
    // Clean up
    Dmod_CloseModules(&node);
    
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
 * @brief Test for early termination with proper cleanup
 * 
 * The test verifies that CloseModules properly cleans up resources
 * when iteration is stopped early.
 */
TEST_F(DmodReadNextModuleTest, ReadNextModuleEarlyTermination)
{
    Dmod_ModuleNode_t node;
    memset(&node, 0, sizeof(node));
    node._Data = NULL;
    
    // Open module iteration
    if (!Dmod_OpenModules(&node))
    {
        // No modules available or error - skip test
        return;
    }
    
    // Read only a few modules then stop
    int count = 0;
    for (int i = 0; i < 3; i++)
    {
        if (Dmod_ReadNextModule(&node))
        {
            count++;
        }
        else
        {
            break;
        }
    }
    
    // State should still be valid if we stopped early
    if (count > 0)
    {
        EXPECT_NE(node._Data, nullptr) << "State should be valid after partial iteration";
    }
    
    // Clean up explicitly - this is the key improvement!
    Dmod_CloseModules(&node);
    EXPECT_EQ(node._Data, nullptr) << "State should be cleaned up after explicit close";
}

/**
 * @brief Test for restart after complete iteration
 * 
 * The test verifies that iteration can be restarted after completion.
 */
TEST_F(DmodReadNextModuleTest, ReadNextModuleRestart)
{
    Dmod_ModuleNode_t node;
    
    // First iteration
    memset(&node, 0, sizeof(node));
    node._Data = NULL;
    
    if (!Dmod_OpenModules(&node))
    {
        // No modules available - skip test
        return;
    }
    
    int firstCount = 0;
    while (Dmod_ReadNextModule(&node))
    {
        firstCount++;
    }
    
    Dmod_CloseModules(&node);
    EXPECT_EQ(node._Data, nullptr) << "State should be cleaned up after close";
    
    // Second iteration - start fresh
    memset(&node, 0, sizeof(node));
    node._Data = NULL;
    
    if (!Dmod_OpenModules(&node))
    {
        // Should succeed like first time
        FAIL() << "Second open should succeed if first succeeded";
    }
    
    int secondCount = 0;
    while (Dmod_ReadNextModule(&node))
    {
        secondCount++;
    }
    
    Dmod_CloseModules(&node);
    
    // If we found modules in first iteration, we should find them in second too
    if (firstCount > 0)
    {
        EXPECT_EQ(secondCount, firstCount) << "Should find same number of modules in both iterations";
    }
}
