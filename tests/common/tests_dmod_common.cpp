#include <gtest/gtest.h>
#include "dmod.h"

class DmodCommonTest : public ::testing::Test
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
//                  Tests for Dmod_ApiSignature_IsModule
// ===============================================================

/**
 * @brief Test for Dmod_ApiSignature_IsModule with exact module name match
 */
TEST_F(DmodCommonTest, IsModule_ExactMatch)
{
    const char* signature = "\021DMOD\022_create@dmlist:1.0";
    ASSERT_TRUE(Dmod_ApiSignature_IsModule(signature, "dmlist"));
}

/**
 * @brief Test for Dmod_ApiSignature_IsModule with different module name
 */
TEST_F(DmodCommonTest, IsModule_DifferentModule)
{
    const char* signature = "\021DMOD\022_create@dmlist:1.0";
    ASSERT_FALSE(Dmod_ApiSignature_IsModule(signature, "other"));
}

/**
 * @brief Test that partial prefix of module name doesn't match
 * 
 * This is the key bug fix test - "dm" should NOT match "dmlist"
 */
TEST_F(DmodCommonTest, IsModule_PartialPrefixNoMatch)
{
    const char* signature = "\021DMOD\022_create@dmlist:1.0";
    // "dm" is a prefix of "dmlist" but should NOT match
    ASSERT_FALSE(Dmod_ApiSignature_IsModule(signature, "dm"));
    // "d" should also NOT match
    ASSERT_FALSE(Dmod_ApiSignature_IsModule(signature, "d"));
    // "dmlis" should also NOT match (missing 't')
    ASSERT_FALSE(Dmod_ApiSignature_IsModule(signature, "dmlis"));
}

/**
 * @brief Test that longer module name doesn't match
 */
TEST_F(DmodCommonTest, IsModule_LongerNameNoMatch)
{
    const char* signature = "\021DMOD\022_create@dmlist:1.0";
    // "dmlistExtra" is longer than "dmlist" and should NOT match
    ASSERT_FALSE(Dmod_ApiSignature_IsModule(signature, "dmlistExtra"));
}

/**
 * @brief Test for Dmod_ApiSignature_IsModule with MAL signature
 */
TEST_F(DmodCommonTest, IsModule_MalSignature)
{
    const char* signature = "\021DMAL\022_create@dmlist:1.0";
    ASSERT_TRUE(Dmod_ApiSignature_IsModule(signature, "dmlist"));
    ASSERT_FALSE(Dmod_ApiSignature_IsModule(signature, "dm"));
}

/**
 * @brief Test for Dmod_ApiSignature_IsModule with DIF signature
 */
TEST_F(DmodCommonTest, IsModule_DifSignature)
{
    const char* signature = "\021DDIF\022_create@dmlist:1.0";
    ASSERT_TRUE(Dmod_ApiSignature_IsModule(signature, "dmlist"));
    ASSERT_FALSE(Dmod_ApiSignature_IsModule(signature, "dm"));
}

/**
 * @brief Test for Dmod_ApiSignature_IsModule with invalid signature
 */
TEST_F(DmodCommonTest, IsModule_InvalidSignature)
{
    const char* signature = "invalid_signature";
    ASSERT_FALSE(Dmod_ApiSignature_IsModule(signature, "dmlist"));
}

/**
 * @brief Test for Dmod_ApiSignature_IsModule with NULL signature
 */
TEST_F(DmodCommonTest, IsModule_NullSignature)
{
    ASSERT_FALSE(Dmod_ApiSignature_IsModule(NULL, "dmlist"));
}

/**
 * @brief Test for Dmod_ApiSignature_IsModule with NULL module name
 */
TEST_F(DmodCommonTest, IsModule_NullModuleName)
{
    const char* signature = "\021DMOD\022_create@dmlist:1.0";
    ASSERT_FALSE(Dmod_ApiSignature_IsModule(signature, NULL));
}

/**
 * @brief Test for Dmod_ApiSignature_IsModule with empty module name
 */
TEST_F(DmodCommonTest, IsModule_EmptyModuleName)
{
    const char* signature = "\021DMOD\022_create@dmlist:1.0";
    ASSERT_FALSE(Dmod_ApiSignature_IsModule(signature, ""));
}

/**
 * @brief Test that "Dmod" module is recognized correctly
 */
TEST_F(DmodCommonTest, IsModule_DmodModule)
{
    const char* signature = "\021DMOD\022_SetLogLevel@Dmod:1.0";
    ASSERT_TRUE(Dmod_ApiSignature_IsModule(signature, "Dmod"));
    // Partial matches should not work
    ASSERT_FALSE(Dmod_ApiSignature_IsModule(signature, "Dm"));
    ASSERT_FALSE(Dmod_ApiSignature_IsModule(signature, "D"));
}
