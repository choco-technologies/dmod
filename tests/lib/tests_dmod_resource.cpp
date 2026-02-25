/**
 * @file tests_dmod_resource.cpp
 * @brief Unit tests for DMOD Resource Parser Library
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string.h>
#include <stdlib.h>
#include "dmod_resource.h"

// Test fixture
class DmodResourceTest : public ::testing::Test {
protected:
    Dmod_ResourceContext_t* ctx = nullptr;

    void SetUp() override {
        ctx = Dmod_Resource_Init("/install/path", "mymodule", nullptr, nullptr, nullptr);
    }

    void TearDown() override {
        if (ctx) {
            Dmod_Resource_Free(ctx);
            ctx = nullptr;
        }
    }
};

// ===============================================================
//                  Initialization Tests
// ===============================================================

TEST_F(DmodResourceTest, InitWithRequiredParams) {
    ASSERT_NE(ctx, nullptr);
}

TEST_F(DmodResourceTest, InitWithNullDestination) {
    Dmod_ResourceContext_t* c = Dmod_Resource_Init(nullptr, "mymodule", nullptr, nullptr, nullptr);
    ASSERT_EQ(c, nullptr);
}

TEST_F(DmodResourceTest, InitWithNullModule) {
    Dmod_ResourceContext_t* c = Dmod_Resource_Init("/install/path", nullptr, nullptr, nullptr, nullptr);
    ASSERT_EQ(c, nullptr);
}

TEST_F(DmodResourceTest, InitWithOptionalPaths) {
    Dmod_ResourceContext_t* c = Dmod_Resource_Init(
        "/install/path", "mymodule",
        "/repo", "/dmf", "/build");
    ASSERT_NE(c, nullptr);
    Dmod_Resource_Free(c);
}

// ===============================================================
//                  Basic Parsing Tests
// ===============================================================

TEST_F(DmodResourceTest, ParseEmptyContent) {
    ASSERT_TRUE(Dmod_Resource_Parse(ctx, ""));
    ASSERT_EQ(Dmod_Resource_GetEntryCount(ctx), 0u);
}

TEST_F(DmodResourceTest, ParseComment) {
    ASSERT_TRUE(Dmod_Resource_Parse(ctx, "# This is a comment\n"));
    ASSERT_EQ(Dmod_Resource_GetEntryCount(ctx), 0u);
}

TEST_F(DmodResourceTest, ParseSimpleEntry) {
    ASSERT_TRUE(Dmod_Resource_Parse(ctx, "dmf=./module.dmf => /install/path/${module}.dmf\n"));
    ASSERT_EQ(Dmod_Resource_GetEntryCount(ctx), 1u);

    Dmod_ResourceEntry_t entry;
    ASSERT_TRUE(Dmod_Resource_GetEntry(ctx, 0, &entry));
    ASSERT_STREQ(entry.key, "dmf");
    ASSERT_STREQ(entry.source, "./module.dmf");
    ASSERT_STREQ(entry.destination, "/install/path/mymodule.dmf");
    ASSERT_TRUE(entry.is_dmf_dmfc);
    ASSERT_EQ(entry.origin_count, 0u);
}

TEST_F(DmodResourceTest, ParseDestinationVariable) {
    ASSERT_TRUE(Dmod_Resource_Parse(ctx, "inc=./include => ${destination}/${module}/include\n"));
    ASSERT_EQ(Dmod_Resource_GetEntryCount(ctx), 1u);

    Dmod_ResourceEntry_t entry;
    ASSERT_TRUE(Dmod_Resource_GetEntry(ctx, 0, &entry));
    ASSERT_STREQ(entry.destination, "/install/path/mymodule/include");
}

TEST_F(DmodResourceTest, ParseNonDmfEntry) {
    ASSERT_TRUE(Dmod_Resource_Parse(ctx, "docs=./docs => ${destination}/${module}/docs\n"));
    Dmod_ResourceEntry_t entry;
    ASSERT_TRUE(Dmod_Resource_GetEntry(ctx, 0, &entry));
    ASSERT_FALSE(entry.is_dmf_dmfc);
}

TEST_F(DmodResourceTest, ParseInvalidMissingEquals) {
    ASSERT_FALSE(Dmod_Resource_Parse(ctx, "nodmf\n"));
    ASSERT_NE(Dmod_Resource_GetError(ctx), nullptr);
}

TEST_F(DmodResourceTest, ParseInvalidMissingArrow) {
    ASSERT_FALSE(Dmod_Resource_Parse(ctx, "key=source\n"));
    ASSERT_NE(Dmod_Resource_GetError(ctx), nullptr);
}

// ===============================================================
//                  [origin] Directive Tests
// ===============================================================

TEST_F(DmodResourceTest, ParseSingleOrigin) {
    const char* content =
        "inc=./include => ${destination}/${module}/include [origin=/repo/include]\n";
    ASSERT_TRUE(Dmod_Resource_Parse(ctx, content));

    Dmod_ResourceEntry_t entry;
    ASSERT_TRUE(Dmod_Resource_GetEntry(ctx, 0, &entry));
    ASSERT_EQ(entry.origin_count, 1u);
    ASSERT_STREQ(entry.origins[0], "/repo/include");
}

TEST_F(DmodResourceTest, ParseMultipleOrigins) {
    const char* content =
        "inc=./include => ${destination}/${module}/include "
        "[origin=/repo/include] [origin=/build/mymodule_defs.h]\n";
    ASSERT_TRUE(Dmod_Resource_Parse(ctx, content));

    Dmod_ResourceEntry_t entry;
    ASSERT_TRUE(Dmod_Resource_GetEntry(ctx, 0, &entry));
    ASSERT_EQ(entry.origin_count, 2u);
    ASSERT_STREQ(entry.origins[0], "/repo/include");
    ASSERT_STREQ(entry.origins[1], "/build/mymodule_defs.h");
}

TEST_F(DmodResourceTest, ParseOriginWithVariableSubstitution) {
    Dmod_ResourceContext_t* c = Dmod_Resource_Init(
        "/install", "mymodule",
        "/repo", "/dmf", "/build");

    const char* content =
        "inc=./include => ${destination}/${module}/include "
        "[origin=${repo_dir}/include] [origin=${build_dir}/${module}_defs.h]\n";
    ASSERT_TRUE(Dmod_Resource_Parse(c, content));

    Dmod_ResourceEntry_t entry;
    ASSERT_TRUE(Dmod_Resource_GetEntry(c, 0, &entry));
    ASSERT_EQ(entry.origin_count, 2u);
    ASSERT_STREQ(entry.origins[0], "/repo/include");
    ASSERT_STREQ(entry.origins[1], "/build/mymodule_defs.h");

    Dmod_Resource_Free(c);
}

TEST_F(DmodResourceTest, ParseOriginWithDmfDirVariable) {
    Dmod_ResourceContext_t* c = Dmod_Resource_Init(
        "/install", "mymodule",
        nullptr, "/dmf_files", nullptr);

    const char* content =
        "dmf=./module.dmf => ${destination}/${module}.dmf [origin=${dmf_dir}/module.dmf]\n";
    ASSERT_TRUE(Dmod_Resource_Parse(c, content));

    Dmod_ResourceEntry_t entry;
    ASSERT_TRUE(Dmod_Resource_GetEntry(c, 0, &entry));
    ASSERT_EQ(entry.origin_count, 1u);
    ASSERT_STREQ(entry.origins[0], "/dmf_files/module.dmf");

    Dmod_Resource_Free(c);
}

TEST_F(DmodResourceTest, ParseOriginMissingClosingBracket) {
    const char* content =
        "inc=./include => ${destination}/${module}/include [origin=/repo/include\n";
    ASSERT_FALSE(Dmod_Resource_Parse(ctx, content));
    ASSERT_NE(Dmod_Resource_GetError(ctx), nullptr);
}

TEST_F(DmodResourceTest, ParseEntryWithoutOriginHasZeroOrigins) {
    ASSERT_TRUE(Dmod_Resource_Parse(ctx, "docs=./docs => ${destination}/${module}/docs\n"));
    Dmod_ResourceEntry_t entry;
    ASSERT_TRUE(Dmod_Resource_GetEntry(ctx, 0, &entry));
    ASSERT_EQ(entry.origin_count, 0u);
}

// ===============================================================
//                  New Predefined Variable Tests
// ===============================================================

TEST_F(DmodResourceTest, RepoDirVariableSubstitution) {
    Dmod_ResourceContext_t* c = Dmod_Resource_Init(
        "/install", "mymodule",
        "/my/repo", nullptr, nullptr);

    ASSERT_TRUE(Dmod_Resource_Parse(c, "inc=./inc => ${repo_dir}/include\n"));
    Dmod_ResourceEntry_t entry;
    ASSERT_TRUE(Dmod_Resource_GetEntry(c, 0, &entry));
    ASSERT_STREQ(entry.destination, "/my/repo/include");

    Dmod_Resource_Free(c);
}

TEST_F(DmodResourceTest, BuildDirVariableSubstitution) {
    Dmod_ResourceContext_t* c = Dmod_Resource_Init(
        "/install", "mymodule",
        nullptr, nullptr, "/my/build");

    ASSERT_TRUE(Dmod_Resource_Parse(c, "gen=./gen => ${build_dir}/generated\n"));
    Dmod_ResourceEntry_t entry;
    ASSERT_TRUE(Dmod_Resource_GetEntry(c, 0, &entry));
    ASSERT_STREQ(entry.destination, "/my/build/generated");

    Dmod_Resource_Free(c);
}

TEST_F(DmodResourceTest, DmfDirVariableSubstitution) {
    Dmod_ResourceContext_t* c = Dmod_Resource_Init(
        "/install", "mymodule",
        nullptr, "/my/dmf", nullptr);

    ASSERT_TRUE(Dmod_Resource_Parse(c, "dmf=./module.dmf => ${dmf_dir}/${module}.dmf\n"));
    Dmod_ResourceEntry_t entry;
    ASSERT_TRUE(Dmod_Resource_GetEntry(c, 0, &entry));
    ASSERT_STREQ(entry.destination, "/my/dmf/mymodule.dmf");

    Dmod_Resource_Free(c);
}

TEST_F(DmodResourceTest, NullOptionalVarsKeepPlaceholder) {
    // When optional vars are NULL and not in env, placeholder is kept as-is
    ASSERT_TRUE(Dmod_Resource_Parse(ctx, "inc=./inc => ${repo_dir}/include\n"));
    Dmod_ResourceEntry_t entry;
    ASSERT_TRUE(Dmod_Resource_GetEntry(ctx, 0, &entry));
    ASSERT_STREQ(entry.destination, "${repo_dir}/include");
}

// ===============================================================
//                  Entry Retrieval Tests
// ===============================================================

TEST_F(DmodResourceTest, GetEntryOutOfBounds) {
    ASSERT_TRUE(Dmod_Resource_Parse(ctx, "dmf=./module.dmf => /install/module.dmf\n"));
    Dmod_ResourceEntry_t entry;
    ASSERT_FALSE(Dmod_Resource_GetEntry(ctx, 1, &entry));
}

TEST_F(DmodResourceTest, MultipleEntries) {
    const char* content =
        "dmf=./module.dmf => ${destination}/${module}.dmf\n"
        "docs=./docs => ${destination}/${module}/docs\n"
        "inc=./include => ${destination}/${module}/include [origin=/repo/include]\n";
    ASSERT_TRUE(Dmod_Resource_Parse(ctx, content));
    ASSERT_EQ(Dmod_Resource_GetEntryCount(ctx), 3u);

    Dmod_ResourceEntry_t entry;
    ASSERT_TRUE(Dmod_Resource_GetEntry(ctx, 2, &entry));
    ASSERT_EQ(entry.origin_count, 1u);
    ASSERT_STREQ(entry.origins[0], "/repo/include");
}
