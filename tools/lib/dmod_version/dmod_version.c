/**
 * @file dmod_version.c
 * @brief Implementation of DMOD Version Utilities
 */

#include "dmod_version.h"
#include "dmod.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/**
 * @brief Parse a version string into semantic version structure
 */
bool Dmod_Version_Parse(const char* version_str, Dmod_SemanticVersion_t* out_version) {
    if (!version_str || !out_version) {
        return false;
    }
    
    // Initialize to zeros
    out_version->major = 0;
    out_version->minor = 0;
    out_version->patch = 0;
    
    // Skip whitespace
    while (isspace((unsigned char)*version_str)) {
        version_str++;
    }
    
    // Empty string
    if (*version_str == '\0') {
        return false;
    }
    
    // Parse major version
    char* endptr;
    long major = strtol(version_str, &endptr, 10);
    if (endptr == version_str || major < 0) {
        return false;
    }
    out_version->major = (int)major;
    version_str = endptr;
    
    // Check for minor version
    if (*version_str == '.') {
        version_str++;
        long minor = strtol(version_str, &endptr, 10);
        if (endptr == version_str || minor < 0) {
            return false;
        }
        out_version->minor = (int)minor;
        version_str = endptr;
        
        // Check for patch version
        if (*version_str == '.') {
            version_str++;
            long patch = strtol(version_str, &endptr, 10);
            if (endptr == version_str || patch < 0) {
                return false;
            }
            out_version->patch = (int)patch;
            version_str = endptr;
        }
    }
    
    // Skip trailing whitespace
    while (isspace((unsigned char)*version_str)) {
        version_str++;
    }
    
    // Should be at end of string
    return *version_str == '\0';
}

/**
 * @brief Parse operator from string
 */
static bool ParseOperator(const char** str, Dmod_VersionOperator_t* op) {
    const char* s = *str;
    
    // Skip whitespace
    while (isspace((unsigned char)*s)) {
        s++;
    }
    
    if (s[0] == '>') {
        if (s[1] == '=') {
            *op = DMOD_VERSION_OP_GTE;
            *str = s + 2;
            return true;
        } else {
            *op = DMOD_VERSION_OP_GT;
            *str = s + 1;
            return true;
        }
    } else if (s[0] == '<') {
        if (s[1] == '=') {
            *op = DMOD_VERSION_OP_LTE;
            *str = s + 2;
            return true;
        } else {
            *op = DMOD_VERSION_OP_LT;
            *str = s + 1;
            return true;
        }
    } else if (s[0] == '=') {
        *op = DMOD_VERSION_OP_EQ;
        *str = s + 1;
        return true;
    }
    
    return false;
}

/**
 * @brief Parse a version constraint string
 */
bool Dmod_Version_ParseConstraint(const char* constraint_str, Dmod_VersionConstraint_t* out_constraint) {
    if (!constraint_str || !out_constraint) {
        return false;
    }
    
    // Initialize constraint
    memset(out_constraint, 0, sizeof(Dmod_VersionConstraint_t));
    
    const char* ptr = constraint_str;
    
    // Skip leading whitespace
    while (isspace((unsigned char)*ptr)) {
        ptr++;
    }
    
    // Try to parse first operator
    Dmod_VersionOperator_t first_op;
    if (ParseOperator(&ptr, &first_op)) {
        // Has operator, parse version
        char version_buf[DMOD_VERSION_MAX_LEN];
        const char* version_start = ptr;
        
        // Find end of version (next operator, whitespace, or end of string)
        while (*ptr && !isspace((unsigned char)*ptr) && 
               *ptr != '>' && *ptr != '<' && *ptr != '=') {
            ptr++;
        }
        
        size_t version_len = ptr - version_start;
        if (version_len == 0 || version_len >= DMOD_VERSION_MAX_LEN) {
            return false;
        }
        
        strncpy(version_buf, version_start, version_len);
        version_buf[version_len] = '\0';
        
        Dmod_SemanticVersion_t version;
        if (!Dmod_Version_Parse(version_buf, &version)) {
            return false;
        }
        
        // Store as minimum constraint
        out_constraint->min_op = first_op;
        out_constraint->min_ver = version;
        out_constraint->has_min = true;
        
        // Check for second constraint
        Dmod_VersionOperator_t second_op;
        if (ParseOperator(&ptr, &second_op)) {
            // Parse second version
            version_start = ptr;
            
            while (*ptr && !isspace((unsigned char)*ptr)) {
                ptr++;
            }
            
            version_len = ptr - version_start;
            if (version_len == 0 || version_len >= DMOD_VERSION_MAX_LEN) {
                return false;
            }
            
            strncpy(version_buf, version_start, version_len);
            version_buf[version_len] = '\0';
            
            if (!Dmod_Version_Parse(version_buf, &version)) {
                return false;
            }
            
            // Store as maximum constraint
            out_constraint->max_op = second_op;
            out_constraint->max_ver = version;
            out_constraint->has_max = true;
        }
    } else {
        // No operator, just version - exact match
        Dmod_SemanticVersion_t version;
        if (!Dmod_Version_Parse(ptr, &version)) {
            return false;
        }
        
        out_constraint->min_op = DMOD_VERSION_OP_EQ;
        out_constraint->min_ver = version;
        out_constraint->has_min = true;
    }
    
    return true;
}

/**
 * @brief Compare two semantic versions
 */
int Dmod_Version_Compare(const Dmod_SemanticVersion_t* v1, const Dmod_SemanticVersion_t* v2) {
    if (!v1 || !v2) {
        return 0;
    }
    
    if (v1->major != v2->major) {
        return v1->major < v2->major ? -1 : 1;
    }
    
    if (v1->minor != v2->minor) {
        return v1->minor < v2->minor ? -1 : 1;
    }
    
    if (v1->patch != v2->patch) {
        return v1->patch < v2->patch ? -1 : 1;
    }
    
    return 0;
}

/**
 * @brief Check if a version satisfies a single constraint
 */
static bool SatisfiesSingleConstraint(const Dmod_SemanticVersion_t* version, 
                                       Dmod_VersionOperator_t op,
                                       const Dmod_SemanticVersion_t* constraint_ver) {
    int cmp = Dmod_Version_Compare(version, constraint_ver);
    
    switch (op) {
        case DMOD_VERSION_OP_EQ:
            return cmp == 0;
        case DMOD_VERSION_OP_GT:
            return cmp > 0;
        case DMOD_VERSION_OP_GTE:
            return cmp >= 0;
        case DMOD_VERSION_OP_LT:
            return cmp < 0;
        case DMOD_VERSION_OP_LTE:
            return cmp <= 0;
        case DMOD_VERSION_OP_NONE:
            return true;
    }
    
    return false;
}

/**
 * @brief Check if a version satisfies a constraint
 */
bool Dmod_Version_Satisfies(const Dmod_SemanticVersion_t* version, const Dmod_VersionConstraint_t* constraint) {
    if (!version || !constraint) {
        return false;
    }
    
    // Check minimum constraint
    if (constraint->has_min) {
        if (!SatisfiesSingleConstraint(version, constraint->min_op, &constraint->min_ver)) {
            return false;
        }
    }
    
    // Check maximum constraint
    if (constraint->has_max) {
        if (!SatisfiesSingleConstraint(version, constraint->max_op, &constraint->max_ver)) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Check if two versions are major-version compatible
 */
bool Dmod_Version_IsCompatible(const Dmod_SemanticVersion_t* v1, const Dmod_SemanticVersion_t* v2) {
    if (!v1 || !v2) {
        return false;
    }
    
    return v1->major == v2->major;
}

/**
 * @brief Convert a version to string
 */
bool Dmod_Version_ToString(const Dmod_SemanticVersion_t* version, char* buffer, size_t buffer_size) {
    if (!version || !buffer || buffer_size == 0) {
        return false;
    }
    
    int written = Dmod_SnPrintf(buffer, buffer_size, "%d.%d.%d", 
                                version->major, version->minor, version->patch);
    
    return written > 0 && (size_t)written < buffer_size;
}
