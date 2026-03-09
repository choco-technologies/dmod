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
//                  Helpers
// ===============================================================

/**
 * @brief A dummy function used as a known function pointer in tests
 */
static void DummyFunction( void ) {}

/**
 * @brief Another dummy function used as a second known function pointer
 */
static void AnotherDummyFunction( void ) {}

// ===============================================================
//                  Test fixture
// ===============================================================

class DmodVerifyApisTest : public ::testing::Test
{
protected:
    Dmod_Api_t m_OriginalBuiltinInputApi;

    void SetUp() override
    {
        // Reset all loaded module contexts
        memset(Dmod_Contexts, 0, sizeof(Dmod_Contexts));

        // Save and reset the builtin input API so tests start from a clean state.
        // The linker-script-based initialisation can produce a non-zero SectionSize
        // value in the test binary, which would cause the helper to iterate over
        // uninitialised memory.  By zeroing SectionSize here each test controls
        // exactly what is present in the builtin inputs.
        m_OriginalBuiltinInputApi = Dmod_BuiltinInputApi;
        memset(&Dmod_BuiltinInputApi, 0, sizeof(Dmod_BuiltinInputApi));
        Dmod_BuiltinInputApi.ApiType = Dmod_ApiType_Input;
    }

    void TearDown() override
    {
        Dmod_BuiltinInputApi = m_OriginalBuiltinInputApi;
    }
};

// ===============================================================
//                  Tests for Dmod_VerifyAllApisSignatures
// ===============================================================

/**
 * @brief Passing NULL as the context must return false.
 */
TEST_F(DmodVerifyApisTest, NullContextReturnsFalse)
{
    EXPECT_FALSE(Dmod_VerifyAllApisSignatures(nullptr));
}

/**
 * @brief A context with no output or input entries must pass verification.
 */
TEST_F(DmodVerifyApisTest, EmptySectionsReturnsTrue)
{
    Dmod_Context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.Signature = DMOD_CONTEXT_SIGNATURE;
    ctx.Outputs.ApiType = Dmod_ApiType_Output;
    ctx.Inputs.ApiType  = Dmod_ApiType_Input;
    // SectionSize == 0 → zero entries → no checks performed

    EXPECT_TRUE(Dmod_VerifyAllApisSignatures(&ctx));
}

/**
 * @brief An output entry that holds a valid signature string must pass.
 */
TEST_F(DmodVerifyApisTest, OutputWithValidSignatureReturnsTrue)
{
    static const char* validSig = DMOD_MAKE_SIGNATURE(TestModule, 1.0, TestFunction);
    static void* outputEntries[1] = { (void*)validSig };

    Dmod_Context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.Signature = DMOD_CONTEXT_SIGNATURE;
    ctx.Outputs.OutputSection = reinterpret_cast<Dmod_OutputsSection_t*>(outputEntries);
    ctx.Outputs.SectionSize   = sizeof(void*);   // 1 entry
    ctx.Outputs.ApiType       = Dmod_ApiType_Output;
    ctx.Inputs.ApiType        = Dmod_ApiType_Input;

    EXPECT_TRUE(Dmod_VerifyAllApisSignatures(&ctx));
}

/**
 * @brief An output entry that holds a function pointer present in the builtin
 *        system inputs must pass verification (API is already connected).
 */
TEST_F(DmodVerifyApisTest, OutputAlreadyConnectedViaBuiltinInputsReturnsTrue)
{
    // Register DummyFunction in the builtin input section
    static Dmod_InputsSection_t builtinInputSection;
    builtinInputSection.Entries[0].Function  = reinterpret_cast<void*>(DummyFunction);
    builtinInputSection.Entries[0].Signature = DMOD_MAKE_BUILTIN_SIGNATURE(Dmod, 1.0, TestApi1);
    Dmod_BuiltinInputApi.InputSection = &builtinInputSection;
    Dmod_BuiltinInputApi.SectionSize  = sizeof(Dmod_ApiRegistration_t); // 1 entry

    // Simulate a connected output: entry holds a function pointer, not a signature
    static void* outputEntries[1] = { reinterpret_cast<void*>(DummyFunction) };

    Dmod_Context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.Signature = DMOD_CONTEXT_SIGNATURE;
    ctx.Outputs.OutputSection = reinterpret_cast<Dmod_OutputsSection_t*>(outputEntries);
    ctx.Outputs.SectionSize   = sizeof(void*);
    ctx.Outputs.ApiType       = Dmod_ApiType_Output;
    ctx.Inputs.ApiType        = Dmod_ApiType_Input;

    EXPECT_TRUE(Dmod_VerifyAllApisSignatures(&ctx));
}

/**
 * @brief An output entry that holds a function pointer present in a loaded
 *        module's input section must pass verification (API is already connected).
 */
TEST_F(DmodVerifyApisTest, OutputAlreadyConnectedViaModuleInputsReturnsTrue)
{
    // Build a minimal "provider" module context whose inputs contain AnotherDummyFunction
    static Dmod_InputsSection_t moduleInputSection;
    moduleInputSection.Entries[0].Function  = reinterpret_cast<void*>(AnotherDummyFunction);
    moduleInputSection.Entries[0].Signature = DMOD_MAKE_SIGNATURE(SomeModule, 1.0, SomeFunc);

    static Dmod_Context_t moduleCtx;
    memset(&moduleCtx, 0, sizeof(moduleCtx));
    moduleCtx.Signature              = DMOD_CONTEXT_SIGNATURE;
    moduleCtx.Inputs.InputSection    = &moduleInputSection;
    moduleCtx.Inputs.SectionSize     = sizeof(Dmod_ApiRegistration_t);
    moduleCtx.Inputs.ApiType         = Dmod_ApiType_Input;
    moduleCtx.Outputs.ApiType        = Dmod_ApiType_Output;

    Dmod_Contexts[0] = &moduleCtx;

    // Simulate a connected output entry in the context under test
    static void* outputEntries[1] = { reinterpret_cast<void*>(AnotherDummyFunction) };

    Dmod_Context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.Signature = DMOD_CONTEXT_SIGNATURE;
    ctx.Outputs.OutputSection = reinterpret_cast<Dmod_OutputsSection_t*>(outputEntries);
    ctx.Outputs.SectionSize   = sizeof(void*);
    ctx.Outputs.ApiType       = Dmod_ApiType_Output;
    ctx.Inputs.ApiType        = Dmod_ApiType_Input;

    EXPECT_TRUE(Dmod_VerifyAllApisSignatures(&ctx));
}

/**
 * @brief An output entry with a pointer that is neither a valid signature nor a
 *        known function must fail verification.
 *
 * The pointer must be readable (so the error-log path can safely treat it as a
 * string).  We use the address of a plain string that does NOT start with any
 * recognised signature prefix.
 */
TEST_F(DmodVerifyApisTest, OutputWithUnknownPointerReturnsFalse)
{
    // A readable string that is not a valid API signature and is not registered
    // as a function pointer in any input section.
    static char notASignature[] = "not_a_valid_signature";
    static void* outputEntries[1] = { static_cast<void*>(notASignature) };

    Dmod_Context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.Signature = DMOD_CONTEXT_SIGNATURE;
    ctx.Outputs.OutputSection = reinterpret_cast<Dmod_OutputsSection_t*>(outputEntries);
    ctx.Outputs.SectionSize   = sizeof(void*);
    ctx.Outputs.ApiType       = Dmod_ApiType_Output;
    ctx.Inputs.ApiType        = Dmod_ApiType_Input;

    EXPECT_FALSE(Dmod_VerifyAllApisSignatures(&ctx));
}

/**
 * @brief An input entry with an invalid signature string must fail verification.
 */
TEST_F(DmodVerifyApisTest, InputWithInvalidSignatureReturnsFalse)
{
    static const char* invalidSig = "not_a_valid_signature";
    static Dmod_ApiRegistration_t inputEntries[1] = {
        { reinterpret_cast<void*>(DummyFunction), invalidSig }
    };

    Dmod_Context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.Signature = DMOD_CONTEXT_SIGNATURE;
    ctx.Inputs.InputSection  = reinterpret_cast<Dmod_InputsSection_t*>(inputEntries);
    ctx.Inputs.SectionSize   = sizeof(Dmod_ApiRegistration_t);
    ctx.Inputs.ApiType       = Dmod_ApiType_Input;
    ctx.Outputs.ApiType      = Dmod_ApiType_Output;

    EXPECT_FALSE(Dmod_VerifyAllApisSignatures(&ctx));
}

/**
 * @brief An input entry with a valid signature string must pass verification.
 */
TEST_F(DmodVerifyApisTest, InputWithValidSignatureReturnsTrue)
{
    static const char* validSig = DMOD_MAKE_SIGNATURE(TestModule, 1.0, TestFunction);
    static Dmod_ApiRegistration_t inputEntries[1] = {
        { reinterpret_cast<void*>(DummyFunction), validSig }
    };

    Dmod_Context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.Signature = DMOD_CONTEXT_SIGNATURE;
    ctx.Inputs.InputSection  = reinterpret_cast<Dmod_InputsSection_t*>(inputEntries);
    ctx.Inputs.SectionSize   = sizeof(Dmod_ApiRegistration_t);
    ctx.Inputs.ApiType       = Dmod_ApiType_Input;
    ctx.Outputs.ApiType      = Dmod_ApiType_Output;

    EXPECT_TRUE(Dmod_VerifyAllApisSignatures(&ctx));
}
