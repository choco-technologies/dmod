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
    Dmod_ManifestNode_t* node = Dmod_Manifest_FindEntry(ctx, "mymodule", nullptr, nullptr, &entry);
    ASSERT_NE(node, nullptr);
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
    Dmod_ManifestNode_t* node = Dmod_Manifest_FindEntry(ctx, "mymodule", "1.1", nullptr, &entry);
    ASSERT_NE(node, nullptr);
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
    Dmod_ManifestNode_t* node = Dmod_Manifest_FindEntry(ctx, "nonexistent", nullptr, nullptr, &entry);
    ASSERT_EQ(node, nullptr);
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
    // Request version 1.5 which doesn't exist - should not find anything
    Dmod_ManifestNode_t* node = Dmod_Manifest_FindEntry(ctx, "mymodule", "1.5", nullptr, &entry);
    ASSERT_EQ(node, nullptr);
    
    Dmod_Manifest_Free(ctx);
}

// ===============================================================
//                  Version Available Tests
// ===============================================================

TEST_F(DmodManifestTest, ParseVersionAvailableDirective) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = 
        "$version-available mymodule 1.0 1.1 1.2\n"
        "mymodule https://example.com/mymodule-<version>.dmf\n";
    
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    
    // Should have 3 entries (one for each version)
    EXPECT_EQ(Dmod_Manifest_GetEntryCount(ctx), 3);
    
    // Verify all three versions are present
    bool found_1_0 = false, found_1_1 = false, found_1_2 = false;
    
    for (size_t i = 0; i < Dmod_Manifest_GetEntryCount(ctx); i++) {
        Dmod_ManifestEntry_t entry;
        ASSERT_TRUE(Dmod_Manifest_GetEntry(ctx, i, &entry));
        EXPECT_STREQ(entry.name, "mymodule");
        
        if (strcmp(entry.version, "1.2") == 0) {
            found_1_2 = true;
            EXPECT_STREQ(entry.url, "https://example.com/mymodule-1.2.dmf");
        } else if (strcmp(entry.version, "1.1") == 0) {
            found_1_1 = true;
            EXPECT_STREQ(entry.url, "https://example.com/mymodule-1.1.dmf");
        } else if (strcmp(entry.version, "1.0") == 0) {
            found_1_0 = true;
            EXPECT_STREQ(entry.url, "https://example.com/mymodule-1.0.dmf");
        }
    }
    
    EXPECT_TRUE(found_1_0);
    EXPECT_TRUE(found_1_1);
    EXPECT_TRUE(found_1_2);
    
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, VersionAvailableWithExplicitVersion) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = 
        "$version-available mymodule 1.0 1.1\n"
        "mymodule@2.0 https://example.com/mymodule-<version>.dmf\n";
    
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    
    // Should have only 1 entry (explicit version is not expanded)
    EXPECT_EQ(Dmod_Manifest_GetEntryCount(ctx), 1);
    
    Dmod_ManifestEntry_t entry;
    ASSERT_TRUE(Dmod_Manifest_GetEntry(ctx, 0, &entry));
    EXPECT_STREQ(entry.name, "mymodule");
    EXPECT_STREQ(entry.version, "2.0");
    EXPECT_STREQ(entry.url, "https://example.com/mymodule-2.0.dmf");
    
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, VersionAvailableWithoutVersionPlaceholder) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = 
        "$version-available mymodule 1.0 1.1\n"
        "mymodule https://example.com/mymodule.dmf\n";
    
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    
    // Should have only 1 entry (URL has no <version> placeholder)
    EXPECT_EQ(Dmod_Manifest_GetEntryCount(ctx), 1);
    
    Dmod_ManifestEntry_t entry;
    ASSERT_TRUE(Dmod_Manifest_GetEntry(ctx, 0, &entry));
    EXPECT_STREQ(entry.name, "mymodule");
    EXPECT_STREQ(entry.version, "");
    EXPECT_STREQ(entry.url, "https://example.com/mymodule.dmf");
    
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, VersionAvailableMultipleModules) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = 
        "$version-available module1 1.0 1.1\n"
        "$version-available module2 2.0 2.1 2.2\n"
        "module1 https://example.com/module1-<version>.dmf\n"
        "module2 https://example.com/module2-<version>.dmf\n";
    
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    
    // Should have 5 entries (2 for module1, 3 for module2)
    EXPECT_EQ(Dmod_Manifest_GetEntryCount(ctx), 5);
    
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, VersionAvailableNoVersions) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = "$version-available mymodule\n";
    
    ASSERT_FALSE(Dmod_Manifest_Parse(ctx, manifest));
    EXPECT_NE(Dmod_Manifest_GetError(ctx), nullptr);
    
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, VersionAvailableNoModuleName) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = "$version-available\n";
    
    ASSERT_FALSE(Dmod_Manifest_Parse(ctx, manifest));
    EXPECT_NE(Dmod_Manifest_GetError(ctx), nullptr);
    
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, FindEntryWithVersionAvailable) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = 
        "$version-available mymodule 1.0 1.1 2.0\n"
        "mymodule https://example.com/mymodule-<version>.dmf\n";
    
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    
    // Find specific version
    Dmod_ManifestEntry_t entry;
    Dmod_ManifestNode_t* node = Dmod_Manifest_FindEntry(ctx, "mymodule", "1.1", nullptr, &entry);
    ASSERT_NE(node, nullptr);
    EXPECT_STREQ(entry.name, "mymodule");
    EXPECT_STREQ(entry.version, "1.1");
    EXPECT_STREQ(entry.url, "https://example.com/mymodule-1.1.dmf");
    
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, VersionAvailableOrderNewestFirst) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = 
        "$version-available dmffs 1.0 1.1\n"
        "dmffs https://example.com/dmffs-<version>.dmf\n";
    
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    
    // Should have 2 entries (one for each version)
    EXPECT_EQ(Dmod_Manifest_GetEntryCount(ctx), 2);
    
    // Verify that newest version (1.1) is tried first
    // When searching without specifying a version, FindEntry returns entries in order
    Dmod_ManifestEntry_t entry;
    Dmod_ManifestNode_t* node = Dmod_Manifest_FindEntry(ctx, "dmffs", nullptr, nullptr, &entry);
    ASSERT_NE(node, nullptr);
    EXPECT_STREQ(entry.name, "dmffs");
    EXPECT_STREQ(entry.version, "1.1");  // Should be newest first
    EXPECT_STREQ(entry.url, "https://example.com/dmffs-1.1.dmf");
    
    // Try to find next entry (should be 1.0)
    node = Dmod_Manifest_FindEntry(ctx, "dmffs", nullptr, node, &entry);
    ASSERT_NE(node, nullptr);
    EXPECT_STREQ(entry.name, "dmffs");
    EXPECT_STREQ(entry.version, "1.0");  // Should be oldest last
    EXPECT_STREQ(entry.url, "https://example.com/dmffs-1.0.dmf");
    
    // No more entries
    node = Dmod_Manifest_FindEntry(ctx, "dmffs", nullptr, node, &entry);
    EXPECT_EQ(node, nullptr);
    
    Dmod_Manifest_Free(ctx);
}

TEST_F(DmodManifestTest, VersionAvailableOrderWithMultipleVersions) {
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init("arch/x86_64", nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* manifest = 
        "$version-available testmod 1.0 1.5 2.0 2.1\n"
        "testmod https://example.com/testmod-<version>.dmf\n";
    
    ASSERT_TRUE(Dmod_Manifest_Parse(ctx, manifest));
    
    // Should have 4 entries
    EXPECT_EQ(Dmod_Manifest_GetEntryCount(ctx), 4);
    
    // Verify versions are returned in order: 2.1, 2.0, 1.5, 1.0 (newest first)
    Dmod_ManifestEntry_t entry;
    Dmod_ManifestNode_t* node = nullptr;
    const char* expected_versions[] = {"2.1", "2.0", "1.5", "1.0"};
    
    for (int i = 0; i < 4; i++) {
        node = Dmod_Manifest_FindEntry(ctx, "testmod", nullptr, node, &entry);
        ASSERT_NE(node, nullptr) << "Failed to find entry " << i;
        EXPECT_STREQ(entry.version, expected_versions[i]) 
            << "Expected version " << expected_versions[i] << " at position " << i;
    }
    
    // No more entries
    node = Dmod_Manifest_FindEntry(ctx, "testmod", nullptr, node, &entry);
    EXPECT_EQ(node, nullptr);
    
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
