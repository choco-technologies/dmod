#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include <string.h>
#include "dmod.h"
#include "private/dmod_vars.h"
#include "private/dmod_ctx.h"

// ===============================================================
//   Tests for per-module log level stored in Dmod_Context_t
// ===============================================================

class DmodModuleLogLevelTest : public ::testing::Test
{
};

/**
 * @brief Dmod_GetModuleLogLevel returns the context's log level when explicitly set
 */
TEST_F(DmodModuleLogLevelTest, GetModuleLogLevel_ContextLevel)
{
    Dmod_Context_t ctx = {};
    ctx.Signature = DMOD_CONTEXT_SIGNATURE;
    ctx.LogLevel = Dmod_LogLevel_Verbose;

    EXPECT_EQ(Dmod_GetModuleLogLevel(&ctx), Dmod_LogLevel_Verbose);
}

/**
 * @brief Dmod_GetModuleLogLevel returns global level when context level is Dmod_LogLevel_Count
 *
 * Dmod_LogLevel_Count is the sentinel meaning "inherit from global".
 */
TEST_F(DmodModuleLogLevelTest, GetModuleLogLevel_InheritsGlobal)
{
    Dmod_Context_t ctx = {};
    ctx.Signature = DMOD_CONTEXT_SIGNATURE;
    ctx.LogLevel = Dmod_LogLevel_Count; /* sentinel - inherit from global */

    Dmod_LogLevel_t savedGlobal = Dmod_LogLevel;
    Dmod_LogLevel = Dmod_LogLevel_Warn;

    EXPECT_EQ(Dmod_GetModuleLogLevel(&ctx), Dmod_LogLevel_Warn);

    Dmod_LogLevel = savedGlobal;
}

/**
 * @brief Dmod_GetModuleLogLevel with NULL context returns global level
 */
TEST_F(DmodModuleLogLevelTest, GetModuleLogLevel_NullContext)
{
    Dmod_LogLevel_t savedGlobal = Dmod_LogLevel;
    Dmod_LogLevel = Dmod_LogLevel_Error;

    EXPECT_EQ(Dmod_GetModuleLogLevel(NULL), Dmod_LogLevel_Error);

    Dmod_LogLevel = savedGlobal;
}

/**
 * @brief Dmod_SetModuleLogLevel updates the context log level for a loaded module
 */
TEST_F(DmodModuleLogLevelTest, SetModuleLogLevel_UpdatesContext)
{
    /* Build a fake context with a valid header */
    Dmod_Context_t ctx = {};
    ctx.Signature = DMOD_CONTEXT_SIGNATURE;
    ctx.LogLevel  = Dmod_LogLevel_Count;

    Dmod_ModuleHeader_t fakeHeader = {};
    strncpy(fakeHeader.Name, "test_setmod", DMOD_MAX_MODULE_NAME_LENGTH - 1);
    ctx.Header = &fakeHeader;

    ASSERT_TRUE(Dmod_Context_Add(&ctx));

    Dmod_SetModuleLogLevel("test_setmod", Dmod_LogLevel_Info);
    EXPECT_EQ(ctx.LogLevel, Dmod_LogLevel_Info);

    Dmod_Context_Remove(&ctx);
}

/**
 * @brief Dmod_GetModuleContext returns the correct context for a loaded module
 */
TEST_F(DmodModuleLogLevelTest, GetModuleContext_FindsContext)
{
    Dmod_Context_t ctx = {};
    ctx.Signature = DMOD_CONTEXT_SIGNATURE;
    ctx.LogLevel  = Dmod_LogLevel_Count;

    Dmod_ModuleHeader_t fakeHeader = {};
    strncpy(fakeHeader.Name, "test_getmod", DMOD_MAX_MODULE_NAME_LENGTH - 1);
    ctx.Header = &fakeHeader;

    ASSERT_TRUE(Dmod_Context_Add(&ctx));

    Dmod_Context_t* found = Dmod_GetModuleContext("test_getmod");
    EXPECT_EQ(found, &ctx);

    Dmod_Context_Remove(&ctx);
}

/**
 * @brief Dmod_GetModuleContext returns NULL for an unknown module
 */
TEST_F(DmodModuleLogLevelTest, GetModuleContext_NotFound)
{
    EXPECT_EQ(Dmod_GetModuleContext("nonexistent_module_xyz123"), nullptr);
}

/**
 * @brief Dmod_SetModuleLogLevel for an unloaded module logs a warning without crashing
 */
TEST_F(DmodModuleLogLevelTest, SetModuleLogLevel_UnknownModuleNocrash)
{
    /* Should not crash - just emit a warning */
    Dmod_SetModuleLogLevel("nonexistent_module_xyz123", Dmod_LogLevel_Verbose);
}

/**
 * @brief Dmod_Context_New initialises LogLevel to Dmod_LogLevel_Count (inherit from global)
 */
TEST_F(DmodModuleLogLevelTest, ContextNew_LogLevelInitialisedToCount)
{
    /* Allocate a tiny fake buffer so Context_New doesn't fail */
    static uint8_t fakeBuf[64] = {0};
    Dmod_Context_t* ctx = Dmod_Context_New(fakeBuf, sizeof(fakeBuf), NULL);
    ASSERT_NE(ctx, nullptr);

    EXPECT_EQ(ctx->LogLevel, Dmod_LogLevel_Count);

    /* Free without calling Context_Delete (which would free fakeBuf) */
    ctx->Data = NULL; /* prevent double-free of fakeBuf */
    Dmod_Context_Delete(ctx);
}
