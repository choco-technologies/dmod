#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <string.h>
#include "dmod.h"
#include "private/dmod_vars.h"

// ===============================================================
//   Tests for Dmod_SetModuleLogLevel and Dmod_CheckModuleLogLevel
// ===============================================================

class DmodModuleLogLevelTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Release any nodes allocated by previous tests so each test
         * starts with a clean per-module list. */
        Dmod_ModuleLogLevel_t* node = Dmod_ModuleLogLevels;
        while( node != NULL )
        {
            Dmod_ModuleLogLevel_t* next = node->Next;
            Dmod_FreeEx(node, false);
            node = next;
        }
        Dmod_ModuleLogLevels = NULL;
    }
};

/**
 * @brief SetModuleLogLevel stores the level and CheckModuleLogLevel reflects it
 */
TEST_F(DmodModuleLogLevelTest, SetAndCheckModuleLogLevel)
{
    Dmod_SetModuleLogLevel("testmod", Dmod_LogLevel_Info);

    EXPECT_TRUE( Dmod_CheckModuleLogLevel("testmod", Dmod_LogLevel_Error) );
    EXPECT_TRUE( Dmod_CheckModuleLogLevel("testmod", Dmod_LogLevel_Warn) );
    EXPECT_TRUE( Dmod_CheckModuleLogLevel("testmod", Dmod_LogLevel_Info) );
    EXPECT_FALSE( Dmod_CheckModuleLogLevel("testmod", Dmod_LogLevel_Verbose) );
}

/**
 * @brief When no per-module level is set the global level is used as fallback
 */
TEST_F(DmodModuleLogLevelTest, FallsBackToGlobalLevel)
{
    Dmod_LogLevel_t savedGlobal = Dmod_LogLevel;
    Dmod_LogLevel = Dmod_LogLevel_Warn;

    /* "unknownmod" has no per-module entry → must follow global Warn level */
    EXPECT_TRUE(  Dmod_CheckModuleLogLevel("unknownmod", Dmod_LogLevel_Error) );
    EXPECT_TRUE(  Dmod_CheckModuleLogLevel("unknownmod", Dmod_LogLevel_Warn) );
    EXPECT_FALSE( Dmod_CheckModuleLogLevel("unknownmod", Dmod_LogLevel_Info) );
    EXPECT_FALSE( Dmod_CheckModuleLogLevel("unknownmod", Dmod_LogLevel_Verbose) );

    Dmod_LogLevel = savedGlobal;
}

/**
 * @brief NULL module name falls back to the global log level
 */
TEST_F(DmodModuleLogLevelTest, NullModuleNameUsesGlobal)
{
    Dmod_LogLevel_t savedGlobal = Dmod_LogLevel;
    Dmod_LogLevel = Dmod_LogLevel_Warn;

    EXPECT_TRUE(  Dmod_CheckModuleLogLevel(NULL, Dmod_LogLevel_Warn) );
    EXPECT_FALSE( Dmod_CheckModuleLogLevel(NULL, Dmod_LogLevel_Verbose) );

    Dmod_LogLevel = savedGlobal;
}

/**
 * @brief Per-module level overrides the global level
 */
TEST_F(DmodModuleLogLevelTest, ModuleLevelOverridesGlobal)
{
    Dmod_LogLevel_t savedGlobal = Dmod_LogLevel;
    Dmod_LogLevel = Dmod_LogLevel_Error; /* global: only errors */

    Dmod_SetModuleLogLevel("verbose_module", Dmod_LogLevel_Verbose);

    /* The module itself should log everything */
    EXPECT_TRUE( Dmod_CheckModuleLogLevel("verbose_module", Dmod_LogLevel_Verbose) );
    EXPECT_TRUE( Dmod_CheckModuleLogLevel("verbose_module", Dmod_LogLevel_Info) );

    /* Another module with no override still follows global Error level */
    EXPECT_FALSE( Dmod_CheckModuleLogLevel("other_module", Dmod_LogLevel_Warn) );

    Dmod_LogLevel = savedGlobal;
}

/**
 * @brief Updating an existing module entry changes the stored level
 */
TEST_F(DmodModuleLogLevelTest, UpdateExistingModuleLevel)
{
    Dmod_SetModuleLogLevel("updatemod", Dmod_LogLevel_Verbose);
    EXPECT_TRUE( Dmod_CheckModuleLogLevel("updatemod", Dmod_LogLevel_Verbose) );

    Dmod_SetModuleLogLevel("updatemod", Dmod_LogLevel_Error);
    EXPECT_TRUE(  Dmod_CheckModuleLogLevel("updatemod", Dmod_LogLevel_Error) );
    EXPECT_FALSE( Dmod_CheckModuleLogLevel("updatemod", Dmod_LogLevel_Warn) );

    /* Verify only one node was created */
    int count = 0;
    Dmod_ModuleLogLevel_t* node = Dmod_ModuleLogLevels;
    while( node != NULL ) { count++; node = node->Next; }
    EXPECT_EQ(count, 1);
}

/**
 * @brief Environment variable <UPPERCASE_MODULE_NAME>_LOG_LEVEL is honoured
 */
TEST_F(DmodModuleLogLevelTest, EnvVarSetsModuleLogLevel)
{
    /* Set the env var for module "envmod" → ENVMOD_LOG_LEVEL=info */
    Dmod_SetEnv("ENVMOD_LOG_LEVEL", "info", 1);

    /* First call should pick up the env var and cache it */
    EXPECT_TRUE(  Dmod_CheckModuleLogLevel("envmod", Dmod_LogLevel_Info) );
    EXPECT_FALSE( Dmod_CheckModuleLogLevel("envmod", Dmod_LogLevel_Verbose) );

    /* Cleanup */
    Dmod_Unsetenv("ENVMOD_LOG_LEVEL");
}

/**
 * @brief Environment variable value is case-insensitive
 */
TEST_F(DmodModuleLogLevelTest, EnvVarCaseInsensitive)
{
    Dmod_SetEnv("CASEMOD_LOG_LEVEL", "VERBOSE", 1);
    EXPECT_TRUE( Dmod_CheckModuleLogLevel("casemod", Dmod_LogLevel_Verbose) );
    Dmod_Unsetenv("CASEMOD_LOG_LEVEL");
}
