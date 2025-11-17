/**
 * @file tests_dmod_manifest.cpp
 * @brief Unit tests for DMOD Manifest Parser Library
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string.h>
#include <stdlib.h>
#include "dmod_manifest.h"

// Mock download function that returns predefined content
static const char* g_mock_download_content = nullptr;
static bool g_mock_download_should_fail = false;

static bool MockDownloadFunc(const char* url, char** buffer, size_t* size, void* user_data) {
    if (g_mock_download_should_fail) {
        return false;
    }
    
    if (g_mock_download_content) {
        *buffer = strdup(g_mock_download_content);
        *size = strlen(g_mock_download_content);
        return true;
    }
    
    return false;
}

// Test fixture
class DmodManifestTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_mock_download_content = nullptr;
        g_mock_download_should_fail = false;
    }
    
    void TearDown() override {
        g_mock_download_content = nullptr;
        g_mock_download_should_fail = false;
    }
};

// ===============================================================
//                  Initialization Tests
// ===============================================================

TEST_F(DmodManifestTest, InitWithNullToolsName) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init(nullptr, nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, InitWithToolsName) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, FreeNullContext) {
    Dmod_Manifest_Free(nullptr); // Should not crash
}

// ===============================================================
//                  Parsing Tests
// ===============================================================

TEST_F(DmodManifestTest, ParseEmptyManifest) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = "";
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    EXPECT_EQ(Dmod_Manifest_GetEntryCount(ctx), 0);
    
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, ParseCommentsOnly) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = 
        "# This is a comment\n"
        "# Another comment\n"
        "\n"
        "   # Indented comment\n";
    
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    EXPECT_EQ(Dmod_Manifest_GetEntryCount(ctx), 0);
    
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, ParseSimpleEntry) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = "mymodule https://example.com/mymodule.dmf\n";
    
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    EXPECT_EQ(Dmod_Manifest_GetEntryCount(ctx), 1);
    
    Dmod_ManifestEntry_t entry;
    ASSERT_TRUE(Dmod_Manifest_GetEntry(ctx, 0, &entry));
    EXPECT_STREQ(entry.name, "mymodule");
    EXPECT_STREQ(entry.version, "");
    EXPECT_STREQ(entry.url, "https://example.com/mymodule.dmf");
    
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, ParseEntryWithVersion) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = "mymodule@1.0 https://example.com/mymodule-1.0.dmf\n";
    
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    EXPECT_EQ(Dmod_Manifest_GetEntryCount(ctx), 1);
    
    Dmod_ManifestEntry_t entry;
    ASSERT_TRUE(Dmod_Manifest_GetEntry(ctx, 0, &entry));
    EXPECT_STREQ(entry.name, "mymodule");
    EXPECT_STREQ(entry.version, "1.0");
    EXPECT_STREQ(entry.url, "https://example.com/mymodule-1.0.dmf");
    
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, ParseMultipleEntries) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = 
        "# Comment line\n"
        "module1@1.0 https://example.com/module1.dmf\n"
        "module2 https://example.com/module2.dmf\n"
        "\n"
        "module3@2.5 https://example.com/module3.dmf\n";
    
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    EXPECT_EQ(Dmod_Manifest_GetEntryCount(ctx), 3);
    
    Dmod_Manifest_Free(ctx);
}

// ===============================================================
//                  Variable Substitution Tests
// ===============================================================

TEST_F(DmodManifestTest, SubstituteToolsName) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/armv7/cortex-m7", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = "mymodule https://example.com/<tools_name>/module.dmf\n";
    
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    EXPECT_EQ(Dmod_Manifest_GetEntryCount(ctx), 1);
    
    Dmod_ManifestEntry_t entry;
    ASSERT_TRUE(Dmod_Manifest_GetEntry(ctx, 0, &entry));
    EXPECT_STREQ(entry.url, "https://example.com/arch/armv7/cortex-m7/module.dmf");
    
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, SubstituteArchName) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/armv7/cortex-m7", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = "mymodule https://example.com/<arch_name>/module.dmf\n";
    
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    EXPECT_EQ(Dmod_Manifest_GetEntryCount(ctx), 1);
    
    Dmod_ManifestEntry_t entry;
    ASSERT_TRUE(Dmod_Manifest_GetEntry(ctx, 0, &entry));
    EXPECT_STREQ(entry.url, "https://example.com/armv7-cortex-m7/module.dmf");
    
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, SubstituteBothVariables) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = "mymodule https://reg.com/<tools_name>/<arch_name>/mod.dmf\n";
    
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    EXPECT_EQ(Dmod_Manifest_GetEntryCount(ctx), 1);
    
    Dmod_ManifestEntry_t entry;
    ASSERT_TRUE(Dmod_Manifest_GetEntry(ctx, 0, &entry));
    EXPECT_STREQ(entry.url, "https://reg.com/arch/x86_64/x86_64/mod.dmf");
    
    Dmod_Manifest_Free(ctx);
}

// ===============================================================
//                  Include Directive Tests
// ===============================================================

TEST_F(DmodManifestTest, ParseIncludeDirective) {
    g_mock_download_content = "included_module@1.0 https://example.com/included.dmf\n";
    
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = 
        "base_module https://example.com/base.dmf\n"
        "$include https://example.com/manifest.dmm\n";
    
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    EXPECT_EQ(Dmod_Manifest_GetEntryCount(ctx), 2);
    
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, ParseIncludeDirectiveFail) {
    g_mock_download_should_fail = true;
    
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = "$include https://example.com/manifest.dmm\n";
    
    ASSERT_FALSE(Dmod_Manifest_Parse(ctx, manifest));
    EXPECT_NE(Dmod_Manifest_GetError(ctx), nullptr);
    
    Dmod_Manifest_Free(ctx);
}

// ===============================================================
//                  Find Entry Tests
// ===============================================================

TEST_F(DmodManifestTest, FindEntryByNameOnly) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = 
        "mymodule@1.0 https://example.com/mymodule-1.0.dmf\n"
        "mymodule@1.1 https://example.com/mymodule-1.1.dmf\n"
        "other https://example.com/other.dmf\n";
    
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    
    Dmod_ManifestEntry_t entry;
    ASSERT_TRUE(Dmod_Manifest_FindEntry(ctx, "mymodule", nullptr, &entry));
    EXPECT_STREQ(entry.name, "mymodule");
    // Should return first match
    
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, FindEntryByNameAndVersion) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = 
        "mymodule@1.0 https://example.com/mymodule-1.0.dmf\n"
        "mymodule@1.1 https://example.com/mymodule-1.1.dmf\n"
        "mymodule@2.0 https://example.com/mymodule-2.0.dmf\n";
    
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    
    Dmod_ManifestEntry_t entry;
    ASSERT_TRUE(Dmod_Manifest_FindEntry(ctx, "mymodule", "1.1", &entry));
    EXPECT_STREQ(entry.name, "mymodule");
    EXPECT_STREQ(entry.version, "1.1");
    EXPECT_STREQ(entry.url, "https://example.com/mymodule-1.1.dmf");
    
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, FindEntryNotFound) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = "mymodule@1.0 https://example.com/mymodule.dmf\n";
    
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    
    Dmod_ManifestEntry_t entry;
    ASSERT_FALSE(Dmod_Manifest_FindEntry(ctx, "nonexistent", nullptr, &entry));
    EXPECT_NE(Dmod_Manifest_GetError(ctx), nullptr);
    
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, FindEntryVersionNotExactMatch) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = 
        "mymodule@1.0 https://example.com/mymodule-1.0.dmf\n"
        "mymodule@2.0 https://example.com/mymodule-2.0.dmf\n";
    
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    
    Dmod_ManifestEntry_t entry;
    // Request version 1.5 which doesn't exist - should return first match
    ASSERT_TRUE(Dmod_Manifest_FindEntry(ctx, "mymodule", "1.5", &entry));
    EXPECT_STREQ(entry.name, "mymodule");
    
    Dmod_Manifest_Free(ctx);
}

// ===============================================================
//                  Error Handling Tests
// ===============================================================

TEST_F(DmodManifestTest, ParseNullContent) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    ASSERT_FALSE(Dmod_Manifest_Parse(ctx, nullptr));
    
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, GetEntryOutOfBounds) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = "mymodule https://example.com/mymodule.dmf\n";
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    
    Dmod_ManifestEntry_t entry;
    ASSERT_FALSE(Dmod_Manifest_GetEntry(ctx, 5, &entry));
    
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, InvalidManifestLine) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = "invalidline\n"; // No URL
    
    ASSERT_FALSE(Dmod_Manifest_Parse(ctx, manifest));
    EXPECT_NE(Dmod_Manifest_GetError(ctx), nullptr);
    
    Dmod_Manifest_Free(ctx);
}

// Main test runner
// Note: We use the shared ut_main.cpp from the tests directory
