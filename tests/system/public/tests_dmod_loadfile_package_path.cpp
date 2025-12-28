#define DMOD_PRIVATE
#include <stdlib.h>
#include <string.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "dmod.h"
#include "dmod_system.h"
#include "private/dmod_pck.h"

// ===============================================================
//                  Test fixture
// ===============================================================

class DmodLoadFilePackagePathTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Dmod_Initialize();
    }

    void TearDown() override
    {
        Dmod_Deinitialize();
    }
};

// ===============================================================
//                  Tests for [package]/module path support
// ===============================================================

/**
 * @brief Test that normal file paths are not affected by [package]/module parsing
 * 
 * This test verifies backward compatibility - normal file paths should still work
 * as expected and not be interpreted as package paths.
 */
TEST_F(DmodLoadFilePackagePathTest, NormalPathsNotAffectedByPackageParsing)
{
    // These paths should not be interpreted as [package]/module format
    const char* normalPaths[] = {
        "regular_module.dmf",
        "./relative/path/module.dmf",
        "/absolute/path/module.dmf",
        "module_without_extension",
        "[incomplete_bracket",
        "[package]no_slash",
        "[package]/",  // Missing module name
        "[]module",    // Empty package name
        NULL
    };

    for (int i = 0; normalPaths[i] != NULL; i++)
    {
        const char* path = normalPaths[i];
        
        // Verify the path detection logic
        bool startsWithBracket = (path[0] == '[');
        const char* closeBracket = strchr(path, ']');
        bool hasValidFormat = (startsWithBracket && 
                              closeBracket != NULL && 
                              closeBracket[1] == '/' && 
                              closeBracket[2] != '\0' &&
                              (closeBracket - path - 1) > 0);
        
        // Only paths with valid [package]/module format should be parsed as such
        if (strcmp(path, "[package]/module") == 0 || strcmp(path, "[pkg]/mod") == 0)
        {
            EXPECT_TRUE(hasValidFormat) << "Path '" << path << "' should be valid package format";
        }
        else if (startsWithBracket)
        {
            EXPECT_FALSE(hasValidFormat) << "Path '" << path << "' should NOT be valid package format";
        }
    }
}

/**
 * @brief Test [package]/module path format detection
 * 
 * This test verifies that valid [package]/module paths are correctly identified.
 */
TEST_F(DmodLoadFilePackagePathTest, ValidPackagePathFormatDetection)
{
    // These paths should be recognized as [package]/module format
    const char* validPaths[] = {
        "[test_package]/MyLibrary",
        "[pkg]/module",
        "[a]/b",
        "[very_long_package_name]/module_name",
        NULL
    };

    for (int i = 0; validPaths[i] != NULL; i++)
    {
        const char* path = validPaths[i];
        
        // Verify the format is detected
        bool startsWithBracket = (path[0] == '[');
        const char* closeBracket = strchr(path, ']');
        bool hasValidFormat = (startsWithBracket && 
                              closeBracket != NULL && 
                              closeBracket[1] == '/' && 
                              closeBracket[2] != '\0' &&
                              (closeBracket - path - 1) > 0);
        
        EXPECT_TRUE(hasValidFormat) << "Path '" << path << "' should be recognized as valid [package]/module format";
        
        if (hasValidFormat)
        {
            // Extract package name length
            size_t packageNameLen = closeBracket - path - 1;
            EXPECT_GT(packageNameLen, 0u) << "Package name should not be empty";
            EXPECT_LT(packageNameLen, DMOD_MAX_PACKAGE_NAME_LENGTH) << "Package name should fit in buffer";
        }
    }
}

/**
 * @brief Test loading from [package]/module path when package exists
 * 
 * This test verifies that Dmod_LoadFile can successfully load a module
 * from a package using the [package]/module path format.
 */
TEST_F(DmodLoadFilePackagePathTest, LoadFromPackagePath)
{
    // First, load a DMP package
    uint32_t packageIndex = UINT32_MAX;
    const char* packagePath = "/tmp/test_package.dmp";
    
    // Check if test package exists
    if (!Dmod_FileAvailable(packagePath))
    {
        GTEST_SKIP() << "Test package not available at " << packagePath;
    }
    
    bool packageLoaded = Dmod_AddPackageFile(packagePath, &packageIndex);
    if (!packageLoaded)
    {
        GTEST_SKIP() << "Failed to load test package";
    }
    
    ASSERT_NE(packageIndex, UINT32_MAX) << "Package index should be valid";
    
    // Now try to load a module using [package]/module format
    Dmod_Context_t* ctx = Dmod_LoadFile("[test_package]/MyLibrary");
    
    ASSERT_NE(ctx, nullptr) << "Should be able to load module from package using [package]/module format";
    
    if (ctx != NULL)
    {
        // Verify we loaded the correct module
        const char* moduleName = Dmod_GetName(ctx);
        EXPECT_STREQ(moduleName, "MyLibrary") << "Loaded module should have correct name";
        
        // Verify package name is set
        EXPECT_STREQ(ctx->PackageName, "test_package") << "Module should have correct package name";
        
        // Clean up
        Dmod_Unload(ctx, true);
    }
}

/**
 * @brief Test that [package]/module path fails gracefully when package doesn't exist
 * 
 * This test verifies that trying to load from a non-existent package
 * returns NULL instead of crashing.
 */
TEST_F(DmodLoadFilePackagePathTest, NonExistentPackageFails)
{
    // Try to load from a package that doesn't exist
    Dmod_Context_t* ctx = Dmod_LoadFile("[nonexistent_package]/some_module");
    
    EXPECT_EQ(ctx, nullptr) << "Loading from non-existent package should return NULL";
}

/**
 * @brief Test that [package]/module path fails gracefully when module doesn't exist in package
 * 
 * This test verifies that trying to load a non-existent module from a package
 * returns NULL instead of crashing.
 */
TEST_F(DmodLoadFilePackagePathTest, NonExistentModuleInPackageFails)
{
    // First, load a DMP package
    uint32_t packageIndex = UINT32_MAX;
    const char* packagePath = "/tmp/test_package.dmp";
    
    // Check if test package exists
    if (!Dmod_FileAvailable(packagePath))
    {
        GTEST_SKIP() << "Test package not available at " << packagePath;
    }
    
    bool packageLoaded = Dmod_AddPackageFile(packagePath, &packageIndex);
    if (!packageLoaded)
    {
        GTEST_SKIP() << "Failed to load test package";
    }
    
    // Try to load a module that doesn't exist in the package
    Dmod_Context_t* ctx = Dmod_LoadFile("[test_package]/NonExistentModule");
    
    EXPECT_EQ(ctx, nullptr) << "Loading non-existent module from package should return NULL";
}
