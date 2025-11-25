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
 * The test checks if the function returns false for a partial module name match.
 * For example, "Dm" should not match "Dmod".
 */
TEST_F(DmodMgrTest, IsNotPartialModuleNameMatch)
{
    // "Dm" is a prefix of "Dmod" but should not match
    ASSERT_FALSE(Dmod_Mgr_IsSystemModule("Dm"));
    // "D" is a prefix of "Dmod" but should not match
    ASSERT_FALSE(Dmod_Mgr_IsSystemModule("D"));
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