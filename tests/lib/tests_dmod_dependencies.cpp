/**
 * @file tests_dmod_dependencies.cpp
 * @brief Unit tests for DMOD Dependencies Parser Library
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string.h>
#include <stdlib.h>
#include "dmod_dependencies.h"

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
class DmodDependenciesTest : public ::testing::Test {
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

TEST_F(DmodDependenciesTest, InitWithDefaultManifest) {
    Dmod_DependenciesContext_t* ctx = Dmod_Dependencies_Init("https://example.com/manifest.dmm", MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    Dmod_Dependencies_Free(ctx);
}

TEST_F(DmodDependenciesTest, InitWithNullManifest) {
    Dmod_DependenciesContext_t* ctx = Dmod_Dependencies_Init(nullptr, MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    Dmod_Dependencies_Free(ctx);
}

TEST_F(DmodDependenciesTest, FreeNullContext) {
    Dmod_Dependencies_Free(nullptr); // Should not crash
}

// ===============================================================
//                  Parsing Tests
// ===============================================================

TEST_F(DmodDependenciesTest, ParseEmptyDependencies) {
    Dmod_DependenciesContext_t* ctx = Dmod_Dependencies_Init("https://example.com/manifest.dmm", MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* dependencies = "";
    ASSERT_TRUE(Dmod_Dependencies_Parse(ctx, dependencies));
    EXPECT_EQ(Dmod_Dependencies_GetEntryCount(ctx), 0);
    
    Dmod_Dependencies_Free(ctx);
}

TEST_F(DmodDependenciesTest, ParseCommentsOnly) {
    Dmod_DependenciesContext_t* ctx = Dmod_Dependencies_Init("https://example.com/manifest.dmm", MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* dependencies = 
        "# This is a comment\n"
        "# Another comment\n"
        "\n"
        "   # Indented comment\n";
    
    ASSERT_TRUE(Dmod_Dependencies_Parse(ctx, dependencies));
    EXPECT_EQ(Dmod_Dependencies_GetEntryCount(ctx), 0);
    
    Dmod_Dependencies_Free(ctx);
}

TEST_F(DmodDependenciesTest, ParseSimpleModule) {
    Dmod_DependenciesContext_t* ctx = Dmod_Dependencies_Init("https://example.com/manifest.dmm", MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* dependencies = "mymodule\n";
    
    ASSERT_TRUE(Dmod_Dependencies_Parse(ctx, dependencies));
    EXPECT_EQ(Dmod_Dependencies_GetEntryCount(ctx), 1);
    
    Dmod_DependencyEntry_t entry;
    ASSERT_TRUE(Dmod_Dependencies_GetEntry(ctx, 0, &entry));
    EXPECT_STREQ(entry.name, "mymodule");
    EXPECT_STREQ(entry.version, "");
    EXPECT_STREQ(entry.manifest, "https://example.com/manifest.dmm");
    
    Dmod_Dependencies_Free(ctx);
}

TEST_F(DmodDependenciesTest, ParseModuleWithVersion) {
    Dmod_DependenciesContext_t* ctx = Dmod_Dependencies_Init("https://example.com/manifest.dmm", MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* dependencies = "mymodule@1.0\n";
    
    ASSERT_TRUE(Dmod_Dependencies_Parse(ctx, dependencies));
    EXPECT_EQ(Dmod_Dependencies_GetEntryCount(ctx), 1);
    
    Dmod_DependencyEntry_t entry;
    ASSERT_TRUE(Dmod_Dependencies_GetEntry(ctx, 0, &entry));
    EXPECT_STREQ(entry.name, "mymodule");
    EXPECT_STREQ(entry.version, "1.0");
    EXPECT_STREQ(entry.manifest, "https://example.com/manifest.dmm");
    
    Dmod_Dependencies_Free(ctx);
}

TEST_F(DmodDependenciesTest, ParseMultipleModules) {
    Dmod_DependenciesContext_t* ctx = Dmod_Dependencies_Init("https://example.com/manifest.dmm", MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* dependencies = 
        "module1\n"
        "module2@1.0\n"
        "module3@2.5.1\n";
    
    ASSERT_TRUE(Dmod_Dependencies_Parse(ctx, dependencies));
    EXPECT_EQ(Dmod_Dependencies_GetEntryCount(ctx), 3);
    
    Dmod_DependencyEntry_t entry;
    
    // Check first module
    ASSERT_TRUE(Dmod_Dependencies_GetEntry(ctx, 0, &entry));
    EXPECT_STREQ(entry.name, "module1");
    EXPECT_STREQ(entry.version, "");
    
    // Check second module
    ASSERT_TRUE(Dmod_Dependencies_GetEntry(ctx, 1, &entry));
    EXPECT_STREQ(entry.name, "module2");
    EXPECT_STREQ(entry.version, "1.0");
    
    // Check third module
    ASSERT_TRUE(Dmod_Dependencies_GetEntry(ctx, 2, &entry));
    EXPECT_STREQ(entry.name, "module3");
    EXPECT_STREQ(entry.version, "2.5.1");
    
    Dmod_Dependencies_Free(ctx);
}

TEST_F(DmodDependenciesTest, ParseFromDirective) {
    Dmod_DependenciesContext_t* ctx = Dmod_Dependencies_Init("https://default.com/manifest.dmm", MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* dependencies = 
        "module1\n"
        "from: https://new.com/manifest.dmm\n"
        "module2\n";
    
    ASSERT_TRUE(Dmod_Dependencies_Parse(ctx, dependencies));
    EXPECT_EQ(Dmod_Dependencies_GetEntryCount(ctx), 2);
    
    Dmod_DependencyEntry_t entry;
    
    // First module should use default manifest
    ASSERT_TRUE(Dmod_Dependencies_GetEntry(ctx, 0, &entry));
    EXPECT_STREQ(entry.name, "module1");
    EXPECT_STREQ(entry.manifest, "https://default.com/manifest.dmm");
    
    // Second module should use new manifest
    ASSERT_TRUE(Dmod_Dependencies_GetEntry(ctx, 1, &entry));
    EXPECT_STREQ(entry.name, "module2");
    EXPECT_STREQ(entry.manifest, "https://new.com/manifest.dmm");
    
    Dmod_Dependencies_Free(ctx);
}

TEST_F(DmodDependenciesTest, ParseMultipleFromDirectives) {
    Dmod_DependenciesContext_t* ctx = Dmod_Dependencies_Init("https://default.com/manifest.dmm", MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* dependencies = 
        "module1\n"
        "from: https://first.com/manifest.dmm\n"
        "module2\n"
        "from: https://second.com/manifest.dmm\n"
        "module3\n";
    
    ASSERT_TRUE(Dmod_Dependencies_Parse(ctx, dependencies));
    EXPECT_EQ(Dmod_Dependencies_GetEntryCount(ctx), 3);
    
    Dmod_DependencyEntry_t entry;
    
    ASSERT_TRUE(Dmod_Dependencies_GetEntry(ctx, 0, &entry));
    EXPECT_STREQ(entry.manifest, "https://default.com/manifest.dmm");
    
    ASSERT_TRUE(Dmod_Dependencies_GetEntry(ctx, 1, &entry));
    EXPECT_STREQ(entry.manifest, "https://first.com/manifest.dmm");
    
    ASSERT_TRUE(Dmod_Dependencies_GetEntry(ctx, 2, &entry));
    EXPECT_STREQ(entry.manifest, "https://second.com/manifest.dmm");
    
    Dmod_Dependencies_Free(ctx);
}

TEST_F(DmodDependenciesTest, ParseIncludeDirective) {
    Dmod_DependenciesContext_t* ctx = Dmod_Dependencies_Init("https://example.com/manifest.dmm", MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    // Set up mock download to return additional dependencies
    g_mock_download_content = "included_module@1.0\n";
    
    const char* dependencies = 
        "module1\n"
        "$include https://example.com/included.dmd\n"
        "module2\n";
    
    ASSERT_TRUE(Dmod_Dependencies_Parse(ctx, dependencies));
    EXPECT_EQ(Dmod_Dependencies_GetEntryCount(ctx), 3);
    
    Dmod_DependencyEntry_t entry;
    
    ASSERT_TRUE(Dmod_Dependencies_GetEntry(ctx, 0, &entry));
    EXPECT_STREQ(entry.name, "module1");
    
    ASSERT_TRUE(Dmod_Dependencies_GetEntry(ctx, 1, &entry));
    EXPECT_STREQ(entry.name, "included_module");
    EXPECT_STREQ(entry.version, "1.0");
    
    ASSERT_TRUE(Dmod_Dependencies_GetEntry(ctx, 2, &entry));
    EXPECT_STREQ(entry.name, "module2");
    
    Dmod_Dependencies_Free(ctx);
}

TEST_F(DmodDependenciesTest, ParseIncludeDirectiveFail) {
    Dmod_DependenciesContext_t* ctx = Dmod_Dependencies_Init("https://example.com/manifest.dmm", MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    // Set up mock download to fail
    g_mock_download_should_fail = true;
    
    const char* dependencies = "$include https://example.com/missing.dmd\n";
    
    ASSERT_FALSE(Dmod_Dependencies_Parse(ctx, dependencies));
    EXPECT_NE(Dmod_Dependencies_GetError(ctx), nullptr);
    
    Dmod_Dependencies_Free(ctx);
}

TEST_F(DmodDependenciesTest, ParseComplexExample) {
    Dmod_DependenciesContext_t* ctx = Dmod_Dependencies_Init("https://default.com/manifest.dmm", MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* dependencies = 
        "# Main dependencies\n"
        "dmffs\n"
        "driver@1.0\n"
        "\n"
        "# Change manifest\n"
        "from: https://new.com/manifest.dmm\n"
        "spi@1.0\n"
        "# More modules\n"
        "i2c@2.0\n";
    
    ASSERT_TRUE(Dmod_Dependencies_Parse(ctx, dependencies));
    EXPECT_EQ(Dmod_Dependencies_GetEntryCount(ctx), 4);
    
    Dmod_DependencyEntry_t entry;
    
    ASSERT_TRUE(Dmod_Dependencies_GetEntry(ctx, 0, &entry));
    EXPECT_STREQ(entry.name, "dmffs");
    EXPECT_STREQ(entry.manifest, "https://default.com/manifest.dmm");
    
    ASSERT_TRUE(Dmod_Dependencies_GetEntry(ctx, 1, &entry));
    EXPECT_STREQ(entry.name, "driver");
    EXPECT_STREQ(entry.version, "1.0");
    
    ASSERT_TRUE(Dmod_Dependencies_GetEntry(ctx, 2, &entry));
    EXPECT_STREQ(entry.name, "spi");
    EXPECT_STREQ(entry.manifest, "https://new.com/manifest.dmm");
    
    ASSERT_TRUE(Dmod_Dependencies_GetEntry(ctx, 3, &entry));
    EXPECT_STREQ(entry.name, "i2c");
    EXPECT_STREQ(entry.version, "2.0");
    EXPECT_STREQ(entry.manifest, "https://new.com/manifest.dmm");
    
    Dmod_Dependencies_Free(ctx);
}

// ===============================================================
//                  Error Handling Tests
// ===============================================================

TEST_F(DmodDependenciesTest, ParseNullContent) {
    Dmod_DependenciesContext_t* ctx = Dmod_Dependencies_Init("https://example.com/manifest.dmm", MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    ASSERT_FALSE(Dmod_Dependencies_Parse(ctx, nullptr));
    EXPECT_NE(Dmod_Dependencies_GetError(ctx), nullptr);
    
    Dmod_Dependencies_Free(ctx);
}

TEST_F(DmodDependenciesTest, GetEntryOutOfBounds) {
    Dmod_DependenciesContext_t* ctx = Dmod_Dependencies_Init("https://example.com/manifest.dmm", MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* dependencies = "module1\n";
    ASSERT_TRUE(Dmod_Dependencies_Parse(ctx, dependencies));
    
    Dmod_DependencyEntry_t entry;
    ASSERT_FALSE(Dmod_Dependencies_GetEntry(ctx, 1, &entry));
    EXPECT_NE(Dmod_Dependencies_GetError(ctx), nullptr);
    
    Dmod_Dependencies_Free(ctx);
}

TEST_F(DmodDependenciesTest, ParseEmptyFromDirective) {
    Dmod_DependenciesContext_t* ctx = Dmod_Dependencies_Init("https://example.com/manifest.dmm", MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* dependencies = "from:\n";
    
    ASSERT_FALSE(Dmod_Dependencies_Parse(ctx, dependencies));
    EXPECT_NE(Dmod_Dependencies_GetError(ctx), nullptr);
    
    Dmod_Dependencies_Free(ctx);
}

TEST_F(DmodDependenciesTest, ParseEmptyIncludeDirective) {
    Dmod_DependenciesContext_t* ctx = Dmod_Dependencies_Init("https://example.com/manifest.dmm", MockDownloadFunc, nullptr);
    ASSERT_NE(ctx, nullptr);
    
    const char* dependencies = "$include\n";
    
    ASSERT_FALSE(Dmod_Dependencies_Parse(ctx, dependencies));
    EXPECT_NE(Dmod_Dependencies_GetError(ctx), nullptr);
    
    Dmod_Dependencies_Free(ctx);
}
