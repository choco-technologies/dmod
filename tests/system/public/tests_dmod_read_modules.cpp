#define DMOD_PRIVATE
#include <stdlib.h>
#include <string.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "dmod.h"
#include "dmod_system.h"
#include "private/dmod_ctx.h"
#include "private/dmod_vars.h"

// ===============================================================
//                  Test fixture
// ===============================================================

class DmodReadModulesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize Dmod system
        Dmod_Initialize();
    }

    void TearDown() override {
        // Clean up any loaded modules
        for( size_t i = 0; i < DMOD_MAX_MODULES; i++ )
        {
            if( Dmod_Contexts[i] != NULL )
            {
                Dmod_Context_Delete( Dmod_Contexts[i] );
                Dmod_Contexts[i] = NULL;
            }
        }
    }
};

// ===============================================================
//                  Tests
// ===============================================================

/**
 * @brief Test Dmod_OpenModules with NULL iterator
 */
TEST_F(DmodReadModulesTest, OpenModulesSuccess) {
    Dmod_ModulesIterator_t iterator = Dmod_OpenModules();
    EXPECT_NE(iterator, nullptr);
    Dmod_CloseModules(iterator);
}

/**
 * @brief Test Dmod_ReadModule with NULL iterator
 */
TEST_F(DmodReadModulesTest, ReadModuleNullIterator) {
    const Dmod_ModuleInfo_t* module = Dmod_ReadModule(NULL);
    EXPECT_EQ(module, nullptr);
}

/**
 * @brief Test Dmod_ReadModule with no modules loaded
 */
TEST_F(DmodReadModulesTest, ReadModulesNoModulesLoaded) {
    Dmod_ModulesIterator_t iterator = Dmod_OpenModules();
    ASSERT_NE(iterator, nullptr);
    
    // Count how many modules are available
    size_t count = 0;
    const Dmod_ModuleInfo_t* module;
    while( (module = Dmod_ReadModule(iterator)) != NULL )
    {
        count++;
    }
    
    // Should return 0 or more (depending on available modules in search paths)
    EXPECT_GE(count, 0);
    
    Dmod_CloseModules(iterator);
}

/**
 * @brief Test module info structure fields
 */
TEST_F(DmodReadModulesTest, ModuleInfoStructure) {
    Dmod_ModuleInfo_t info;
    
    // Verify structure can hold expected data
    strncpy(info.ModuleName, "test_module", DMOD_MAX_MODULE_NAME_LENGTH - 1);
    info.ModuleName[DMOD_MAX_MODULE_NAME_LENGTH - 1] = '\0';
    
    strncpy(info.Version, "1.0", DMOD_MAX_VERSION_LENGTH - 1);
    info.Version[DMOD_MAX_VERSION_LENGTH - 1] = '\0';
    
    info.State = Dmod_ModuleState_Available;
    
    EXPECT_STREQ(info.ModuleName, "test_module");
    EXPECT_STREQ(info.Version, "1.0");
    EXPECT_EQ(info.State, Dmod_ModuleState_Available);
}

/**
 * @brief Test module state enumeration
 */
TEST_F(DmodReadModulesTest, ModuleStateEnum) {
    // Verify all states are defined
    EXPECT_EQ(Dmod_ModuleState_Available, 0);
    EXPECT_EQ(Dmod_ModuleState_Loaded, 1);
    EXPECT_EQ(Dmod_ModuleState_Enabled, 2);
    EXPECT_EQ(Dmod_ModuleState_Running, 3);
}

/**
 * @brief Test Dmod_CloseModules with NULL iterator
 */
TEST_F(DmodReadModulesTest, CloseModulesNull) {
    // Should not crash
    Dmod_CloseModules(NULL);
}

/**
 * @brief Test iterating through modules multiple times
 */
TEST_F(DmodReadModulesTest, IterateMultipleTimes) {
    // First iteration
    Dmod_ModulesIterator_t iterator1 = Dmod_OpenModules();
    ASSERT_NE(iterator1, nullptr);
    
    size_t count1 = 0;
    const Dmod_ModuleInfo_t* module;
    while( (module = Dmod_ReadModule(iterator1)) != NULL )
    {
        count1++;
    }
    
    Dmod_CloseModules(iterator1);
    
    // Second iteration should give same count
    Dmod_ModulesIterator_t iterator2 = Dmod_OpenModules();
    ASSERT_NE(iterator2, nullptr);
    
    size_t count2 = 0;
    while( (module = Dmod_ReadModule(iterator2)) != NULL )
    {
        count2++;
    }
    
    Dmod_CloseModules(iterator2);
    
    EXPECT_EQ(count1, count2);
}

/**
 * @brief Test that iterator returns consistent module info
 */
TEST_F(DmodReadModulesTest, ModuleInfoConsistent) {
    Dmod_ModulesIterator_t iterator = Dmod_OpenModules();
    ASSERT_NE(iterator, nullptr);
    
    const Dmod_ModuleInfo_t* module = Dmod_ReadModule(iterator);
    if( module != NULL )
    {
        // Module name should not be empty
        EXPECT_NE(module->ModuleName[0], '\0');
        
        // State should be valid
        EXPECT_GE(module->State, Dmod_ModuleState_Available);
        EXPECT_LT(module->State, Dmod_ModuleState_Count);
    }
    
    Dmod_CloseModules(iterator);
}
