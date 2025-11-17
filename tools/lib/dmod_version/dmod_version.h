/**
 * @file dmod_version.h
 * @brief DMOD Version Utilities
 * 
 * This library provides functionality for parsing and comparing semantic versions,
 * including support for version ranges.
 * 
 * Supports version formats:
 * - Simple versions: "1.0", "1.2.3"
 * - Version ranges: ">=1.0", "<=2.0", ">=1.0<=2.0"
 */

#ifndef DMOD_VERSION_H
#define DMOD_VERSION_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum length for version strings
 */
#define DMOD_VERSION_MAX_LEN 32

/**
 * @brief Semantic version structure
 */
typedef struct {
    int major;  /**< Major version number */
    int minor;  /**< Minor version number */
    int patch;  /**< Patch version number */
} Dmod_SemanticVersion_t;

/**
 * @brief Version comparison operators
 */
typedef enum {
    DMOD_VERSION_OP_NONE,      /**< No operator - exact match */
    DMOD_VERSION_OP_GTE,       /**< Greater than or equal (>=) */
    DMOD_VERSION_OP_LTE,       /**< Less than or equal (<=) */
    DMOD_VERSION_OP_GT,        /**< Greater than (>) */
    DMOD_VERSION_OP_LT,        /**< Less than (<) */
    DMOD_VERSION_OP_EQ         /**< Equal (=) */
} Dmod_VersionOperator_t;

/**
 * @brief Version constraint structure
 */
typedef struct {
    Dmod_VersionOperator_t min_op;  /**< Minimum version operator */
    Dmod_SemanticVersion_t min_ver; /**< Minimum version */
    Dmod_VersionOperator_t max_op;  /**< Maximum version operator */
    Dmod_SemanticVersion_t max_ver; /**< Maximum version */
    bool has_min;                    /**< Whether minimum constraint exists */
    bool has_max;                    /**< Whether maximum constraint exists */
} Dmod_VersionConstraint_t;

/**
 * @brief Parse a version string into semantic version structure
 * 
 * Parses version strings in format "major.minor.patch" or "major.minor" or "major"
 * 
 * @param version_str Version string to parse
 * @param out_version Pointer to store parsed version
 * @return true if parsing succeeded, false otherwise
 */
bool Dmod_Version_Parse(const char* version_str, Dmod_SemanticVersion_t* out_version);

/**
 * @brief Parse a version constraint string
 * 
 * Parses version constraint strings like ">=1.0", "<=2.0", ">=1.0<=2.0"
 * 
 * @param constraint_str Constraint string to parse
 * @param out_constraint Pointer to store parsed constraint
 * @return true if parsing succeeded, false otherwise
 */
bool Dmod_Version_ParseConstraint(const char* constraint_str, Dmod_VersionConstraint_t* out_constraint);

/**
 * @brief Compare two semantic versions
 * 
 * @param v1 First version
 * @param v2 Second version
 * @return -1 if v1 < v2, 0 if v1 == v2, 1 if v1 > v2
 */
int Dmod_Version_Compare(const Dmod_SemanticVersion_t* v1, const Dmod_SemanticVersion_t* v2);

/**
 * @brief Check if a version satisfies a constraint
 * 
 * @param version Version to check
 * @param constraint Constraint to check against
 * @return true if version satisfies constraint, false otherwise
 */
bool Dmod_Version_Satisfies(const Dmod_SemanticVersion_t* version, const Dmod_VersionConstraint_t* constraint);

/**
 * @brief Check if two versions are major-version compatible
 * 
 * Two versions are compatible if they have the same major version number.
 * For example, 1.0 is compatible with 1.2, but not with 2.0
 * 
 * @param v1 First version
 * @param v2 Second version
 * @return true if versions are compatible, false otherwise
 */
bool Dmod_Version_IsCompatible(const Dmod_SemanticVersion_t* v1, const Dmod_SemanticVersion_t* v2);

/**
 * @brief Convert a version to string
 * 
 * @param version Version to convert
 * @param buffer Buffer to store string
 * @param buffer_size Size of buffer
 * @return true if conversion succeeded, false if buffer too small
 */
bool Dmod_Version_ToString(const Dmod_SemanticVersion_t* version, char* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* DMOD_VERSION_H */
