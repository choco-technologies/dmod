/**
 * @file dmod_dependencies.h
 * @brief DMOD Dependencies Parser Library
 * 
 * This library provides functionality for parsing DMOD dependencies files (.dmd)
 * which contain lists of modules to download with their versions and sources.
 * 
 * Dependencies format supports:
 * - Comments (lines starting with #)
 * - Module entries: module[@version] or module[@version_constraint]
 * - Version constraints: >=1.0, <=2.0, >=1.0<=2.0
 * - Include directives: $include url
 * - Source directives: from: manifest_url
 */

#ifndef DMOD_DEPENDENCIES_H
#define DMOD_DEPENDENCIES_H

#include <stddef.h>
#include <stdbool.h>
#include "dmod_version.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum length for module names
 */
#define DMOD_DEPENDENCIES_MAX_NAME_LEN 128

/**
 * @brief Maximum length for version strings
 */
#define DMOD_DEPENDENCIES_MAX_VERSION_LEN 32

/**
 * @brief Maximum length for manifest URLs
 */
#define DMOD_DEPENDENCIES_MAX_URL_LEN 512

/**
 * @brief Maximum length for configuration paths
 */
#define DMOD_DEPENDENCIES_MAX_CONFIG_LEN 256

/**
 * @brief Represents a single dependency entry
 */
typedef struct {
    char name[DMOD_DEPENDENCIES_MAX_NAME_LEN];      /**< Module name */
    char version[DMOD_DEPENDENCIES_MAX_VERSION_LEN]; /**< Module version or constraint (empty if not specified) */
    char manifest[DMOD_DEPENDENCIES_MAX_URL_LEN];   /**< Manifest URL to use for this module */
    char config[DMOD_DEPENDENCIES_MAX_CONFIG_LEN];  /**< Configuration file path (empty if not specified) */
    Dmod_VersionConstraint_t constraint;             /**< Parsed version constraint */
    bool has_constraint;                             /**< Whether constraint is parsed and valid */
} Dmod_DependencyEntry_t;

/**
 * @brief Opaque dependencies context structure
 */
typedef struct Dmod_DependenciesContext Dmod_DependenciesContext_t;

/**
 * @brief Function pointer type for downloading content from URLs
 * 
 * @param url The URL to download from
 * @param buffer Pointer to store the downloaded content (caller must free)
 * @param size Pointer to store the size of downloaded content
 * @param user_data User-provided data passed through
 * @return true if download succeeded, false otherwise
 */
typedef bool (*Dmod_DepDownloadFunc_t)(const char* url, char** buffer, size_t* size, void* user_data);

/**
 * @brief Initialize a new dependencies context
 * 
 * @param default_manifest Default manifest URL to use for modules
 * @param download_func Function to download content from URLs
 * @param user_data User data to pass to download function
 * @return Pointer to dependencies context, or NULL on failure
 */
Dmod_DependenciesContext_t* Dmod_Dependencies_Init(
    const char* default_manifest,
    Dmod_DepDownloadFunc_t download_func,
    void* user_data
);

/**
 * @brief Free a dependencies context and all associated resources
 * 
 * @param ctx Dependencies context to free
 */
void Dmod_Dependencies_Free(Dmod_DependenciesContext_t* ctx);

/**
 * @brief Parse dependencies from a string
 * 
 * @param ctx Dependencies context
 * @param dependencies_content The dependencies content as a string
 * @return true if parsing succeeded, false otherwise
 */
bool Dmod_Dependencies_Parse(Dmod_DependenciesContext_t* ctx, const char* dependencies_content);

/**
 * @brief Parse dependencies from a file
 * 
 * @param ctx Dependencies context
 * @param file_path Path to the dependencies file
 * @return true if parsing succeeded, false otherwise
 */
bool Dmod_Dependencies_ParseFile(Dmod_DependenciesContext_t* ctx, const char* file_path);

/**
 * @brief Parse dependencies from a URL
 * 
 * @param ctx Dependencies context
 * @param url URL to download and parse the dependencies from
 * @return true if parsing succeeded, false otherwise
 */
bool Dmod_Dependencies_ParseUrl(Dmod_DependenciesContext_t* ctx, const char* url);

/**
 * @brief Get the number of entries in the dependencies list
 * 
 * @param ctx Dependencies context
 * @return Number of entries
 */
size_t Dmod_Dependencies_GetEntryCount(Dmod_DependenciesContext_t* ctx);

/**
 * @brief Get an entry by index
 * 
 * @param ctx Dependencies context
 * @param index Entry index
 * @param out_entry Pointer to store the entry
 * @return true if entry was retrieved, false if index out of bounds
 */
bool Dmod_Dependencies_GetEntry(
    Dmod_DependenciesContext_t* ctx,
    size_t index,
    Dmod_DependencyEntry_t* out_entry
);

/**
 * @brief Get the last error message
 * 
 * @param ctx Dependencies context
 * @return Error message string, or NULL if no error
 */
const char* Dmod_Dependencies_GetError(Dmod_DependenciesContext_t* ctx);

#ifdef __cplusplus
}
#endif

#endif /* DMOD_DEPENDENCIES_H */
