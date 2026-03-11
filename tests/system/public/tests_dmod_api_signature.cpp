#define DMOD_PRIVATE
#include <stdlib.h>
#include <string.h>
#include <gtest/gtest.h>
#include "dmod.h"

// ===============================================================
//                  Test fixture
// ===============================================================

// Note on version format in DMOD_MAKE_SIGNATURE:
//   DMOD_MAKE_SIGNATURE(MODULE, VERSION, FUNC) stringifies VERSION as a literal
//   token sequence. Use `API/MODULE` (e.g. 1/1) to produce a version string with
//   both an API version and a module version separated by '/'. Use a dotted value
//   (e.g. 1.0) to produce a simple version string with no module version part.

class DmodApiSignatureTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ===============================================================
//         Tests for Dmod_ApiSignature_IsValid
// ===============================================================

/**
 * @brief A standard DMOD signature must be valid.
 */
TEST_F(DmodApiSignatureTest, IsValidDmodSignature)
{
    static const char* sig = DMOD_MAKE_SIGNATURE(TestModule, 1/1, TestFunc);
    EXPECT_TRUE(Dmod_ApiSignature_IsValid(sig));
}

/**
 * @brief A builtin (DMBI) signature must be valid.
 */
TEST_F(DmodApiSignatureTest, IsValidBuiltinSignature)
{
    static const char* sig = DMOD_MAKE_BUILTIN_SIGNATURE(TestModule, 1/1, TestFunc);
    EXPECT_TRUE(Dmod_ApiSignature_IsValid(sig));
}

/**
 * @brief A MAL signature must be valid.
 */
TEST_F(DmodApiSignatureTest, IsValidMalSignature)
{
    static const char* sig = DMOD_MAKE_MAL_SIGNATURE(TestModule, 1/1, TestFunc);
    EXPECT_TRUE(Dmod_ApiSignature_IsValid(sig));
}

/**
 * @brief NULL must not be valid.
 */
TEST_F(DmodApiSignatureTest, IsValidNullReturnsFalse)
{
    EXPECT_FALSE(Dmod_ApiSignature_IsValid(nullptr));
}

/**
 * @brief A plain string without a proper prefix must not be valid.
 */
TEST_F(DmodApiSignatureTest, IsValidPlainStringReturnsFalse)
{
    EXPECT_FALSE(Dmod_ApiSignature_IsValid("TestFunc@TestModule:1/1"));
}

// ===============================================================
//         Tests for Dmod_ApiSignature_AreCompatible
// ===============================================================

/**
 * @brief Two identical signatures must be compatible.
 */
TEST_F(DmodApiSignatureTest, AreCompatibleIdenticalSignatures)
{
    static const char* sig = DMOD_MAKE_SIGNATURE(TestModule, 1/1, TestFunc);
    EXPECT_TRUE(Dmod_ApiSignature_AreCompatible(sig, sig));
}

/**
 * @brief Two independently-created signatures with the same name, module, and
 *        version must be compatible.
 */
TEST_F(DmodApiSignatureTest, AreCompatibleSameNameModuleVersion)
{
    static const char* sig1 = DMOD_MAKE_SIGNATURE(TestModule, 1/1, TestFunc);
    static const char* sig2 = DMOD_MAKE_SIGNATURE(TestModule, 1/1, TestFunc);
    EXPECT_TRUE(Dmod_ApiSignature_AreCompatible(sig1, sig2));
}

/**
 * @brief Signatures with the same major API version and the same major module
 *        version but different minor module versions must be compatible.
 *        e.g. "1/1.0" and "1/1.3" — minor module version difference is allowed.
 */
TEST_F(DmodApiSignatureTest, AreCompatibleDifferentMinorModuleVersions)
{
    static const char* sig1 = DMOD_MAKE_SIGNATURE(TestModule, 1/1.0, TestFunc);
    static const char* sig2 = DMOD_MAKE_SIGNATURE(TestModule, 1/1.3, TestFunc);
    EXPECT_TRUE(Dmod_ApiSignature_AreCompatible(sig1, sig2));
}

/**
 * @brief Signatures with different function names must not be compatible.
 */
TEST_F(DmodApiSignatureTest, AreCompatibleDifferentNameReturnsFalse)
{
    static const char* sig1 = DMOD_MAKE_SIGNATURE(TestModule, 1/1, FuncA);
    static const char* sig2 = DMOD_MAKE_SIGNATURE(TestModule, 1/1, FuncB);
    EXPECT_FALSE(Dmod_ApiSignature_AreCompatible(sig1, sig2));
}

/**
 * @brief Signatures with different module names must not be compatible.
 */
TEST_F(DmodApiSignatureTest, AreCompatibleDifferentModuleReturnsFalse)
{
    static const char* sig1 = DMOD_MAKE_SIGNATURE(ModuleA, 1/1, TestFunc);
    static const char* sig2 = DMOD_MAKE_SIGNATURE(ModuleB, 1/1, TestFunc);
    EXPECT_FALSE(Dmod_ApiSignature_AreCompatible(sig1, sig2));
}

/**
 * @brief Signatures with different major API versions must not be compatible.
 */
TEST_F(DmodApiSignatureTest, AreCompatibleDifferentMajorApiVersionReturnsFalse)
{
    static const char* sig1 = DMOD_MAKE_SIGNATURE(TestModule, 1/1, TestFunc);
    static const char* sig2 = DMOD_MAKE_SIGNATURE(TestModule, 2/1, TestFunc);
    EXPECT_FALSE(Dmod_ApiSignature_AreCompatible(sig1, sig2));
}

/**
 * @brief Signatures with different major module versions must not be compatible.
 */
TEST_F(DmodApiSignatureTest, AreCompatibleDifferentMajorModuleVersionReturnsFalse)
{
    static const char* sig1 = DMOD_MAKE_SIGNATURE(TestModule, 1/1, TestFunc);
    static const char* sig2 = DMOD_MAKE_SIGNATURE(TestModule, 1/2, TestFunc);
    EXPECT_FALSE(Dmod_ApiSignature_AreCompatible(sig1, sig2));
}

/**
 * @brief Signatures using a simple dotted version (no '/' separator for module
 *        version) must not crash and must be compatible when all other fields match.
 *
 * This is the regression test for the NULL-dereference crash that occurred when
 * the version string did not contain a '/' character.
 */
TEST_F(DmodApiSignatureTest, AreCompatibleSimpleDotVersionNoCrash)
{
    static const char* sig1 = DMOD_MAKE_SIGNATURE(TestModule, 1.0, TestFunc);
    static const char* sig2 = DMOD_MAKE_SIGNATURE(TestModule, 1.0, TestFunc);
    // Both lack a module version (no '/') -> compatible
    EXPECT_TRUE(Dmod_ApiSignature_AreCompatible(sig1, sig2));
}

/**
 * @brief Two signatures with a simple version but different names must not crash
 *        and must return false.
 */
TEST_F(DmodApiSignatureTest, AreCompatibleSimpleDotVersionDifferentNameReturnsFalse)
{
    static const char* sig1 = DMOD_MAKE_SIGNATURE(TestModule, 1.0, FuncA);
    static const char* sig2 = DMOD_MAKE_SIGNATURE(TestModule, 1.0, FuncB);
    EXPECT_FALSE(Dmod_ApiSignature_AreCompatible(sig1, sig2));
}

/**
 * @brief One signature has a module version ('/') and the other does not:
 *        they must not be compatible.
 */
TEST_F(DmodApiSignatureTest, AreCompatibleMixedVersionFormatReturnsFalse)
{
    static const char* sig1 = DMOD_MAKE_SIGNATURE(TestModule, 1/1, TestFunc);
    static const char* sig2 = DMOD_MAKE_SIGNATURE(TestModule, 1.0, TestFunc);
    EXPECT_FALSE(Dmod_ApiSignature_AreCompatible(sig1, sig2));
}

/**
 * @brief Passing NULL as the first signature must return false without crashing.
 */
TEST_F(DmodApiSignatureTest, AreCompatibleNullFirstReturnsFalse)
{
    static const char* sig = DMOD_MAKE_SIGNATURE(TestModule, 1/1, TestFunc);
    EXPECT_FALSE(Dmod_ApiSignature_AreCompatible(nullptr, sig));
}

/**
 * @brief Passing NULL as the second signature must return false without crashing.
 */
TEST_F(DmodApiSignatureTest, AreCompatibleNullSecondReturnsFalse)
{
    static const char* sig = DMOD_MAKE_SIGNATURE(TestModule, 1/1, TestFunc);
    EXPECT_FALSE(Dmod_ApiSignature_AreCompatible(sig, nullptr));
}

/**
 * @brief Passing two NULLs must return false without crashing.
 */
TEST_F(DmodApiSignatureTest, AreCompatibleBothNullReturnsFalse)
{
    EXPECT_FALSE(Dmod_ApiSignature_AreCompatible(nullptr, nullptr));
}

/**
 * @brief An invalid (unprefixed) signature must return false.
 */
TEST_F(DmodApiSignatureTest, AreCompatibleInvalidSignatureReturnsFalse)
{
    static const char* invalid = "TestFunc@TestModule:1/1";
    static const char* sig     = DMOD_MAKE_SIGNATURE(TestModule, 1/1, TestFunc);
    EXPECT_FALSE(Dmod_ApiSignature_AreCompatible(invalid, sig));
    EXPECT_FALSE(Dmod_ApiSignature_AreCompatible(sig, invalid));
}

/**
 * @brief A DMOD and a BUILTIN signature for the same API must be compatible.
 */
TEST_F(DmodApiSignatureTest, AreCompatibleDmodAndBuiltinSameApi)
{
    static const char* dmodSig    = DMOD_MAKE_SIGNATURE(TestModule, 1/1, TestFunc);
    static const char* builtinSig = DMOD_MAKE_BUILTIN_SIGNATURE(TestModule, 1/1, TestFunc);
    EXPECT_TRUE(Dmod_ApiSignature_AreCompatible(dmodSig, builtinSig));
}

// ===============================================================
//         Tests for Dmod_ApiSignature_GetName / GetVersion /
//         GetModule / GetModuleVersion
// ===============================================================

/**
 * @brief GetName must return a pointer that starts with the function name.
 */
TEST_F(DmodApiSignatureTest, GetNameReturnsNamePart)
{
    static const char* sig = DMOD_MAKE_SIGNATURE(TestModule, 1/1, MyFunc);
    const char* name = Dmod_ApiSignature_GetName(sig);
    ASSERT_NE(name, nullptr);
    EXPECT_EQ(name[0], 'M');
    EXPECT_EQ(name[1], 'y');
}

/**
 * @brief GetName with NULL must return NULL.
 */
TEST_F(DmodApiSignatureTest, GetNameNullReturnsNull)
{
    EXPECT_EQ(Dmod_ApiSignature_GetName(nullptr), nullptr);
}

/**
 * @brief GetVersion must return a pointer that starts with the API version digit.
 */
TEST_F(DmodApiSignatureTest, GetVersionReturnsVersionPart)
{
    static const char* sig = DMOD_MAKE_SIGNATURE(TestModule, 2/5, MyFunc);
    const char* version = Dmod_ApiSignature_GetVersion(sig);
    ASSERT_NE(version, nullptr);
    EXPECT_EQ(version[0], '2');
}

/**
 * @brief GetModuleVersion must return a pointer that starts with the module
 *        version digit (the part after '/').
 */
TEST_F(DmodApiSignatureTest, GetModuleVersionReturnsModuleVersionPart)
{
    static const char* sig = DMOD_MAKE_SIGNATURE(TestModule, 1/3, MyFunc);
    const char* moduleVersion = Dmod_ApiSignature_GetModuleVersion(sig);
    ASSERT_NE(moduleVersion, nullptr);
    EXPECT_EQ(moduleVersion[0], '3');
}

/**
 * @brief GetModuleVersion must return NULL when the version has no '/' separator.
 */
TEST_F(DmodApiSignatureTest, GetModuleVersionReturnsNullForSimpleVersion)
{
    static const char* sig = DMOD_MAKE_SIGNATURE(TestModule, 1.0, MyFunc);
    EXPECT_EQ(Dmod_ApiSignature_GetModuleVersion(sig), nullptr);
}

/**
 * @brief GetModule must return a pointer that starts with the module name.
 */
TEST_F(DmodApiSignatureTest, GetModuleReturnsModulePart)
{
    static const char* sig = DMOD_MAKE_SIGNATURE(SampleModule, 1/1, MyFunc);
    const char* module = Dmod_ApiSignature_GetModule(sig);
    ASSERT_NE(module, nullptr);
    EXPECT_EQ(module[0], 'S');
}

// ===============================================================
//         Tests for Dmod_ApiSignature_ReadModuleName /
//         ReadVersion / ReadModuleVersion
// ===============================================================

/**
 * @brief ReadModuleName must correctly copy the module name into the buffer.
 */
TEST_F(DmodApiSignatureTest, ReadModuleNameCopiesCorrectly)
{
    static const char* sig = DMOD_MAKE_SIGNATURE(SampleModule, 1/1, MyFunc);
    char buf[64] = {};
    ASSERT_TRUE(Dmod_ApiSignature_ReadModuleName(sig, buf, sizeof(buf)));
    EXPECT_STREQ(buf, "SampleModule");
}

/**
 * @brief ReadModuleName must return false when the output buffer is NULL.
 */
TEST_F(DmodApiSignatureTest, ReadModuleNameNullBufferReturnsFalse)
{
    static const char* sig = DMOD_MAKE_SIGNATURE(TestModule, 1/1, MyFunc);
    EXPECT_FALSE(Dmod_ApiSignature_ReadModuleName(sig, nullptr, 64));
}

/**
 * @brief ReadModuleName must return false when MaxLength is zero.
 */
TEST_F(DmodApiSignatureTest, ReadModuleNameZeroLengthReturnsFalse)
{
    static const char* sig = DMOD_MAKE_SIGNATURE(TestModule, 1/1, MyFunc);
    char buf[64] = {};
    EXPECT_FALSE(Dmod_ApiSignature_ReadModuleName(sig, buf, 0));
}

/**
 * @brief ReadVersion must correctly copy the API version string into the buffer.
 */
TEST_F(DmodApiSignatureTest, ReadVersionCopiesCorrectly)
{
    static const char* sig = DMOD_MAKE_SIGNATURE(TestModule, 2/5, MyFunc);
    char buf[32] = {};
    ASSERT_TRUE(Dmod_ApiSignature_ReadVersion(sig, buf, sizeof(buf)));
    EXPECT_EQ(buf[0], '2');
}

/**
 * @brief ReadVersion must return false when the output buffer is NULL.
 */
TEST_F(DmodApiSignatureTest, ReadVersionNullBufferReturnsFalse)
{
    static const char* sig = DMOD_MAKE_SIGNATURE(TestModule, 1/1, MyFunc);
    EXPECT_FALSE(Dmod_ApiSignature_ReadVersion(sig, nullptr, 32));
}

/**
 * @brief ReadModuleVersion must correctly copy the module version into the buffer.
 */
TEST_F(DmodApiSignatureTest, ReadModuleVersionCopiesCorrectly)
{
    static const char* sig = DMOD_MAKE_SIGNATURE(TestModule, 1/7, MyFunc);
    char buf[32] = {};
    ASSERT_TRUE(Dmod_ApiSignature_ReadModuleVersion(sig, buf, sizeof(buf)));
    EXPECT_EQ(buf[0], '7');
}

/**
 * @brief ReadModuleVersion must return false when the version has no '/' separator.
 */
TEST_F(DmodApiSignatureTest, ReadModuleVersionNoSeparatorReturnsFalse)
{
    static const char* sig = DMOD_MAKE_SIGNATURE(TestModule, 1.0, MyFunc);
    char buf[32] = {};
    EXPECT_FALSE(Dmod_ApiSignature_ReadModuleVersion(sig, buf, sizeof(buf)));
}

/**
 * @brief ReadModuleVersion must return false when the output buffer is NULL.
 */
TEST_F(DmodApiSignatureTest, ReadModuleVersionNullBufferReturnsFalse)
{
    static const char* sig = DMOD_MAKE_SIGNATURE(TestModule, 1/1, MyFunc);
    EXPECT_FALSE(Dmod_ApiSignature_ReadModuleVersion(sig, nullptr, 32));
}

// ===============================================================
//         Tests for Dmod_ApiSignature_IsModuleNameGiven /
//         IsModule / IsMal / IsBuiltin
// ===============================================================

/**
 * @brief A signature with a module name must report that the module name is given.
 */
TEST_F(DmodApiSignatureTest, IsModuleNameGivenReturnsTrueForModuleSignature)
{
    static const char* sig = DMOD_MAKE_SIGNATURE(MyModule, 1/1, MyFunc);
    EXPECT_TRUE(Dmod_ApiSignature_IsModuleNameGiven(sig));
}

/**
 * @brief IsModule must return true when the module name matches.
 */
TEST_F(DmodApiSignatureTest, IsModuleMatchingNameReturnsTrue)
{
    static const char* sig = DMOD_MAKE_SIGNATURE(MyModule, 1/1, MyFunc);
    EXPECT_TRUE(Dmod_ApiSignature_IsModule(sig, "MyModule"));
}

/**
 * @brief IsModule must return false when the module name does not match.
 */
TEST_F(DmodApiSignatureTest, IsModuleNonMatchingNameReturnsFalse)
{
    static const char* sig = DMOD_MAKE_SIGNATURE(MyModule, 1/1, MyFunc);
    EXPECT_FALSE(Dmod_ApiSignature_IsModule(sig, "OtherModule"));
}

/**
 * @brief IsMal must return true only for MAL signatures.
 */
TEST_F(DmodApiSignatureTest, IsMalReturnsTrueForMalSignature)
{
    static const char* malSig  = DMOD_MAKE_MAL_SIGNATURE(TestModule, 1/1, TestFunc);
    static const char* dmodSig = DMOD_MAKE_SIGNATURE(TestModule, 1/1, TestFunc);
    EXPECT_TRUE(Dmod_ApiSignature_IsMal(malSig));
    EXPECT_FALSE(Dmod_ApiSignature_IsMal(dmodSig));
}

/**
 * @brief IsBuiltin must return true only for builtin (DMBI) signatures.
 */
TEST_F(DmodApiSignatureTest, IsBuiltinReturnsTrueForBuiltinSignature)
{
    static const char* builtinSig = DMOD_MAKE_BUILTIN_SIGNATURE(TestModule, 1/1, TestFunc);
    static const char* dmodSig    = DMOD_MAKE_SIGNATURE(TestModule, 1/1, TestFunc);
    EXPECT_TRUE(Dmod_ApiSignature_IsBuiltin(builtinSig));
    EXPECT_FALSE(Dmod_ApiSignature_IsBuiltin(dmodSig));
}
