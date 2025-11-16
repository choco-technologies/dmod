/**
 * @file dmod_manifest.c
 * @brief DMOD Manifest Parser Library Implementation
 */

#include "dmod_manifest.h"
#include "dmod.h"
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
 * @brief Manifest context structure
 */
struct Dmod_ManifestContext {
    char* tools_name;                    /**< Tools name for substitution */
    char* arch_name;                     /**< Architecture name for substitution */
    Dmod_DownloadFunc_t download_func;   /**< Download function */
    void* user_data;                     /**< User data for download function */
    Dmod_ManifestNode_t* entries;        /**< Linked list of entries */
    size_t entry_count;                  /**< Number of entries */
    char error[256];                     /**< Last error message */
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
 * Replaces <tools_name> and <arch_name> with actual values
 * 
 * @param ctx Manifest context
 * @param input Input string
 * @param output Output buffer
 * @param output_size Size of output buffer
 * @return true if substitution succeeded, false otherwise
 */
static bool SubstituteVariables(Dmod_ManifestContext_t* ctx, const char* input, char* output, size_t output_size) {
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
    
    // Check for $include directive
    if (strncmp(line, "$include", 8) == 0) {
        char* url_start = line + 8;
        url_start = TrimWhitespace(url_start);
        
        char url[DMOD_MANIFEST_MAX_URL_LEN];
        if (!SubstituteVariables(ctx, url_start, url, sizeof(url))) {
            Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "URL too long in $include: %s", url_start);
            return false;
        }
        
        // Parse the included manifest
        if (!Dmod_Manifest_ParseUrl(ctx, url)) {
            return false;
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
    
    // Create new entry
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
    
    if (!SubstituteVariables(ctx, url_raw, node->entry.url, sizeof(node->entry.url))) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "URL too long after substitution: %s", url_raw);
        Dmod_Free(node);
        return false;
    }
    
    // Add to linked list
    node->next = ctx->entries;
    ctx->entries = node;
    ctx->entry_count++;
    
    return true;
}

Dmod_ManifestContext_t* Dmod_Manifest_Init(
    const char* tools_name,
    Dmod_DownloadFunc_t download_func,
    void* user_data
) {
    Dmod_ManifestContext_t* ctx = Dmod_Malloc(sizeof(Dmod_ManifestContext_t));
    if (!ctx) return NULL;
    
    memset(ctx, 0, sizeof(Dmod_ManifestContext_t));
    
    if (tools_name) {
        size_t len = strlen(tools_name);
        ctx->tools_name = Dmod_Malloc(len + 1);
        if (ctx->tools_name) {
            strcpy(ctx->tools_name, tools_name);
        }
        ctx->arch_name = ConvertToArchName(tools_name);
        if (!ctx->tools_name || !ctx->arch_name) {
            Dmod_Manifest_Free(ctx);
            return NULL;
        }
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
    
    // Free strings
    if (ctx->tools_name) Dmod_Free(ctx->tools_name);
    if (ctx->arch_name) Dmod_Free(ctx->arch_name);
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

bool Dmod_Manifest_FindEntry(
    Dmod_ManifestContext_t* ctx,
    const char* name,
    const char* version,
    Dmod_ManifestEntry_t* out_entry
) {
    if (!ctx || !name || !out_entry) return false;
    
    bool has_version = version && version[0] != '\0';
    Dmod_ManifestNode_t* best_match = NULL;
    
    // Search for matching entry
    for (Dmod_ManifestNode_t* node = ctx->entries; node; node = node->next) {
        if (strcmp(node->entry.name, name) != 0) continue;
        
        // If no version specified, take first match
        if (!has_version) {
            best_match = node;
            break;
        }
        
        // Check version match
        if (strcmp(node->entry.version, version) == 0) {
            best_match = node;
            break;
        }
        
        // If no exact match yet, keep this as potential match
        if (!best_match) {
            best_match = node;
        }
    }
    
    if (best_match) {
        *out_entry = best_match->entry;
        return true;
    }
    
    Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "Module not found: %s%s%s",
             name, has_version ? "@" : "", has_version ? version : "");
    return false;
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
