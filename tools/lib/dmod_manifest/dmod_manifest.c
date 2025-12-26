/**
 * @file dmod_manifest.c
 * @brief DMOD Manifest Parser Library Implementation
 */

#include "dmod_manifest.h"
#include "dmod.h"
#include "dmod_version.h"
#include <string.h>
#include <ctype.h>

/**
 * @brief Internal manifest entry node for linked list
 */
typedef struct Dmod_ManifestNode {
    Dmod_ManifestEntry_t entry;
    struct Dmod_ManifestNode* next;
} Dmod_ManifestNode_t;

/**
 * @brief Available version list node
 */
typedef struct Dmod_AvailableVersionNode {
    char version[DMOD_MANIFEST_MAX_VERSION_LEN];
    struct Dmod_AvailableVersionNode* next;
} Dmod_AvailableVersionNode_t;

/**
 * @brief Available versions for a specific module
 */
typedef struct Dmod_ModuleVersionsNode {
    char module_name[DMOD_MANIFEST_MAX_NAME_LEN];
    Dmod_AvailableVersionNode_t* versions;
    struct Dmod_ModuleVersionsNode* next;
} Dmod_ModuleVersionsNode_t;

/**
 * @brief Manifest context structure
 */
struct Dmod_ManifestContext {
    char* tools_name;                    /**< Tools name for substitution */
    char* arch_name;                     /**< Architecture name for substitution */
    char* cpu_name;                      /**< CPU name for substitution (e.g., stm32f746ngh6) */
    char* cpu_family;                    /**< CPU family for substitution (e.g., stm32f7) */
    Dmod_DownloadFunc_t download_func;   /**< Download function */
    void* user_data;                     /**< User data for download function */
    Dmod_ManifestNode_t* entries;        /**< Linked list of entries */
    size_t entry_count;                  /**< Number of entries */
    char error[256];                     /**< Last error message */
    Dmod_SemanticVersion_t current_dmod_version; /**< Current DMOD version for entries */
    bool has_dmod_version;               /**< Whether current_dmod_version is set */
    Dmod_ModuleVersionsNode_t* module_versions; /**< List of available versions per module */
};

/**
 * @brief Convert tools_name to arch_name format
 * 
 * Converts "arch/armv7/cortex-m7" to "armv7-cortex-m7"
 * 
 * @param tools_name Input tools name
 * @return Allocated arch_name string (caller must free), or NULL on failure
 */
static char* ConvertToArchName(const char* tools_name) {
    if (!tools_name) return NULL;
    
    // Skip "arch/" prefix if present
    const char* start = tools_name;
    if (strncmp(start, "arch/", 5) == 0) {
        start += 5;
    }
    
    // Allocate buffer
    char* arch_name = Dmod_Malloc(strlen(start) + 1);
    if (!arch_name) return NULL;
    
    // Copy and replace '/' with '-'
    char* dst = arch_name;
    for (const char* src = start; *src; src++, dst++) {
        *dst = (*src == '/') ? '-' : *src;
    }
    *dst = '\0';
    
    return arch_name;
}

/**
 * @brief Substitute variables in a string
 * 
 * Replaces <tools_name>, <arch_name>, <cpu_name>, <cpu_family>, and <version> with actual values
 * 
 * @param ctx Manifest context
 * @param input Input string
 * @param output Output buffer
 * @param output_size Size of output buffer
 * @param version Optional version string (can be NULL)
 * @return true if substitution succeeded, false otherwise
 */
static bool SubstituteVariables(Dmod_ManifestContext_t* ctx, const char* input, char* output, size_t output_size, const char* version) {
    const char* src = input;
    char* dst = output;
    char* dst_end = output + output_size - 1;
    
    while (*src && dst < dst_end) {
        if (*src == '<') {
            // Check for <tools_name>
            if (strncmp(src, "<tools_name>", 12) == 0) {
                if (ctx->tools_name) {
                    size_t len = strlen(ctx->tools_name);
                    if (dst + len >= dst_end) return false;
                    strcpy(dst, ctx->tools_name);
                    dst += len;
                }
                src += 12;
            }
            // Check for <arch_name>
            else if (strncmp(src, "<arch_name>", 11) == 0) {
                if (ctx->arch_name) {
                    size_t len = strlen(ctx->arch_name);
                    if (dst + len >= dst_end) return false;
                    strcpy(dst, ctx->arch_name);
                    dst += len;
                }
                src += 11;
            }
            // Check for <cpu_name>
            else if (strncmp(src, "<cpu_name>", 10) == 0) {
                if (ctx->cpu_name) {
                    size_t len = strlen(ctx->cpu_name);
                    if (dst + len >= dst_end) return false;
                    strcpy(dst, ctx->cpu_name);
                    dst += len;
                }
                src += 10;
            }
            // Check for <cpu_family>
            else if (strncmp(src, "<cpu_family>", 12) == 0) {
                if (ctx->cpu_family) {
                    size_t len = strlen(ctx->cpu_family);
                    if (dst + len >= dst_end) return false;
                    strcpy(dst, ctx->cpu_family);
                    dst += len;
                }
                src += 12;
            }
            // Check for <version>
            else if (strncmp(src, "<version>", 9) == 0) {
                if (version && version[0] != '\0') {
                    size_t len = strlen(version);
                    if (dst + len >= dst_end) return false;
                    strcpy(dst, version);
                    dst += len;
                    src += 9;
                } else {
                    // Keep the placeholder if no version available
                    if (dst + 9 >= dst_end) return false;
                    strcpy(dst, "<version>");
                    dst += 9;
                    src += 9;
                }
            }
            else {
                *dst++ = *src++;
            }
        }
        else {
            *dst++ = *src++;
        }
    }
    
    *dst = '\0';
    return *src == '\0'; // True if we processed all input
}

/**
 * @brief Trim whitespace from both ends of a string in-place
 */
static char* TrimWhitespace(char* str) {
    if (!str) return NULL;
    
    // Trim leading whitespace
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == '\0') return str;
    
    // Trim trailing whitespace
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    
    return str;
}

/**
 * @brief Get or create available versions list for a module
 */
static Dmod_ModuleVersionsNode_t* GetOrCreateModuleVersions(Dmod_ManifestContext_t* ctx, const char* module_name) {
    if (!ctx || !module_name) return NULL;
    
    // Check if versions already exist for this module
    for (Dmod_ModuleVersionsNode_t* node = ctx->module_versions; node; node = node->next) {
        if (strcmp(node->module_name, module_name) == 0) {
            return node;
        }
    }
    
    // Create new module versions node
    Dmod_ModuleVersionsNode_t* new_node = Dmod_Malloc(sizeof(Dmod_ModuleVersionsNode_t));
    if (!new_node) return NULL;
    
    strncpy(new_node->module_name, module_name, sizeof(new_node->module_name) - 1);
    new_node->module_name[sizeof(new_node->module_name) - 1] = '\0';
    new_node->versions = NULL;
    new_node->next = ctx->module_versions;
    ctx->module_versions = new_node;
    
    return new_node;
}

/**
 * @brief Get available versions for a module
 */
static Dmod_AvailableVersionNode_t* GetAvailableVersions(Dmod_ManifestContext_t* ctx, const char* module_name) {
    if (!ctx || !module_name) return NULL;
    
    for (Dmod_ModuleVersionsNode_t* node = ctx->module_versions; node; node = node->next) {
        if (strcmp(node->module_name, module_name) == 0) {
            return node->versions;
        }
    }
    
    return NULL;
}

/**
 * @brief Parse a single manifest line
 * 
 * @param ctx Manifest context
 * @param line Line to parse
 * @return true if line was processed successfully (or was a comment/empty), false on error
 */
static bool ParseLine(Dmod_ManifestContext_t* ctx, char* line) {
    // Trim whitespace
    line = TrimWhitespace(line);
    
    // Skip empty lines and comments
    if (line[0] == '\0' || line[0] == '#') {
        return true;
    }
    
    // Check for $dmod-version directive
    if (strncmp(line, "$dmod-version", 13) == 0) {
        char* version_start = line + 13;
        version_start = TrimWhitespace(version_start);
        
        if (version_start[0] == '\0') {
            Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "$dmod-version directive requires a version");
            return false;
        }
        
        Dmod_SemanticVersion_t version;
        if (!Dmod_Version_Parse(version_start, &version)) {
            Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "Invalid version in $dmod-version: %s", version_start);
            return false;
        }
        
        ctx->current_dmod_version = version;
        ctx->has_dmod_version = true;
        
        DMOD_LOG_VERBOSE("Set DMOD version requirement to %d.%d.%d\n", 
                        version.major, version.minor, version.patch);
        
        return true;
    }
    
    // Check for $include directive
    if (strncmp(line, "$include", 8) == 0) {
        char* url_start = line + 8;
        url_start = TrimWhitespace(url_start);
        
        char url[DMOD_MANIFEST_MAX_URL_LEN];
        if (!SubstituteVariables(ctx, url_start, url, sizeof(url), NULL)) {
            Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "URL too long in $include: %s", url_start);
            return false;
        }

        DMOD_LOG_VERBOSE("Including manifest from URL: %s\n", url);
        
        // Parse the included manifest
        // Note: We continue parsing even if the included manifest fails to download
        // This prevents a single failed include from stopping the entire parsing process
        if (!Dmod_Manifest_ParseUrl(ctx, url)) {
            // Save the error message before logging to avoid format string issues
            char error_msg[256];
            Dmod_SnPrintf(error_msg, sizeof(error_msg), "%s", ctx->error);
            
            DMOD_LOG_WARN("Failed to include manifest from %s (continuing anyway): %s\n", 
                         url, error_msg);
            
            // Clear error so it doesn't affect subsequent parsing
            ctx->error[0] = '\0';
        }
        return true;
    }
    
    // Check for $version-available directive
    if (strncmp(line, "$version-available", 18) == 0) {
        char* rest = line + 18;
        rest = TrimWhitespace(rest);
        
        if (rest[0] == '\0') {
            Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "$version-available directive requires module name and versions");
            return false;
        }
        
        // Extract module name
        char module_name[DMOD_MANIFEST_MAX_NAME_LEN];
        char* space = strchr(rest, ' ');
        if (!space) {
            space = strchr(rest, '\t');
        }
        
        if (!space) {
            Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "$version-available directive requires versions after module name");
            return false;
        }
        
        size_t name_len = space - rest;
        if (name_len >= DMOD_MANIFEST_MAX_NAME_LEN) {
            Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "Module name too long in $version-available");
            return false;
        }
        
        strncpy(module_name, rest, name_len);
        module_name[name_len] = '\0';
        
        // Get or create module versions node
        Dmod_ModuleVersionsNode_t* mod_versions = GetOrCreateModuleVersions(ctx, module_name);
        if (!mod_versions) {
            Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "Out of memory");
            return false;
        }
        
        // Clear existing versions for this module
        Dmod_AvailableVersionNode_t* ver_node = mod_versions->versions;
        while (ver_node) {
            Dmod_AvailableVersionNode_t* next = ver_node->next;
            Dmod_Free(ver_node);
            ver_node = next;
        }
        mod_versions->versions = NULL;
        
        // Parse versions (space-separated list)
        char* versions_str = TrimWhitespace(space + 1);
        char* token = strtok(versions_str, " \t");
        
        while (token) {
            token = TrimWhitespace(token);
            if (token[0] != '\0') {
                // Create new version node
                Dmod_AvailableVersionNode_t* new_ver = Dmod_Malloc(sizeof(Dmod_AvailableVersionNode_t));
                if (!new_ver) {
                    Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "Out of memory");
                    return false;
                }
                
                strncpy(new_ver->version, token, sizeof(new_ver->version) - 1);
                new_ver->version[sizeof(new_ver->version) - 1] = '\0';
                new_ver->next = mod_versions->versions;
                mod_versions->versions = new_ver;
                
                DMOD_LOG_VERBOSE("Added available version %s for module %s\n", token, module_name);
            }
            token = strtok(NULL, " \t");
        }
        
        return true;
    }
    
    // Parse module entry: name[@version] url
    char name[DMOD_MANIFEST_MAX_NAME_LEN];
    char version[DMOD_MANIFEST_MAX_VERSION_LEN] = {0};
    char url_raw[DMOD_MANIFEST_MAX_URL_LEN];
    
    // Find first space - separates module identifier from URL
    char* space = strchr(line, ' ');
    if (!space) {
        space = strchr(line, '\t');
    }
    
    if (!space) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "Invalid manifest entry: %s", line);
        return false;
    }
    
    // Extract module identifier (name[@version])
    size_t id_len = space - line;
    if (id_len >= DMOD_MANIFEST_MAX_NAME_LEN) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "Module identifier too long: %s", line);
        return false;
    }
    
    char module_id[DMOD_MANIFEST_MAX_NAME_LEN];
    strncpy(module_id, line, id_len);
    module_id[id_len] = '\0';
    
    // Split name and version
    char* at_sign = strchr(module_id, '@');
    if (at_sign) {
        *at_sign = '\0';
        strncpy(name, module_id, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        strncpy(version, at_sign + 1, sizeof(version) - 1);
        version[sizeof(version) - 1] = '\0';
    } else {
        strncpy(name, module_id, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
    }
    
    // Extract and process URL
    char* url_start = TrimWhitespace(space + 1);
    strncpy(url_raw, url_start, sizeof(url_raw) - 1);
    url_raw[sizeof(url_raw) - 1] = '\0';
    
    // Check if we need to expand this entry using available versions
    bool has_explicit_version = (version[0] != '\0');
    bool url_has_version_placeholder = (strstr(url_raw, "<version>") != NULL);
    
    if (!has_explicit_version && url_has_version_placeholder) {
        // Try to get available versions for this module
        Dmod_AvailableVersionNode_t* available = GetAvailableVersions(ctx, name);
        
        if (available) {
            // Build temporary list of expanded entries in correct order
            // Versions are stored newest-first in the available list (due to prepending during parse).
            // We want entries newest-first in ctx->entries too.
            // Strategy: append each entry to temp list (so same order as available list),
            // then prepend entire temp list to ctx->entries (preserving the order).
            Dmod_ManifestNode_t* temp_head = NULL;
            Dmod_ManifestNode_t* temp_tail = NULL;
            size_t temp_count = 0;
            
            // Expand entry for each available version
            for (Dmod_AvailableVersionNode_t* ver = available; ver; ver = ver->next) {
                Dmod_ManifestNode_t* node = Dmod_Malloc(sizeof(Dmod_ManifestNode_t));
                if (!node) {
                    // Clean up temporary list on error
                    while (temp_head) {
                        Dmod_ManifestNode_t* next = temp_head->next;
                        Dmod_Free(temp_head);
                        temp_head = next;
                    }
                    Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "Out of memory");
                    return false;
                }
                
                // Fill entry data with version
                strncpy(node->entry.name, name, sizeof(node->entry.name) - 1);
                node->entry.name[sizeof(node->entry.name) - 1] = '\0';
                
                strncpy(node->entry.version, ver->version, sizeof(node->entry.version) - 1);
                node->entry.version[sizeof(node->entry.version) - 1] = '\0';
                
                if (!SubstituteVariables(ctx, url_raw, node->entry.url, sizeof(node->entry.url), ver->version)) {
                    Dmod_Free(node);
                    // Clean up temporary list on error
                    while (temp_head) {
                        Dmod_ManifestNode_t* next = temp_head->next;
                        Dmod_Free(temp_head);
                        temp_head = next;
                    }
                    Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "URL too long after substitution: %s", url_raw);
                    return false;
                }
                
                // Store DMOD version requirement if set
                if (ctx->has_dmod_version) {
                    node->entry.dmod_version = ctx->current_dmod_version;
                    node->entry.has_dmod_version = true;
                } else {
                    node->entry.has_dmod_version = false;
                }
                
                // Add to temporary list (append to end, so keeps same order as available list)
                node->next = NULL;
                if (!temp_head) {
                    temp_head = node;
                    temp_tail = node;
                } else {
                    temp_tail->next = node;
                    temp_tail = node;
                }
                temp_count++;
                
                DMOD_LOG_VERBOSE("Expanded entry: %s@%s -> %s\n", name, ver->version, node->entry.url);
            }
            
            // Now prepend the entire temporary list to ctx->entries
            // This maintains newest-first order from the available list
            if (temp_tail) {
                temp_tail->next = ctx->entries;
                ctx->entries = temp_head;
                ctx->entry_count += temp_count;
            }
            
            return true;
        }
    }
    
    // Create single entry (normal case or no available versions defined)
    Dmod_ManifestNode_t* node = Dmod_Malloc(sizeof(Dmod_ManifestNode_t));
    if (!node) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "Out of memory");
        return false;
    }
    
    // Fill entry data
    strncpy(node->entry.name, name, sizeof(node->entry.name) - 1);
    node->entry.name[sizeof(node->entry.name) - 1] = '\0';
    
    strncpy(node->entry.version, version, sizeof(node->entry.version) - 1);
    node->entry.version[sizeof(node->entry.version) - 1] = '\0';
    
    if (!SubstituteVariables(ctx, url_raw, node->entry.url, sizeof(node->entry.url), version)) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "URL too long after substitution: %s", url_raw);
        Dmod_Free(node);
        return false;
    }
    
    // Store DMOD version requirement if set
    if (ctx->has_dmod_version) {
        node->entry.dmod_version = ctx->current_dmod_version;
        node->entry.has_dmod_version = true;
    } else {
        node->entry.has_dmod_version = false;
    }
    
    // Add to linked list
    node->next = ctx->entries;
    ctx->entries = node;
    ctx->entry_count++;
    
    return true;
}

Dmod_ManifestContext_t* Dmod_Manifest_Init(
    const char* tools_name,
    const char* arch_name,
    const char* cpu_name,
    const char* cpu_family,
    Dmod_DownloadFunc_t download_func,
    void* user_data
) {
    Dmod_ManifestContext_t* ctx = Dmod_Malloc(sizeof(Dmod_ManifestContext_t));
    if (!ctx) return NULL;
    
    memset(ctx, 0, sizeof(Dmod_ManifestContext_t));
    
    if (tools_name) {
        size_t len = strlen(tools_name);
        ctx->tools_name = Dmod_Malloc(len + 1);
        if (!ctx->tools_name) {
            Dmod_Manifest_Free(ctx);
            return NULL;
        }
        strcpy(ctx->tools_name, tools_name);
    }
    
    // Use provided arch_name or convert from tools_name
    if (arch_name) {
        size_t len = strlen(arch_name);
        ctx->arch_name = Dmod_Malloc(len + 1);
        if (!ctx->arch_name) {
            Dmod_Manifest_Free(ctx);
            return NULL;
        }
        strcpy(ctx->arch_name, arch_name);
    } else if (tools_name) {
        ctx->arch_name = ConvertToArchName(tools_name);
        if (!ctx->arch_name) {
            Dmod_Manifest_Free(ctx);
            return NULL;
        }
    }
    
    // Store cpu_name if provided
    if (cpu_name) {
        size_t len = strlen(cpu_name);
        ctx->cpu_name = Dmod_Malloc(len + 1);
        if (!ctx->cpu_name) {
            Dmod_Manifest_Free(ctx);
            return NULL;
        }
        strcpy(ctx->cpu_name, cpu_name);
    }
    
    // Store cpu_family if provided
    if (cpu_family) {
        size_t len = strlen(cpu_family);
        ctx->cpu_family = Dmod_Malloc(len + 1);
        if (!ctx->cpu_family) {
            Dmod_Manifest_Free(ctx);
            return NULL;
        }
        strcpy(ctx->cpu_family, cpu_family);
    }
    
    ctx->download_func = download_func;
    ctx->user_data = user_data;
    
    return ctx;
}

void Dmod_Manifest_Free(Dmod_ManifestContext_t* ctx) {
    if (!ctx) return;
    
    // Free entries
    Dmod_ManifestNode_t* node = ctx->entries;
    while (node) {
        Dmod_ManifestNode_t* next = node->next;
        Dmod_Free(node);
        node = next;
    }
    
    // Free module versions
    Dmod_ModuleVersionsNode_t* mod_node = ctx->module_versions;
    while (mod_node) {
        Dmod_ModuleVersionsNode_t* mod_next = mod_node->next;
        
        // Free version list for this module
        Dmod_AvailableVersionNode_t* ver_node = mod_node->versions;
        while (ver_node) {
            Dmod_AvailableVersionNode_t* ver_next = ver_node->next;
            Dmod_Free(ver_node);
            ver_node = ver_next;
        }
        
        Dmod_Free(mod_node);
        mod_node = mod_next;
    }
    
    // Free strings
    if (ctx->tools_name) Dmod_Free(ctx->tools_name);
    if (ctx->arch_name) Dmod_Free(ctx->arch_name);
    if (ctx->cpu_name) Dmod_Free(ctx->cpu_name);
    if (ctx->cpu_family) Dmod_Free(ctx->cpu_family);
    Dmod_Free(ctx);
}

bool Dmod_Manifest_Parse(Dmod_ManifestContext_t* ctx, const char* manifest_content) {
    if (!ctx || !manifest_content) return false;
    
    // Make a copy of the content as we'll modify it
    size_t len = strlen(manifest_content);
    char* content = Dmod_Malloc(len + 1);
    if (!content) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "Out of memory");
        return false;
    }
    strcpy(content, manifest_content);
    
    bool success = true;
    char* line = content;
    char* next_line;
    
    while (line && *line) {
        // Find end of line
        next_line = strchr(line, '\n');
        if (next_line) {
            *next_line = '\0';
            next_line++;
        }
        
        // Parse the line
        if (!ParseLine(ctx, line)) {
            success = false;
            break;
        }
        
        line = next_line;
    }
    
    Dmod_Free(content);
    return success;
}

bool Dmod_Manifest_ParseFile(Dmod_ManifestContext_t* ctx, const char* file_path) {
    if (!ctx || !file_path) return false;
    
    void* file = Dmod_FileOpen(file_path, "r");
    if (!file) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "Cannot open file: %s", file_path);
        return false;
    }
    
    // Get file size
    size_t size = Dmod_FileSize(file);
    
    if (size > 1024 * 1024) { // Limit to 1MB
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "File too large: %s", file_path);
        Dmod_FileClose(file);
        return false;
    }
    
    // Read file content
    char* content = Dmod_Malloc(size + 1);
    if (!content) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "Out of memory");
        Dmod_FileClose(file);
        return false;
    }
    
    size_t read = Dmod_FileRead(content, 1, size, file);
    content[read] = '\0';
    Dmod_FileClose(file);
    
    // Parse content
    bool result = Dmod_Manifest_Parse(ctx, content);
    Dmod_Free(content);
    
    return result;
}

bool Dmod_Manifest_ParseUrl(Dmod_ManifestContext_t* ctx, const char* url) {
    if (!ctx || !url) return false;
    
    if (!ctx->download_func) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "No download function provided");
        return false;
    }
    
    char* content = NULL;
    size_t size = 0;
    
    if (!ctx->download_func(url, &content, &size, ctx->user_data)) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "Failed to download manifest from: %s", url);
        return false;
    }
    
    if (!content) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "Download returned NULL content");
        return false;
    }
    
    bool result = Dmod_Manifest_Parse(ctx, content);
    Dmod_Free(content);
    
    return result;
}

Dmod_ManifestNode_t* Dmod_Manifest_FindEntry(
    Dmod_ManifestContext_t* ctx,
    const char* name,
    const char* version,
    Dmod_ManifestNode_t* last_node,
    Dmod_ManifestEntry_t* out_entry
    ) 
{
    if (!ctx || !name || !out_entry) return NULL;

    bool has_version = version && version[0] != '\0';
    Dmod_ManifestNode_t* best_match = NULL;
    
    // Parse version constraint if provided
    Dmod_VersionConstraint_t constraint;
    bool has_constraint = false;
    if (has_version) {
        has_constraint = Dmod_Version_ParseConstraint(version, &constraint);
    }

    Dmod_ManifestNode_t* start_node = last_node ? last_node->next : ctx->entries;

    // Search for matching entry
    for (Dmod_ManifestNode_t* node = start_node; node; node = node->next) {
        if (strcmp(node->entry.name, name) != 0) continue;

        // If no version specified, take first match
        if (!has_version) {
            best_match = node;
            break;
        }

        bool node_has_version = node->entry.version[0] != '\0';
        if(!node_has_version)
        {
            best_match = node;
            break;
        }

        // If we have a constraint, check if node version satisfies it
        if (has_constraint) {
            Dmod_SemanticVersion_t node_version;
            if (Dmod_Version_Parse(node->entry.version, &node_version)) {
                if (Dmod_Version_Satisfies(&node_version, &constraint)) {
                    best_match = node;
                    break;
                }
            }
        } else {
            // No constraint parsed, try exact match
            if (strcmp(node->entry.version, version) == 0) {
                best_match = node;
                break;
            }
        }
    }

    if (best_match) {
        memcpy( out_entry, &best_match->entry, sizeof(Dmod_ManifestEntry_t) );
        return best_match;
    }
    
    Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "Module not found: %s%s%s",
                name, has_version ? "@" : "", has_version ? version : "");
    return NULL;
}

size_t Dmod_Manifest_GetEntryCount(Dmod_ManifestContext_t* ctx) {
    return ctx ? ctx->entry_count : 0;
}

bool Dmod_Manifest_GetEntry(
    Dmod_ManifestContext_t* ctx,
    size_t index,
    Dmod_ManifestEntry_t* out_entry
) {
    if (!ctx || !out_entry) return false;
    
    if (index >= ctx->entry_count) return false;
    
    Dmod_ManifestNode_t* node = ctx->entries;
    for (size_t i = 0; i < index && node; i++) {
        node = node->next;
    }
    
    if (node) {
        *out_entry = node->entry;
        return true;
    }
    
    return false;
}

const char* Dmod_Manifest_GetError(Dmod_ManifestContext_t* ctx) {
    if (!ctx || ctx->error[0] == '\0') return NULL;
    return ctx->error;
}

bool Dmod_Manifest_IsEntryCompatible(
    const Dmod_ManifestEntry_t* entry,
    const Dmod_SemanticVersion_t* current_dmod_version
) {
    if (!entry || !current_dmod_version) {
        return false;
    }
    
    // If entry has no DMOD version requirement, it's compatible
    if (!entry->has_dmod_version) {
        return true;
    }
    
    // Check major version compatibility
    return Dmod_Version_IsCompatible(&entry->dmod_version, current_dmod_version);
}
