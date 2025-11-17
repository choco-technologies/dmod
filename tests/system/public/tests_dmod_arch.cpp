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
 * @brief Test Dmod_GetPackageArchitecture with NULL parameters
 */
TEST_F(DmodArchTest, GetPackageArchitectureNullPath) {
    char arch[DMOD_MAX_ARCH_NAME_LENGTH];
    EXPECT_FALSE(Dmod_GetPackageArchitecture(NULL, arch, sizeof(arch)));
}

/**
 * @brief Test Dmod_GetPackageArchitecture with NULL output buffer
 */
TEST_F(DmodArchTest, GetPackageArchitectureNullOutput) {
    EXPECT_FALSE(Dmod_GetPackageArchitecture("test.dmf", NULL, 64));
}

/**
 * @brief Test Dmod_GetPackageArchitecture with zero length
 */
TEST_F(DmodArchTest, GetPackageArchitectureZeroLength) {
    char arch[DMOD_MAX_ARCH_NAME_LENGTH];
    EXPECT_FALSE(Dmod_GetPackageArchitecture("test.dmf", arch, 0));
}

/**
 * @brief Test Dmod_GetPackageArchitecture with non-existent file
 */
TEST_F(DmodArchTest, GetPackageArchitectureNonExistentFile) {
    char arch[DMOD_MAX_ARCH_NAME_LENGTH];
    EXPECT_FALSE(Dmod_GetPackageArchitecture("/tmp/nonexistent_file_12345.dmf", arch, sizeof(arch)));
}

/**
 * @brief Test that DMOD_ARCH is defined
 */
TEST_F(DmodArchTest, DmodArchDefined) {
    // DMOD_ARCH should be defined at compile time
    const char* arch = DMOD_ARCH;
    EXPECT_NE(arch, nullptr);
    EXPECT_GT(strlen(arch), 0);
    
    // Verify it's a reasonable architecture string
    EXPECT_TRUE(
        strcmp(arch, "x86_64") == 0 ||
        strcmp(arch, "x86") == 0 ||
        strcmp(arch, "ARM") == 0 ||
        strcmp(arch, "ARM64") == 0 ||
        strcmp(arch, "ARMv7") == 0 ||
        strcmp(arch, "ARMv4") == 0 ||
        strcmp(arch, "PPC") == 0 ||
        strcmp(arch, "MIPS") == 0 ||
        strcmp(arch, "RISCV32") == 0 ||
        strcmp(arch, "RISCV64") == 0 ||
        strcmp(arch, "RISCV") == 0 ||
        strncmp(arch, "ARMv", 4) == 0  // For other ARM versions
    ) << "Unexpected architecture: " << arch;
}
