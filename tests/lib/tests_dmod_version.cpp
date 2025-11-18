/**
 * @file tests_dmod_version.cpp
 * @brief Unit tests for DMOD Version Utilities Library
 */

#include <gtest/gtest.h>
#include <string.h>
#include "dmod_version.h"

// ===============================================================
//                  Version Parsing Tests
// ===============================================================

TEST(DmodVersionTest, ParseSimpleVersion) {
    Dmod_SemanticVersion_t version;
    
    ASSERT_TRUE(Dmod_Version_Parse("1.0.0", &version));
    EXPECT_EQ(version.major, 1);
    EXPECT_EQ(version.minor, 0);
    EXPECT_EQ(version.patch, 0);
}

TEST(DmodVersionTest, ParseVersionWithMinorOnly) {
    Dmod_SemanticVersion_t version;
    
    ASSERT_TRUE(Dmod_Version_Parse("2.5", &version));
    EXPECT_EQ(version.major, 2);
    EXPECT_EQ(version.minor, 5);
    EXPECT_EQ(version.patch, 0);
}

TEST(DmodVersionTest, ParseVersionMajorOnly) {
    Dmod_SemanticVersion_t version;
    
    ASSERT_TRUE(Dmod_Version_Parse("3", &version));
    EXPECT_EQ(version.major, 3);
    EXPECT_EQ(version.minor, 0);
    EXPECT_EQ(version.patch, 0);
}

TEST(DmodVersionTest, ParseComplexVersion) {
    Dmod_SemanticVersion_t version;
    
    ASSERT_TRUE(Dmod_Version_Parse("2.5.3", &version));
    EXPECT_EQ(version.major, 2);
    EXPECT_EQ(version.minor, 5);
    EXPECT_EQ(version.patch, 3);
}

TEST(DmodVersionTest, ParseInvalidVersion) {
    Dmod_SemanticVersion_t version;
    
    EXPECT_FALSE(Dmod_Version_Parse("", &version));
    EXPECT_FALSE(Dmod_Version_Parse("abc", &version));
    EXPECT_FALSE(Dmod_Version_Parse("1.2.3.4", &version));
    EXPECT_FALSE(Dmod_Version_Parse(nullptr, &version));
}

TEST(DmodVersionTest, ParseVersionWithWhitespace) {
    Dmod_SemanticVersion_t version;
    
    ASSERT_TRUE(Dmod_Version_Parse("  1.0.0  ", &version));
    EXPECT_EQ(version.major, 1);
    EXPECT_EQ(version.minor, 0);
    EXPECT_EQ(version.patch, 0);
}

// ===============================================================
//                  Version Comparison Tests
// ===============================================================

TEST(DmodVersionTest, CompareEqualVersions) {
    Dmod_SemanticVersion_t v1, v2;
    
    Dmod_Version_Parse("1.0.0", &v1);
    Dmod_Version_Parse("1.0.0", &v2);
    
    EXPECT_EQ(Dmod_Version_Compare(&v1, &v2), 0);
}

TEST(DmodVersionTest, CompareDifferentMajor) {
    Dmod_SemanticVersion_t v1, v2;
    
    Dmod_Version_Parse("1.0.0", &v1);
    Dmod_Version_Parse("2.0.0", &v2);
    
    EXPECT_LT(Dmod_Version_Compare(&v1, &v2), 0);
    EXPECT_GT(Dmod_Version_Compare(&v2, &v1), 0);
}

TEST(DmodVersionTest, CompareDifferentMinor) {
    Dmod_SemanticVersion_t v1, v2;
    
    Dmod_Version_Parse("1.0.0", &v1);
    Dmod_Version_Parse("1.5.0", &v2);
    
    EXPECT_LT(Dmod_Version_Compare(&v1, &v2), 0);
    EXPECT_GT(Dmod_Version_Compare(&v2, &v1), 0);
}

TEST(DmodVersionTest, CompareDifferentPatch) {
    Dmod_SemanticVersion_t v1, v2;
    
    Dmod_Version_Parse("1.0.0", &v1);
    Dmod_Version_Parse("1.0.5", &v2);
    
    EXPECT_LT(Dmod_Version_Compare(&v1, &v2), 0);
    EXPECT_GT(Dmod_Version_Compare(&v2, &v1), 0);
}

// ===============================================================
//                  Constraint Parsing Tests
// ===============================================================

TEST(DmodVersionTest, ParseGreaterThanOrEqualConstraint) {
    Dmod_VersionConstraint_t constraint;
    
    ASSERT_TRUE(Dmod_Version_ParseConstraint(">=1.0", &constraint));
    EXPECT_TRUE(constraint.has_min);
    EXPECT_EQ(constraint.min_op, DMOD_VERSION_OP_GTE);
    EXPECT_EQ(constraint.min_ver.major, 1);
    EXPECT_EQ(constraint.min_ver.minor, 0);
    EXPECT_FALSE(constraint.has_max);
}

TEST(DmodVersionTest, ParseLessThanOrEqualConstraint) {
    Dmod_VersionConstraint_t constraint;
    
    ASSERT_TRUE(Dmod_Version_ParseConstraint("<=2.0", &constraint));
    EXPECT_TRUE(constraint.has_min);
    EXPECT_EQ(constraint.min_op, DMOD_VERSION_OP_LTE);
    EXPECT_EQ(constraint.min_ver.major, 2);
    EXPECT_EQ(constraint.min_ver.minor, 0);
    EXPECT_FALSE(constraint.has_max);
}

TEST(DmodVersionTest, ParseGreaterThanConstraint) {
    Dmod_VersionConstraint_t constraint;
    
    ASSERT_TRUE(Dmod_Version_ParseConstraint(">1.0", &constraint));
    EXPECT_TRUE(constraint.has_min);
    EXPECT_EQ(constraint.min_op, DMOD_VERSION_OP_GT);
}

TEST(DmodVersionTest, ParseLessThanConstraint) {
    Dmod_VersionConstraint_t constraint;
    
    ASSERT_TRUE(Dmod_Version_ParseConstraint("<2.0", &constraint));
    EXPECT_TRUE(constraint.has_min);
    EXPECT_EQ(constraint.min_op, DMOD_VERSION_OP_LT);
}

TEST(DmodVersionTest, ParseRangeConstraint) {
    Dmod_VersionConstraint_t constraint;
    
    ASSERT_TRUE(Dmod_Version_ParseConstraint(">=1.0<=2.0", &constraint));
    EXPECT_TRUE(constraint.has_min);
    EXPECT_TRUE(constraint.has_max);
    EXPECT_EQ(constraint.min_op, DMOD_VERSION_OP_GTE);
    EXPECT_EQ(constraint.min_ver.major, 1);
    EXPECT_EQ(constraint.max_op, DMOD_VERSION_OP_LTE);
    EXPECT_EQ(constraint.max_ver.major, 2);
}

TEST(DmodVersionTest, ParseExactVersionConstraint) {
    Dmod_VersionConstraint_t constraint;
    
    ASSERT_TRUE(Dmod_Version_ParseConstraint("1.5.0", &constraint));
    EXPECT_TRUE(constraint.has_min);
    EXPECT_EQ(constraint.min_op, DMOD_VERSION_OP_EQ);
    EXPECT_EQ(constraint.min_ver.major, 1);
    EXPECT_EQ(constraint.min_ver.minor, 5);
    EXPECT_EQ(constraint.min_ver.patch, 0);
}

// ===============================================================
//                  Version Satisfies Constraint Tests
// ===============================================================

TEST(DmodVersionTest, SatisfiesGreaterThanOrEqual) {
    Dmod_SemanticVersion_t version;
    Dmod_VersionConstraint_t constraint;
    
    Dmod_Version_Parse("1.5.0", &version);
    Dmod_Version_ParseConstraint(">=1.0", &constraint);
    
    EXPECT_TRUE(Dmod_Version_Satisfies(&version, &constraint));
    
    Dmod_Version_Parse("0.9.0", &version);
    EXPECT_FALSE(Dmod_Version_Satisfies(&version, &constraint));
}

TEST(DmodVersionTest, SatisfiesLessThanOrEqual) {
    Dmod_SemanticVersion_t version;
    Dmod_VersionConstraint_t constraint;
    
    Dmod_Version_Parse("1.5.0", &version);
    Dmod_Version_ParseConstraint("<=2.0", &constraint);
    
    EXPECT_TRUE(Dmod_Version_Satisfies(&version, &constraint));
    
    Dmod_Version_Parse("2.5.0", &version);
    EXPECT_FALSE(Dmod_Version_Satisfies(&version, &constraint));
}

TEST(DmodVersionTest, SatisfiesRange) {
    Dmod_SemanticVersion_t version;
    Dmod_VersionConstraint_t constraint;
    
    Dmod_Version_ParseConstraint(">=1.0<=2.0", &constraint);
    
    // Inside range
    Dmod_Version_Parse("1.5.0", &version);
    EXPECT_TRUE(Dmod_Version_Satisfies(&version, &constraint));
    
    // At lower bound
    Dmod_Version_Parse("1.0.0", &version);
    EXPECT_TRUE(Dmod_Version_Satisfies(&version, &constraint));
    
    // At upper bound
    Dmod_Version_Parse("2.0.0", &version);
    EXPECT_TRUE(Dmod_Version_Satisfies(&version, &constraint));
    
    // Below range
    Dmod_Version_Parse("0.9.0", &version);
    EXPECT_FALSE(Dmod_Version_Satisfies(&version, &constraint));
    
    // Above range
    Dmod_Version_Parse("2.1.0", &version);
    EXPECT_FALSE(Dmod_Version_Satisfies(&version, &constraint));
}

TEST(DmodVersionTest, SatisfiesExactVersion) {
    Dmod_SemanticVersion_t version;
    Dmod_VersionConstraint_t constraint;
    
    Dmod_Version_ParseConstraint("1.5.0", &constraint);
    
    Dmod_Version_Parse("1.5.0", &version);
    EXPECT_TRUE(Dmod_Version_Satisfies(&version, &constraint));
    
    Dmod_Version_Parse("1.5.1", &version);
    EXPECT_FALSE(Dmod_Version_Satisfies(&version, &constraint));
}

// ===============================================================
//                  Version Compatibility Tests
// ===============================================================

TEST(DmodVersionTest, CompatibleSameMajor) {
    Dmod_SemanticVersion_t v1, v2;
    
    Dmod_Version_Parse("1.0.0", &v1);
    Dmod_Version_Parse("1.5.0", &v2);
    
    EXPECT_TRUE(Dmod_Version_IsCompatible(&v1, &v2));
}

TEST(DmodVersionTest, IncompatibleDifferentMajor) {
    Dmod_SemanticVersion_t v1, v2;
    
    Dmod_Version_Parse("1.0.0", &v1);
    Dmod_Version_Parse("2.0.0", &v2);
    
    EXPECT_FALSE(Dmod_Version_IsCompatible(&v1, &v2));
}

TEST(DmodVersionTest, CompatibleDifferentMinorAndPatch) {
    Dmod_SemanticVersion_t v1, v2;
    
    Dmod_Version_Parse("1.2.3", &v1);
    Dmod_Version_Parse("1.9.8", &v2);
    
    EXPECT_TRUE(Dmod_Version_IsCompatible(&v1, &v2));
}

// ===============================================================
//                  Version To String Tests
// ===============================================================

TEST(DmodVersionTest, VersionToString) {
    Dmod_SemanticVersion_t version;
    char buffer[32];
    
    Dmod_Version_Parse("1.2.3", &version);
    ASSERT_TRUE(Dmod_Version_ToString(&version, buffer, sizeof(buffer)));
    EXPECT_STREQ(buffer, "1.2.3");
}

TEST(DmodVersionTest, VersionToStringBufferTooSmall) {
    Dmod_SemanticVersion_t version;
    char buffer[5];
    
    Dmod_Version_Parse("1.2.3", &version);
    EXPECT_FALSE(Dmod_Version_ToString(&version, buffer, sizeof(buffer)));
}
