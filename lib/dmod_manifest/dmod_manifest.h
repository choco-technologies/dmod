/**
 * @file dmod_manifest.h
 * @brief DMOD Manifest Parser Library
 * 
 * This library provides functionality for parsing DMOD manifest files (.dmm)
 * which contain module package information including URLs, versions, and dependencies.
 * 
 * Manifest format supports:
 * - Comments (lines starting with #)
 * - Module entries: module[@version] url
 * - Include directives: $include url
 * - Variable substitution: <tools_name> and <arch_name>
 */

#ifndef DMOD_MANIFEST_H
#define DMOD_MANIFEST_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum length for module names
 */
#define DMOD_MANIFEST_MAX_NAME_LEN 128

/**
 * @brief Maximum length for URLs
 */
#define DMOD_MANIFEST_MAX_URL_LEN 512

/**
 * @brief Maximum length for version strings
 */
#define DMOD_MANIFEST_MAX_VERSION_LEN 32

/**
 * @brief Represents a single manifest entry
 */
typedef struct {
    char name[DMOD_MANIFEST_MAX_NAME_LEN];      /**< Module name */
    char version[DMOD_MANIFEST_MAX_VERSION_LEN]; /**< Module version (empty if not specified) */
    char url[DMOD_MANIFEST_MAX_URL_LEN];        /**< Download URL */
} Dmod_ManifestEntry_t;

/**
 * @brief Opaque manifest context structure
 */
typedef struct Dmod_ManifestContext Dmod_ManifestContext_t;

/**
 * @brief Function pointer type for downloading content from URLs
 * 
 * @param url The URL to download from
 * @param buffer Pointer to store the downloaded content (caller must free)
 * @param size Pointer to store the size of downloaded content
 * @param user_data User-provided data passed through
 * @return true if download succeeded, false otherwise
 */
typedef bool (*Dmod_DownloadFunc_t)(const char* url, char** buffer, size_t* size, void* user_data);

/**
 * @brief Initialize a new manifest context
 * 
 * @param tools_name The tools name for variable substitution (e.g., "arch/x86_64")
 * @param download_func Function to download content from URLs
 * @param user_data User data to pass to download function
 * @return Pointer to manifest context, or NULL on failure
 */
Dmod_ManifestContext_t* Dmod_Manifest_Init(
    const char* tools_name,
    Dmod_DownloadFunc_t download_func,
    void* user_data
);

/**
 * @brief Free a manifest context and all associated resources
 * 
 * @param ctx Manifest context to free
 */
void Dmod_Manifest_Free(Dmod_ManifestContext_t* ctx);

/**
 * @brief Parse a manifest from a string
 * 
 * @param ctx Manifest context
 * @param manifest_content The manifest content as a string
 * @return true if parsing succeeded, false otherwise
 */
bool Dmod_Manifest_Parse(Dmod_ManifestContext_t* ctx, const char* manifest_content);

/**
 * @brief Parse a manifest from a file
 * 
 * @param ctx Manifest context
 * @param file_path Path to the manifest file
 * @return true if parsing succeeded, false otherwise
 */
bool Dmod_Manifest_ParseFile(Dmod_ManifestContext_t* ctx, const char* file_path);

/**
 * @brief Parse a manifest from a URL
 * 
 * @param ctx Manifest context
 * @param url URL to download and parse the manifest from
 * @return true if parsing succeeded, false otherwise
 */
bool Dmod_Manifest_ParseUrl(Dmod_ManifestContext_t* ctx, const char* url);

/**
 * @brief Find the best matching entry for a module
 * 
 * Finds the entry that best matches the given module name and version.
 * If version is NULL or empty, returns the latest version or first match.
 * 
 * @param ctx Manifest context
 * @param name Module name to search for
 * @param version Module version to match (can be NULL for any version)
 * @param out_entry Pointer to store the found entry
 * @return true if a matching entry was found, false otherwise
 */
bool Dmod_Manifest_FindEntry(
    Dmod_ManifestContext_t* ctx,
    const char* name,
    const char* version,
    Dmod_ManifestEntry_t* out_entry
);

/**
 * @brief Get the number of entries in the manifest
 * 
 * @param ctx Manifest context
 * @return Number of entries
 */
size_t Dmod_Manifest_GetEntryCount(Dmod_ManifestContext_t* ctx);

/**
 * @brief Get an entry by index
 * 
 * @param ctx Manifest context
 * @param index Entry index
 * @param out_entry Pointer to store the entry
 * @return true if entry was retrieved, false if index out of bounds
 */
bool Dmod_Manifest_GetEntry(
    Dmod_ManifestContext_t* ctx,
    size_t index,
    Dmod_ManifestEntry_t* out_entry
);

/**
 * @brief Get the last error message
 * 
 * @param ctx Manifest context
 * @return Error message string, or NULL if no error
 */
const char* Dmod_Manifest_GetError(Dmod_ManifestContext_t* ctx);

#ifdef __cplusplus
}
#endif

#endif /* DMOD_MANIFEST_H */
