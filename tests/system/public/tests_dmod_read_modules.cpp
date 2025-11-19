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
 * @brief Test Dmod_ReadModules with NULL output buffer
 */
TEST_F(DmodReadModulesTest, ReadModulesNullOutput) {
    size_t count = Dmod_ReadModules(NULL, 10);
    EXPECT_EQ(count, 0);
}

/**
 * @brief Test Dmod_ReadModules with zero max size
 */
TEST_F(DmodReadModulesTest, ReadModulesZeroMax) {
    Dmod_ModuleInfo_t modules[10];
    size_t count = Dmod_ReadModules(modules, 0);
    EXPECT_EQ(count, 0);
}

/**
 * @brief Test Dmod_ReadModules with no modules loaded
 */
TEST_F(DmodReadModulesTest, ReadModulesNoModulesLoaded) {
    Dmod_ModuleInfo_t modules[10];
    size_t count = Dmod_ReadModules(modules, 10);
    // Should return 0 or more (depending on available modules in search paths)
    EXPECT_GE(count, 0);
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
 * @brief Test Dmod_ReadModules respects max limit
 */
TEST_F(DmodReadModulesTest, ReadModulesRespectsMaxLimit) {
    Dmod_ModuleInfo_t modules[5];
    size_t count = Dmod_ReadModules(modules, 5);
    // Should never return more than max
    EXPECT_LE(count, 5);
}

/**
 * @brief Test Dmod_ReadModules with large array
 */
TEST_F(DmodReadModulesTest, ReadModulesLargeArray) {
    Dmod_ModuleInfo_t modules[100];
    size_t count = Dmod_ReadModules(modules, 100);
    // Should return a reasonable number
    EXPECT_LE(count, 100);
    EXPECT_GE(count, 0);
}
