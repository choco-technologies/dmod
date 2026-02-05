/**
 * @file dmod_dependencies.c
 * @brief Implementation of DMOD Dependencies Parser Library
 */

#include "dmod_dependencies.h"
#include "dmod.h"
#include "dmod_version.h"
#include <string.h>
#include <ctype.h>

/**
 * @brief Internal entry node for linked list
 */
typedef struct DependencyNode {
    Dmod_DependencyEntry_t entry;
    struct DependencyNode* next;
} DependencyNode_t;

/**
 * @brief Dependencies context structure
 */
struct Dmod_DependenciesContext {
    char* default_manifest;           /**< Default manifest URL */
    char* current_manifest;           /**< Current manifest URL (changes with from:) */
    DependencyNode_t* head;           /**< Head of dependencies list */
    DependencyNode_t* tail;           /**< Tail of dependencies list */
    size_t count;                     /**< Number of entries */
    char* error_message;              /**< Last error message */
    Dmod_DepDownloadFunc_t download_func; /**< Download function */
    void* user_data;                  /**< User data for download function */
};

/**
 * @brief Set error message
 */
static void SetError(Dmod_DependenciesContext_t* ctx, const char* format, ...) {
    if (!ctx) return;
    
    if (ctx->error_message) {
        Dmod_Free(ctx->error_message);
        ctx->error_message = NULL;
    }
    
    // Allocate buffer for error message
    char buffer[512];
    va_list args;
    va_start(args, format);
    Dmod_VSnPrintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    size_t len = strlen(buffer);
    ctx->error_message = (char*)Dmod_Malloc(len + 1);
    if (ctx->error_message) {
        strcpy(ctx->error_message, buffer);
    }
}

/**
 * @brief Add a dependency entry to the list
 */
static bool AddEntry(Dmod_DependenciesContext_t* ctx, const char* name, 
                     const char* version, const char* manifest, const char* config) {
    DependencyNode_t* node = (DependencyNode_t*)Dmod_Malloc(sizeof(DependencyNode_t));
    if (!node) {
        SetError(ctx, "Out of memory");
        return false;
    }
    
    memset(node, 0, sizeof(DependencyNode_t));
    
    // Copy name
    strncpy(node->entry.name, name, DMOD_DEPENDENCIES_MAX_NAME_LEN - 1);
    node->entry.name[DMOD_DEPENDENCIES_MAX_NAME_LEN - 1] = '\0';
    
    // Copy version if provided and parse constraint
    if (version) {
        strncpy(node->entry.version, version, DMOD_DEPENDENCIES_MAX_VERSION_LEN - 1);
        node->entry.version[DMOD_DEPENDENCIES_MAX_VERSION_LEN - 1] = '\0';
        
        // Try to parse version constraint
        node->entry.has_constraint = Dmod_Version_ParseConstraint(version, &node->entry.constraint);
    }
    
    // Copy manifest
    strncpy(node->entry.manifest, manifest, DMOD_DEPENDENCIES_MAX_URL_LEN - 1);
    node->entry.manifest[DMOD_DEPENDENCIES_MAX_URL_LEN - 1] = '\0';
    
    // Copy config if provided
    if (config) {
        strncpy(node->entry.config, config, DMOD_DEPENDENCIES_MAX_CONFIG_LEN - 1);
        node->entry.config[DMOD_DEPENDENCIES_MAX_CONFIG_LEN - 1] = '\0';
    }
    
    // Add to list
    if (ctx->tail) {
        ctx->tail->next = node;
        ctx->tail = node;
    } else {
        ctx->head = ctx->tail = node;
    }
    
    ctx->count++;
    return true;
}

/**
 * @brief Trim whitespace from both ends of a string
 */
static char* TrimWhitespace(char* str) {
    char* end;
    
    // Trim leading space
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return str;
    
    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    // Write new null terminator
    end[1] = '\0';
    
    return str;
}

/**
 * @brief Parse a single line from the dependencies file
 */
static bool ParseLine(Dmod_DependenciesContext_t* ctx, char* line) {
    // Trim whitespace
    line = TrimWhitespace(line);
    
    // Skip empty lines
    if (line[0] == '\0') {
        return true;
    }
    
    // Skip comments
    if (line[0] == '#') {
        return true;
    }
    
    // Handle $include directive
    if (strncmp(line, "$include", 8) == 0) {
        char* url = line + 8;
        url = TrimWhitespace(url);
        
        if (url[0] == '\0') {
            SetError(ctx, "$include directive requires a URL");
            return false;
        }
        
        // Download and parse the included file
        char* buffer = NULL;
        size_t size = 0;
        
        if (!ctx->download_func(url, &buffer, &size, ctx->user_data)) {
            SetError(ctx, "Failed to download included file: %s", url);
            return false;
        }
        
        bool result = Dmod_Dependencies_Parse(ctx, buffer);
        Dmod_Free(buffer);
        
        return result;
    }
    
    // Handle $from directive
    if (strncmp(line, "$from", 5) == 0) {
        char* manifest_url = line + 5;
        manifest_url = TrimWhitespace(manifest_url);
        
        if (manifest_url[0] == '\0') {
            SetError(ctx, "$from directive requires a manifest URL");
            return false;
        }
        
        // Update current manifest
        if (ctx->current_manifest) {
            Dmod_Free(ctx->current_manifest);
        }
        
        size_t len = strlen(manifest_url);
        ctx->current_manifest = (char*)Dmod_Malloc(len + 1);
        if (!ctx->current_manifest) {
            SetError(ctx, "Out of memory");
            return false;
        }
        strcpy(ctx->current_manifest, manifest_url);
        
        return true;
    }
    
    // Parse module entry: module[@version] [config_path] [$from manifest_url]
    // First check if there's an inline $from directive
    char* from_pos = strstr(line, "$from");
    char* manifest_for_entry = NULL;
    char line_copy[512];
    
    if (from_pos) {
        // Extract the manifest URL from inline $from
        char* manifest_url = from_pos + 5;  // Skip "$from"
        manifest_url = TrimWhitespace(manifest_url);
        
        if (manifest_url[0] == '\0') {
            SetError(ctx, "Inline $from directive requires a manifest URL");
            return false;
        }
        
        // Allocate memory for this specific manifest
        size_t len = strlen(manifest_url);
        manifest_for_entry = (char*)Dmod_Malloc(len + 1);
        if (!manifest_for_entry) {
            SetError(ctx, "Out of memory");
            return false;
        }
        strcpy(manifest_for_entry, manifest_url);
        
        // Copy line without the $from part
        size_t module_part_len = from_pos - line;
        if (module_part_len >= sizeof(line_copy)) {
            Dmod_Free(manifest_for_entry);
            SetError(ctx, "Module entry too long");
            return false;
        }
        strncpy(line_copy, line, module_part_len);
        line_copy[module_part_len] = '\0';
        line = line_copy;
        line = TrimWhitespace(line);
    }
    
    // Parse: module[@version] [config_path]
    char* at_sign = strchr(line, '@');
    char module_name[DMOD_DEPENDENCIES_MAX_NAME_LEN];
    char module_version[DMOD_DEPENDENCIES_MAX_VERSION_LEN] = {0};
    char module_config[DMOD_DEPENDENCIES_MAX_CONFIG_LEN] = {0};
    char* rest_of_line = NULL;
    
    if (at_sign) {
        // Has version
        size_t name_len = at_sign - line;
        if (name_len >= sizeof(module_name)) {
            SetError(ctx, "Module name too long: %s", line);
            if (manifest_for_entry) Dmod_Free(manifest_for_entry);
            return false;
        }
        
        strncpy(module_name, line, name_len);
        module_name[name_len] = '\0';
        
        // Trim the name
        char* trimmed_name = TrimWhitespace(module_name);
        memmove(module_name, trimmed_name, strlen(trimmed_name) + 1);
        
        // Get version and possibly config
        rest_of_line = at_sign + 1;
    } else {
        // No version, might have config
        rest_of_line = line;
    }
    
    // Parse version and config from rest_of_line
    if (rest_of_line && at_sign) {
        // We have version, parse it and check for config
        char* space = strchr(rest_of_line, ' ');
        if (space) {
            // We have both version and config
            size_t version_len = space - rest_of_line;
            if (version_len >= sizeof(module_version)) {
                SetError(ctx, "Version string too long");
                if (manifest_for_entry) Dmod_Free(manifest_for_entry);
                return false;
            }
            strncpy(module_version, rest_of_line, version_len);
            module_version[version_len] = '\0';
            
            // Get config path
            char* config = TrimWhitespace(space + 1);
            if (config[0] != '\0') {
                strncpy(module_config, config, sizeof(module_config) - 1);
                module_config[sizeof(module_config) - 1] = '\0';
            }
        } else {
            // Only version
            char* version = TrimWhitespace(rest_of_line);
            strncpy(module_version, version, sizeof(module_version) - 1);
            module_version[sizeof(module_version) - 1] = '\0';
        }
    } else if (rest_of_line) {
        // No @ sign, parse module name and possibly config
        char* space = strchr(rest_of_line, ' ');
        if (space) {
            // We have both module name and config
            size_t name_len = space - rest_of_line;
            if (name_len >= sizeof(module_name)) {
                SetError(ctx, "Module name too long: %s", line);
                if (manifest_for_entry) Dmod_Free(manifest_for_entry);
                return false;
            }
            strncpy(module_name, rest_of_line, name_len);
            module_name[name_len] = '\0';
            
            // Trim the name
            char* trimmed_name = TrimWhitespace(module_name);
            memmove(module_name, trimmed_name, strlen(trimmed_name) + 1);
            
            // Get config path
            char* config = TrimWhitespace(space + 1);
            if (config[0] != '\0') {
                strncpy(module_config, config, sizeof(module_config) - 1);
                module_config[sizeof(module_config) - 1] = '\0';
            }
        } else {
            // Only module name
            strncpy(module_name, rest_of_line, sizeof(module_name) - 1);
            module_name[sizeof(module_name) - 1] = '\0';
            
            // Trim the name
            char* trimmed_name = TrimWhitespace(module_name);
            memmove(module_name, trimmed_name, strlen(trimmed_name) + 1);
        }
    }
    
    // Validate module name
    if (module_name[0] == '\0') {
        SetError(ctx, "Empty module name");
        if (manifest_for_entry) Dmod_Free(manifest_for_entry);
        return false;
    }
    
    // Use inline manifest if provided, otherwise use current manifest
    const char* manifest = manifest_for_entry ? manifest_for_entry : 
                          (ctx->current_manifest ? ctx->current_manifest : ctx->default_manifest);
    bool result = AddEntry(ctx, module_name, module_version[0] ? module_version : NULL, manifest, 
                          module_config[0] ? module_config : NULL);
    
    // Free the inline manifest memory after adding the entry
    if (manifest_for_entry) {
        Dmod_Free(manifest_for_entry);
    }
    
    return result;
}

/**
 * @brief Initialize a new dependencies context
 */
Dmod_DependenciesContext_t* Dmod_Dependencies_Init(
    const char* default_manifest,
    Dmod_DepDownloadFunc_t download_func,
    void* user_data
) {
    Dmod_DependenciesContext_t* ctx = (Dmod_DependenciesContext_t*)Dmod_Malloc(sizeof(Dmod_DependenciesContext_t));
    if (!ctx) {
        return NULL;
    }
    
    memset(ctx, 0, sizeof(Dmod_DependenciesContext_t));
    
    // Set default manifest
    if (default_manifest) {
        size_t len = strlen(default_manifest);
        ctx->default_manifest = (char*)Dmod_Malloc(len + 1);
        if (!ctx->default_manifest) {
            Dmod_Free(ctx);
            return NULL;
        }
        strcpy(ctx->default_manifest, default_manifest);
    }
    
    ctx->download_func = download_func;
    ctx->user_data = user_data;
    
    return ctx;
}

/**
 * @brief Free a dependencies context
 */
void Dmod_Dependencies_Free(Dmod_DependenciesContext_t* ctx) {
    if (!ctx) return;
    
    // Free entries list
    DependencyNode_t* node = ctx->head;
    while (node) {
        DependencyNode_t* next = node->next;
        Dmod_Free(node);
        node = next;
    }
    
    // Free strings
    if (ctx->default_manifest) {
        Dmod_Free(ctx->default_manifest);
    }
    if (ctx->current_manifest) {
        Dmod_Free(ctx->current_manifest);
    }
    if (ctx->error_message) {
        Dmod_Free(ctx->error_message);
    }
    
    Dmod_Free(ctx);
}

/**
 * @brief Parse dependencies from a string
 */
bool Dmod_Dependencies_Parse(Dmod_DependenciesContext_t* ctx, const char* dependencies_content) {
    if (!ctx) return false;
    if (!dependencies_content) {
        SetError(ctx, "Dependencies content is NULL");
        return false;
    }
    
    // Make a copy of the content since we'll modify it
    size_t len = strlen(dependencies_content);
    char* content_copy = (char*)Dmod_Malloc(len + 1);
    if (!content_copy) {
        SetError(ctx, "Out of memory");
        return false;
    }
    strcpy(content_copy, dependencies_content);
    
    // Parse line by line
    char* line = content_copy;
    char* next_line = NULL;
    bool success = true;
    
    while (line && *line) {
        // Find end of line
        next_line = strchr(line, '\n');
        if (next_line) {
            *next_line = '\0';
            next_line++;
        }
        
        // Remove carriage return if present
        char* cr = strchr(line, '\r');
        if (cr) *cr = '\0';
        
        // Parse the line
        if (!ParseLine(ctx, line)) {
            success = false;
            break;
        }
        
        line = next_line;
    }
    
    Dmod_Free(content_copy);
    return success;
}

/**
 * @brief Parse dependencies from a file
 */
bool Dmod_Dependencies_ParseFile(Dmod_DependenciesContext_t* ctx, const char* file_path) {
    if (!ctx) return false;
    if (!file_path) {
        SetError(ctx, "File path is NULL");
        return false;
    }
    
    // Read file content
    void* file = Dmod_FileOpen(file_path, "rb");
    if (!file) {
        SetError(ctx, "Cannot open file: %s", file_path);
        return false;
    }
    
    // Get file size
    Dmod_FileSeek(file, 0, DMOD_SEEK_END);
    long size = Dmod_FileTell(file);
    Dmod_FileSeek(file, 0, DMOD_SEEK_SET);
    
    if (size < 0) {
        SetError(ctx, "Cannot get file size: %s", file_path);
        Dmod_FileClose(file);
        return false;
    }
    
    // Allocate buffer
    char* buffer = (char*)Dmod_Malloc(size + 1);
    if (!buffer) {
        SetError(ctx, "Out of memory");
        Dmod_FileClose(file);
        return false;
    }
    
    // Read content
    size_t read = Dmod_FileRead(buffer, 1, size, file);
    Dmod_FileClose(file);
    
    if (read != (size_t)size) {
        SetError(ctx, "Failed to read file: %s", file_path);
        Dmod_Free(buffer);
        return false;
    }
    
    buffer[size] = '\0';
    
    // Parse content
    bool result = Dmod_Dependencies_Parse(ctx, buffer);
    Dmod_Free(buffer);
    
    return result;
}

/**
 * @brief Parse dependencies from a URL
 */
bool Dmod_Dependencies_ParseUrl(Dmod_DependenciesContext_t* ctx, const char* url) {
    if (!ctx) return false;
    if (!url) {
        SetError(ctx, "URL is NULL");
        return false;
    }
    
    if (!ctx->download_func) {
        SetError(ctx, "No download function provided");
        return false;
    }
    
    // Download content
    char* buffer = NULL;
    size_t size = 0;
    
    if (!ctx->download_func(url, &buffer, &size, ctx->user_data)) {
        SetError(ctx, "Failed to download from URL: %s", url);
        return false;
    }
    
    // Parse content
    bool result = Dmod_Dependencies_Parse(ctx, buffer);
    Dmod_Free(buffer);
    
    return result;
}

/**
 * @brief Get the number of entries
 */
size_t Dmod_Dependencies_GetEntryCount(Dmod_DependenciesContext_t* ctx) {
    if (!ctx) return 0;
    return ctx->count;
}

/**
 * @brief Get an entry by index
 */
bool Dmod_Dependencies_GetEntry(
    Dmod_DependenciesContext_t* ctx,
    size_t index,
    Dmod_DependencyEntry_t* out_entry
) {
    if (!ctx || !out_entry) return false;
    
    if (index >= ctx->count) {
        SetError(ctx, "Index out of bounds");
        return false;
    }
    
    // Find the node at the given index
    DependencyNode_t* node = ctx->head;
    for (size_t i = 0; i < index && node; i++) {
        node = node->next;
    }
    
    if (!node) {
        SetError(ctx, "Internal error: node not found");
        return false;
    }
    
    // Copy entry
    *out_entry = node->entry;
    return true;
}

/**
 * @brief Get the last error message
 */
const char* Dmod_Dependencies_GetError(Dmod_DependenciesContext_t* ctx) {
    if (!ctx) return NULL;
    return ctx->error_message;
}
