#define DMOD_PRIVATE
#include <stdlib.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "dmod.h"
#include "dmod_system.h"
#include "dmod_arch_defs.h"

// ===============================================================
//                  Test fixture
// ===============================================================

class DmodArchTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize if needed
    }

    void TearDown() override {
        // Cleanup if needed
    }
};

// ===============================================================
//                  Tests
// ===============================================================

/**
 * @brief Test Dmod_GetFileArchitecture with NULL parameters
 */
TEST_F(DmodArchTest, GetFileArchitectureNullPath) {
    char arch[DMOD_MAX_ARCH_NAME_LENGTH];
    EXPECT_FALSE(Dmod_GetFileArchitecture(NULL, arch, sizeof(arch)));
}

/**
 * @brief Test Dmod_GetFileArchitecture with NULL output buffer
 */
TEST_F(DmodArchTest, GetFileArchitectureNullOutput) {
    EXPECT_FALSE(Dmod_GetFileArchitecture("test.dmf", NULL, 64));
}

/**
 * @brief Test Dmod_GetFileArchitecture with zero length
 */
TEST_F(DmodArchTest, GetFileArchitectureZeroLength) {
    char arch[DMOD_MAX_ARCH_NAME_LENGTH];
    EXPECT_FALSE(Dmod_GetFileArchitecture("test.dmf", arch, 0));
}

/**
 * @brief Test Dmod_GetFileArchitecture with non-existent file
 */
TEST_F(DmodArchTest, GetFileArchitectureNonExistentFile) {
    char arch[DMOD_MAX_ARCH_NAME_LENGTH];
    EXPECT_FALSE(Dmod_GetFileArchitecture("/tmp/nonexistent_file_12345.dmf", arch, sizeof(arch)));
}
