#define DMOD_PRIVATE
#include <stdlib.h>
#include <gtest/gtest.h>
#include "private/dmod_ctx.h"
#include "private/dmod_ldr.h"
#include "private/dmod_mgr.h"
#include "private/dmod_vars.h"
#include "dmod.h"

class DmodMgrTest : public ::testing::Test
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

// ===============================================================
//                  Tests for Dmod_Mgr_IsSystemModule
// ===============================================================

/**
 * @brief Test for Dmod_Mgr_IsSystemModule
 * 
 * The test checks if the function returns true for a system module.
 */
TEST_F(DmodMgrTest, IsSystemModule)
{
    ASSERT_TRUE(Dmod_Mgr_IsSystemModule("Dmod"));
}

/**
 * @brief Test for Dmod_Mgr_IsSystemModule
 * 
 * The test checks if the function returns false for a non-system module.
 */
TEST_F(DmodMgrTest, IsNotSystemModule)
{
    ASSERT_FALSE(Dmod_Mgr_IsSystemModule("Test"));
}

/**
 * @brief Test for Dmod_Mgr_IsSystemModule
 * 
 * The test checks if the function returns false for an empty module name.
 */
TEST_F(DmodMgrTest, IsEmptyModule)
{
    ASSERT_FALSE(Dmod_Mgr_IsSystemModule(""));
}

/**
 * @brief Test for Dmod_Mgr_IsSystemModule
 * 
 * The test checks if the function returns false for a NULL module name.
 */
TEST_F(DmodMgrTest, IsNullModule)
{
    ASSERT_FALSE(Dmod_Mgr_IsSystemModule(NULL));
}

/**
 * @brief Test for Dmod_Mgr_InitSystemModules
 *
 * The test checks if the list of the system modules can be built more than
 * once, and if the result of Dmod_Mgr_IsSystemModule does not change with it.
 */
TEST_F(DmodMgrTest, InitSystemModules)
{
    ASSERT_TRUE(Dmod_Mgr_InitSystemModules());
    ASSERT_TRUE(Dmod_Mgr_InitSystemModules());
    ASSERT_TRUE(Dmod_Mgr_IsSystemModule("Dmod"));
    ASSERT_FALSE(Dmod_Mgr_IsSystemModule("Test"));
}

/**
 * @brief Test for Dmod_Mgr_DeinitSystemModules
 *
 * The test checks if Dmod_Mgr_IsSystemModule builds the list of the system
 * modules again when it has been freed.
 */
TEST_F(DmodMgrTest, IsSystemModuleAfterDeinit)
{
    Dmod_Mgr_DeinitSystemModules();
    ASSERT_TRUE(Dmod_Mgr_IsSystemModule("Dmod"));
    ASSERT_FALSE(Dmod_Mgr_IsSystemModule("Test"));
}

/**
 * @brief Test for Dmod_Mgr_IsSystemModule
 *
 * The test checks if every module named by a built-in API signature is
 * reported as a system module.
 */
TEST_F(DmodMgrTest, IsSystemModuleForEveryBuiltinApi)
{
    Dmod_BuiltinInputApi.SectionSize = (size_t)((uint8_t*)&__dmod_inputs_end - (uint8_t*)&__dmod_inputs_start);
    size_t numberOfEntries = Dmod_Api_GetNumberOfEntries(&Dmod_BuiltinInputApi);
    ASSERT_GT(numberOfEntries, 0u);
    size_t checked = 0;
    for(size_t i = 0; i < numberOfEntries; i++)
    {
        const char* signature = Dmod_BuiltinInputApi.InputSection->Entries[i].Signature;
        char moduleName[DMOD_MAX_MODULE_NAME_LENGTH] = {0};
        if(!Dmod_ApiSignature_IsValid(signature)
        || Dmod_ApiSignature_IsTest(signature)
        || strncmp(signature, DMOD_IRQ_SIGNATURE_PREFIX, sizeof(DMOD_IRQ_SIGNATURE_PREFIX) - 1) == 0
        || !Dmod_ApiSignature_ReadModuleName(signature, moduleName, sizeof(moduleName) - 1)
        || moduleName[0] == '\0')
        {
            continue;
        }
        EXPECT_TRUE(Dmod_Mgr_IsSystemModule(moduleName)) << moduleName;
        checked++;
    }
    ASSERT_GT(checked, 0u);
}

// ===============================================================
//                  Tests for Dmod_Mgr_IsLoaded
// ===============================================================
/**
 * @brief Test for Dmod_Mgr_IsLoaded
 * 
 * The test checks if the function returns true for a system module.
 */
TEST_F(DmodMgrTest, IsLoadedSystemModule)
{
    ASSERT_TRUE(Dmod_Mgr_IsLoaded("Dmod"));
}

/**
 * @brief Test for Dmod_Mgr_IsLoaded
 * 
 * The test checks if the function returns false for a non-loaded module.
 */
TEST_F(DmodMgrTest, IsNotLoadedModule)
{
    ASSERT_FALSE(Dmod_Mgr_IsLoaded("Test"));
}

/**
 * @brief Test for Dmod_Mgr_IsLoaded
 * 
 * The test checks if the function returns false for an empty module name.
 */
TEST_F(DmodMgrTest, IsEmptyLoadedModule)
{
    ASSERT_FALSE(Dmod_Mgr_IsLoaded(""));
}

/**
 * @brief Test for Dmod_Mgr_IsLoaded
 * 
 * The test checks if the function returns false for a NULL module name.
 */
TEST_F(DmodMgrTest, IsNullLoadedModule)
{
    ASSERT_FALSE(Dmod_Mgr_IsLoaded(NULL));
}

/**
 * @brief Test for Dmod_Mgr_IsLoaded
 * 
 * The test checks if the function returns true for a non-system module.
 */
TEST_F(DmodMgrTest, IsLoadedModule)
{
    Dmod_Context_t* context = Dmod_LoadFile(DMOD_TEST_DMF_LIB_FILE);
    ASSERT_NE( context, nullptr );
    ASSERT_TRUE(Dmod_Context_Add(context));
    ASSERT_TRUE(Dmod_Mgr_IsLoaded(context->Header->Name));
    Dmod_Context_Delete(context);
}

// ===============================================================
//                  Tests for Dmod_Mgr_IsEnabled
// ===============================================================

/**
 * @brief Test for Dmod_Mgr_IsEnabled
 * 
 * The test checks if the function returns true for a system module.
 */
TEST_F(DmodMgrTest, IsEnabledSystemModule)
{
    ASSERT_TRUE(Dmod_Mgr_IsEnabled("Dmod"));
}

/**
 * @brief Test for Dmod_Mgr_IsEnabled
 * 
 * The test checks if the function returns false for a non-enabled module.
 */
TEST_F(DmodMgrTest, IsNotEnabledModule)
{
    ASSERT_FALSE(Dmod_Mgr_IsEnabled("Test"));
}

/**
 * @brief Test for Dmod_Mgr_IsEnabled
 * 
 * The test checks if the function returns false for an empty module name.
 */
TEST_F(DmodMgrTest, IsEmptyEnabledModule)
{
    ASSERT_FALSE(Dmod_Mgr_IsEnabled(""));
}

/**
 * @brief Test for Dmod_Mgr_IsEnabled
 * 
 * The test checks if the function returns false for a NULL module name.
 */
TEST_F(DmodMgrTest, IsNullEnabledModule)
{
    ASSERT_FALSE(Dmod_Mgr_IsEnabled(NULL));
}

/**
 * @brief Test for Dmod_Mgr_IsEnabled
 * 
 * The test checks if the function returns true for a non-system module.
 */
TEST_F(DmodMgrTest, IsEnabledModule)
{
    Dmod_Context_t* context = Dmod_LoadFile(DMOD_TEST_DMF_LIB_FILE);
    ASSERT_NE( context, nullptr );
    ASSERT_TRUE(Dmod_Context_Add(context));
    ASSERT_FALSE(Dmod_Mgr_IsEnabled(context->Header->Name));
    context->Enabled = true;
    ASSERT_TRUE(Dmod_Mgr_IsEnabled(context->Header->Name));
    Dmod_Context_Delete(context);
}