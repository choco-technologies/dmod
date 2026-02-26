/**
 * @file dmod_resource.c
 * @brief DMOD Resource Parser Implementation
 */

#include "dmod_resource.h"
#include "dmod.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_ENTRIES 128
#define MAX_ERROR_LEN 256

/**
 * @brief Resource context structure
 */
struct Dmod_ResourceContext {
    char destination_path[DMOD_RESOURCE_MAX_PATH_LEN];
    char module_name[DMOD_RESOURCE_MAX_KEY_LEN];
    char repo_dir[DMOD_RESOURCE_MAX_PATH_LEN];
    char dmf_dir[DMOD_RESOURCE_MAX_PATH_LEN];
    char dmfc_dir[DMOD_RESOURCE_MAX_PATH_LEN];
    char build_dir[DMOD_RESOURCE_MAX_PATH_LEN];
    Dmod_ResourceEntry_t entries[MAX_ENTRIES];
    size_t entry_count;
    char error[MAX_ERROR_LEN];
};

/**
 * @brief Trim whitespace from both ends of a string
 */
static char* Trim(char* str) {
    if (!str) return NULL;
    
    // Trim leading whitespace
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return str;
    
    // Trim trailing whitespace
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    
    return str;
}

/**
 * @brief Substitute environment variables in a string
 * 
 * Replaces ${VAR_NAME} with the value of the environment variable VAR_NAME.
 * Special variables ${destination} and ${module} are replaced with context values.
 * 
 * @param ctx Resource context
 * @param input Input string with variables
 * @param output Output buffer
 * @param output_size Size of output buffer
 * @return true on success, false on buffer overflow
 */
static bool SubstituteVariables(Dmod_ResourceContext_t* ctx, const char* input, 
                                char* output, size_t output_size) {
    const char* src = input;
    char* dst = output;
    char* dst_end = output + output_size - 1;
    
    while (*src && dst < dst_end) {
        if (src[0] == '$' && src[1] == '{') {
            // Found variable start
            const char* var_start = src + 2;
            const char* var_end = strchr(var_start, '}');
            
            if (!var_end) {
                // Malformed variable, copy as-is
                *dst++ = *src++;
                continue;
            }
            
            // Extract variable name
            size_t var_len = var_end - var_start;
            char var_name[128];
            if (var_len >= sizeof(var_name)) {
                // Variable name too long
                *dst++ = *src++;
                continue;
            }
            
            strncpy(var_name, var_start, var_len);
            var_name[var_len] = '\0';
            
            // Substitute variable
            const char* value = NULL;
            
            if (strcmp(var_name, "destination") == 0) {
                value = ctx->destination_path;
            } else if (strcmp(var_name, "module") == 0) {
                value = ctx->module_name;
            } else if (strcmp(var_name, "repo_dir") == 0) {
                value = ctx->repo_dir[0] != '\0' ? ctx->repo_dir : NULL;
            } else if (strcmp(var_name, "dmf_dir") == 0) {
                value = ctx->dmf_dir[0] != '\0' ? ctx->dmf_dir : NULL;
            } else if (strcmp(var_name, "dmfc_dir") == 0) {
                value = ctx->dmfc_dir[0] != '\0' ? ctx->dmfc_dir : NULL;
            } else if (strcmp(var_name, "build_dir") == 0) {
                value = ctx->build_dir[0] != '\0' ? ctx->build_dir : NULL;
            } else {
                // Try environment variable
                value = Dmod_GetEnv(var_name);
            }
            
            if (value) {
                size_t value_len = strlen(value);
                if (dst + value_len >= dst_end) {
                    // Buffer overflow
                    return false;
                }
                strcpy(dst, value);
                dst += value_len;
            } else {
                // Variable not found, keep the original ${var} syntax
                size_t placeholder_len = (var_end - src) + 1;
                if (dst + placeholder_len >= dst_end) {
                    return false;
                }
                strncpy(dst, src, placeholder_len);
                dst += placeholder_len;
            }
            
            src = var_end + 1;
        } else {
            *dst++ = *src++;
        }
    }
    
    if (dst >= dst_end && *src) {
        // Buffer overflow
        return false;
    }
    
    *dst = '\0';
    return true;
}

/**
 * @brief Parse a single resource line
 * 
 * Format: key=source_path => destination_path [origin=path] ...
 * 
 * @param ctx Resource context
 * @param line Line to parse
 * @return true if line was parsed successfully, false otherwise
 */
static bool ParseLine(Dmod_ResourceContext_t* ctx, const char* line) {
    char line_copy[1024];
    strncpy(line_copy, line, sizeof(line_copy) - 1);
    line_copy[sizeof(line_copy) - 1] = '\0';
    
    // Split by '='
    char* eq_pos = strchr(line_copy, '=');
    if (!eq_pos) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), 
                     "Invalid line format: missing '=' (expected: key=source => dest)");
        return false;
    }
    
    *eq_pos = '\0';
    char* key = Trim(line_copy);
    char* rest = eq_pos + 1;
    
    // Split by '=>'
    char* arrow = strstr(rest, "=>");
    if (!arrow) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), 
                     "Invalid line format: missing '=>' (expected: key=source => dest)");
        return false;
    }
    
    *arrow = '\0';
    char* source = Trim(rest);
    char* dest_and_origins = Trim(arrow + 2);
    
    // Separate destination from optional [origin=...] directives
    char* origins_start = NULL;
    
    // Find the first '[' that is not inside a variable substitution
    char* bracket = strchr(dest_and_origins, '[');
    if (bracket) {
        origins_start = bracket;
        // Trim the destination part (everything before '[')
        char* dest_trim_end = bracket - 1;
        while (dest_trim_end > dest_and_origins && isspace((unsigned char)*dest_trim_end)) {
            dest_trim_end--;
        }
        dest_trim_end[1] = '\0';
    }
    
    char* dest = Trim(dest_and_origins);
    
    if (strlen(key) == 0 || strlen(source) == 0 || strlen(dest) == 0) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), 
                     "Invalid line format: empty key, source, or destination");
        return false;
    }
    
    if (ctx->entry_count >= MAX_ENTRIES) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), 
                     "Too many entries (max %d)", MAX_ENTRIES);
        return false;
    }
    
    // Create entry
    Dmod_ResourceEntry_t* entry = &ctx->entries[ctx->entry_count];
    
    // Copy key
    strncpy(entry->key, key, sizeof(entry->key) - 1);
    entry->key[sizeof(entry->key) - 1] = '\0';
    
    // Substitute variables in source path
    if (!SubstituteVariables(ctx, source, entry->source, sizeof(entry->source))) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error),
                     "Source path too long after variable substitution");
        return false;
    }
    
    // Substitute variables in destination
    if (!SubstituteVariables(ctx, dest, entry->destination, sizeof(entry->destination))) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), 
                     "Destination path too long after variable substitution");
        return false;
    }
    
    // Check if this is a dmf or dmfc resource
    entry->is_dmf_dmfc = (strcmp(key, "dmf") == 0 || strcmp(key, "dmfc") == 0);
    
    // Parse [origin=...] directives
    entry->origin_count = 0;
    if (origins_start) {
        char* p = origins_start;
        while (*p && entry->origin_count < DMOD_RESOURCE_MAX_ORIGINS) {
            // Skip whitespace
            while (isspace((unsigned char)*p)) p++;
            if (*p != '[') break;
            p++; // skip '['
            
            // Check for "origin="
            if (strncmp(p, "origin=", 7) != 0) {
                // Unknown directive, skip to closing ']'
                char* close = strchr(p, ']');
                if (!close) break;
                p = close + 1;
                continue;
            }
            p += 7; // skip "origin="
            
            // Find closing ']'
            char* close = strchr(p, ']');
            if (!close) {
                Dmod_SnPrintf(ctx->error, sizeof(ctx->error),
                             "Invalid [origin] directive: missing ']'");
                return false;
            }
            
            // Extract origin path
            size_t origin_len = (size_t)(close - p);
            char origin_raw[DMOD_RESOURCE_MAX_PATH_LEN];
            if (origin_len >= sizeof(origin_raw)) {
                Dmod_SnPrintf(ctx->error, sizeof(ctx->error),
                             "Origin path too long");
                return false;
            }
            strncpy(origin_raw, p, origin_len);
            origin_raw[origin_len] = '\0';
            
            char* trimmed_origin = Trim(origin_raw);
            if (strlen(trimmed_origin) == 0) {
                Dmod_SnPrintf(ctx->error, sizeof(ctx->error),
                             "Empty [origin] path");
                return false;
            }
            
            // Substitute variables in origin path
            if (!SubstituteVariables(ctx, trimmed_origin,
                                     entry->origins[entry->origin_count],
                                     sizeof(entry->origins[entry->origin_count]))) {
                Dmod_SnPrintf(ctx->error, sizeof(ctx->error),
                             "Origin path too long after variable substitution");
                return false;
            }
            
            entry->origin_count++;
            p = close + 1;
        }
    }
    
    ctx->entry_count++;
    return true;
}

Dmod_ResourceContext_t* Dmod_Resource_Init(const char* destination_path, 
                                            const char* module_name,
                                            const char* repo_dir,
                                            const char* dmf_dir,
                                            const char* dmfc_dir,
                                            const char* build_dir) {
    if (!destination_path || !module_name) {
        return NULL;
    }
    
    Dmod_ResourceContext_t* ctx = (Dmod_ResourceContext_t*)Dmod_Malloc(sizeof(Dmod_ResourceContext_t));
    if (!ctx) {
        return NULL;
    }
    
    memset(ctx, 0, sizeof(Dmod_ResourceContext_t));
    
    strncpy(ctx->destination_path, destination_path, sizeof(ctx->destination_path) - 1);
    ctx->destination_path[sizeof(ctx->destination_path) - 1] = '\0';
    
    strncpy(ctx->module_name, module_name, sizeof(ctx->module_name) - 1);
    ctx->module_name[sizeof(ctx->module_name) - 1] = '\0';
    
    if (repo_dir) {
        strncpy(ctx->repo_dir, repo_dir, sizeof(ctx->repo_dir) - 1);
        ctx->repo_dir[sizeof(ctx->repo_dir) - 1] = '\0';
    }
    
    if (dmf_dir) {
        strncpy(ctx->dmf_dir, dmf_dir, sizeof(ctx->dmf_dir) - 1);
        ctx->dmf_dir[sizeof(ctx->dmf_dir) - 1] = '\0';
    }
    
    if (dmfc_dir) {
        strncpy(ctx->dmfc_dir, dmfc_dir, sizeof(ctx->dmfc_dir) - 1);
        ctx->dmfc_dir[sizeof(ctx->dmfc_dir) - 1] = '\0';
    }
    
    if (build_dir) {
        strncpy(ctx->build_dir, build_dir, sizeof(ctx->build_dir) - 1);
        ctx->build_dir[sizeof(ctx->build_dir) - 1] = '\0';
    }
    
    return ctx;
}

void Dmod_Resource_Free(Dmod_ResourceContext_t* ctx) {
    if (ctx) {
        Dmod_Free(ctx);
    }
}

bool Dmod_Resource_Parse(Dmod_ResourceContext_t* ctx, const char* resource_content) {
    if (!ctx || !resource_content) {
        return false;
    }
    
    // Reset entries
    ctx->entry_count = 0;
    ctx->error[0] = '\0';
    
    // Parse line by line
    char* content_copy = Dmod_StrDup(resource_content);
    if (!content_copy) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "Out of memory");
        return false;
    }
    
    char* line = strtok(content_copy, "\n\r");
    int line_number = 0;
    
    while (line) {
        line_number++;
        char* trimmed = Trim(line);
        
        // Skip empty lines and comments
        if (trimmed[0] != '\0' && trimmed[0] != '#') {
            if (!ParseLine(ctx, trimmed)) {
                char temp_error[MAX_ERROR_LEN];
                strncpy(temp_error, ctx->error, sizeof(temp_error) - 1);
                temp_error[sizeof(temp_error) - 1] = '\0';
                Dmod_SnPrintf(ctx->error, sizeof(ctx->error), 
                             "Line %d: %s", line_number, temp_error);
                Dmod_Free(content_copy);
                return false;
            }
        }
        
        line = strtok(NULL, "\n\r");
    }
    
    Dmod_Free(content_copy);
    return true;
}

bool Dmod_Resource_ParseFile(Dmod_ResourceContext_t* ctx, const char* file_path) {
    if (!ctx || !file_path) {
        return false;
    }
    
    // Read file
    void* file = Dmod_FileOpen(file_path, "rb");
    if (!file) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), 
                     "Cannot open file: %s", file_path);
        return false;
    }
    
    // Get file size
    Dmod_FileSeek(file, 0, DMOD_SEEK_END);
    long file_size = Dmod_FileTell(file);
    Dmod_FileSeek(file, 0, DMOD_SEEK_SET);
    
    if (file_size < 0 || file_size > 1024 * 1024) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), 
                     "Invalid file size: %ld", file_size);
        Dmod_FileClose(file);
        return false;
    }
    
    // Read content
    char* content = (char*)Dmod_Malloc(file_size + 1);
    if (!content) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "Out of memory");
        Dmod_FileClose(file);
        return false;
    }
    
    size_t read_size = Dmod_FileRead(content, 1, file_size, file);
    if (read_size > (size_t)file_size) {
        Dmod_SnPrintf(ctx->error, sizeof(ctx->error), "Read size exceeds file size");
        Dmod_Free(content);
        Dmod_FileClose(file);
        return false;
    }
    content[read_size] = '\0';
    
    Dmod_FileClose(file);
    
    // Parse content
    bool result = Dmod_Resource_Parse(ctx, content);
    Dmod_Free(content);
    
    return result;
}

size_t Dmod_Resource_GetEntryCount(Dmod_ResourceContext_t* ctx) {
    if (!ctx) {
        return 0;
    }
    return ctx->entry_count;
}

bool Dmod_Resource_GetEntry(Dmod_ResourceContext_t* ctx, size_t index, 
                             Dmod_ResourceEntry_t* out_entry) {
    if (!ctx || !out_entry || index >= ctx->entry_count) {
        return false;
    }
    
    *out_entry = ctx->entries[index];
    return true;
}

const char* Dmod_Resource_GetError(Dmod_ResourceContext_t* ctx) {
    if (!ctx || ctx->error[0] == '\0') {
        return NULL;
    }
    return ctx->error;
}
