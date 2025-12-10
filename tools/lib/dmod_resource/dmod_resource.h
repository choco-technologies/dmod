/**
 * @file dmod_resource.h
 * @brief DMOD Resource Parser Library
 * 
 * This library provides functionality for parsing DMOD resource files (.dmr)
 * which contain resource mappings for module installations, showing what files
 * should be installed and where.
 * 
 * Resource format supports:
 * - Comments (lines starting with #)
 * - Resource entries: key=source_path => destination_path
 * - Environment variable substitution: ${VAR_NAME}
 * - Special variables: ${destination}, ${module}
 */

#ifndef DMOD_RESOURCE_H
#define DMOD_RESOURCE_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum length for resource keys
 */
#define DMOD_RESOURCE_MAX_KEY_LEN 128

/**
 * @brief Maximum length for paths
 */
#define DMOD_RESOURCE_MAX_PATH_LEN 512

/**
 * @brief Represents a single resource entry
 */
typedef struct {
    char key[DMOD_RESOURCE_MAX_KEY_LEN];           /**< Resource key (e.g., "dmf", "docs") */
    char source[DMOD_RESOURCE_MAX_PATH_LEN];       /**< Source path in zip */
    char destination[DMOD_RESOURCE_MAX_PATH_LEN];  /**< Destination path after substitution */
    bool is_dmf_dmfc;                               /**< True if this is a dmf or dmfc resource */
} Dmod_ResourceEntry_t;

/**
 * @brief Opaque resource context structure
 */
typedef struct Dmod_ResourceContext Dmod_ResourceContext_t;

/**
 * @brief Initialize a new resource context
 * 
 * @param destination_path The destination path for ${destination} substitution
 * @param module_name The module name for ${module} substitution
 * @return Pointer to resource context, or NULL on failure
 */
Dmod_ResourceContext_t* Dmod_Resource_Init(
    const char* destination_path,
    const char* module_name
);

/**
 * @brief Free a resource context and all associated resources
 * 
 * @param ctx Resource context to free
 */
void Dmod_Resource_Free(Dmod_ResourceContext_t* ctx);

/**
 * @brief Parse resources from a string
 * 
 * @param ctx Resource context
 * @param resource_content The resource content as a string
 * @return true if parsing succeeded, false otherwise
 */
bool Dmod_Resource_Parse(Dmod_ResourceContext_t* ctx, const char* resource_content);

/**
 * @brief Parse resources from a file
 * 
 * @param ctx Resource context
 * @param file_path Path to the resource file
 * @return true if parsing succeeded, false otherwise
 */
bool Dmod_Resource_ParseFile(Dmod_ResourceContext_t* ctx, const char* file_path);

/**
 * @brief Get the number of entries in the resource list
 * 
 * @param ctx Resource context
 * @return Number of entries
 */
size_t Dmod_Resource_GetEntryCount(Dmod_ResourceContext_t* ctx);

/**
 * @brief Get an entry by index
 * 
 * @param ctx Resource context
 * @param index Entry index
 * @param out_entry Pointer to store the entry
 * @return true if entry was retrieved, false if index out of bounds
 */
bool Dmod_Resource_GetEntry(
    Dmod_ResourceContext_t* ctx,
    size_t index,
    Dmod_ResourceEntry_t* out_entry
);

/**
 * @brief Get the last error message
 * 
 * @param ctx Resource context
 * @return Error message string, or NULL if no error
 */
const char* Dmod_Resource_GetError(Dmod_ResourceContext_t* ctx);

#ifdef __cplusplus
}
#endif

#endif /* DMOD_RESOURCE_H */
