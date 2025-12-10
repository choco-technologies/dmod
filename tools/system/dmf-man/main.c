/**
 * @file main.c
 * @brief dmf-man - DMOD Documentation Viewer
 * 
 * This tool displays documentation for DMOD modules in Markdown format
 * with basic VT100 terminal formatting support.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include "dmod.h"

// Default paths
#define DEFAULT_DMF_DIR "./dmf"

// Environment variables
#define ENV_DOC_DIR "DMOD_DOC_DIR"
#define ENV_DMF_DIR "DMOD_DMF_DIR"

// VT100 escape codes
#define VT100_RESET "\033[0m"
#define VT100_BOLD "\033[1m"
#define VT100_DIM "\033[2m"
#define VT100_ITALIC "\033[3m"
#define VT100_UNDERLINE "\033[4m"
#define VT100_REVERSE "\033[7m"

// Colors
#define VT100_BLACK "\033[30m"
#define VT100_RED "\033[31m"
#define VT100_GREEN "\033[32m"
#define VT100_YELLOW "\033[33m"
#define VT100_BLUE "\033[34m"
#define VT100_MAGENTA "\033[35m"
#define VT100_CYAN "\033[36m"
#define VT100_WHITE "\033[37m"

/**
 * @brief Print usage message
 */
static void PrintUsage(const char* app_name) {
    printf("Usage: %s [OPTIONS] <module_name>\n", app_name);
    printf("\n");
    printf("Options:\n");
    printf("  -d, --doc-dir <path>  Path to documentation directory\n");
    printf("  -h, --help            Show this help message\n");
    printf("  -v, --version         Show version information\n");
    printf("\n");
    printf("Environment Variables:\n");
    printf("  %s     Documentation directory (checked first)\n", ENV_DOC_DIR);
    printf("  %s        DMF directory (used as fallback: <dir>/<module>/docs)\n", ENV_DMF_DIR);
    printf("\n");
    printf("Examples:\n");
    printf("  %s mymodule                    # Search in default locations\n", app_name);
    printf("  %s -d /path/to/docs mymodule   # Use custom documentation directory\n", app_name);
}

/**
 * @brief Print help message
 */
static void PrintHelp(const char* app_name) {
    printf("-- dmf-man - DMOD Documentation Viewer ver. " DMOD_VERSION_STRING " --\n\n");
    printf("This tool displays documentation for DMOD modules.\n");
    printf("Documentation is rendered from Markdown with basic VT100 formatting.\n\n");
    PrintUsage(app_name);
}

/**
 * @brief Check if a file exists
 */
static bool FileExists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

/**
 * @brief Check if a directory exists
 */
static bool DirExists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

/**
 * @brief Find documentation file for a module
 * 
 * Search order:
 * 1. DMOD_DOC_DIR/<module>.md
 * 2. DMOD_DOC_DIR/<module>/README.md
 * 3. DMOD_DMF_DIR/<module>/docs/<module>.md
 * 4. DMOD_DMF_DIR/<module>/docs/README.md
 * 5. Custom doc_dir if provided
 */
static char* FindDocumentation(const char* module_name, const char* custom_doc_dir) {
    char path[1024];
    
    // Try custom doc directory first
    if (custom_doc_dir) {
        // Try <custom_doc_dir>/<module>.md
        snprintf(path, sizeof(path), "%s/%s.md", custom_doc_dir, module_name);
        if (FileExists(path)) {
            return strdup(path);
        }
        
        // Try <custom_doc_dir>/<module>/README.md
        snprintf(path, sizeof(path), "%s/%s/README.md", custom_doc_dir, module_name);
        if (FileExists(path)) {
            return strdup(path);
        }
        
        // Try <custom_doc_dir>/README.md
        snprintf(path, sizeof(path), "%s/README.md", custom_doc_dir);
        if (FileExists(path)) {
            return strdup(path);
        }
    }
    
    // Try DMOD_DOC_DIR
    const char* doc_dir = Dmod_GetEnv(ENV_DOC_DIR);
    if (doc_dir) {
        // Try <doc_dir>/<module>.md
        snprintf(path, sizeof(path), "%s/%s.md", doc_dir, module_name);
        if (FileExists(path)) {
            return strdup(path);
        }
        
        // Try <doc_dir>/<module>/README.md
        snprintf(path, sizeof(path), "%s/%s/README.md", doc_dir, module_name);
        if (FileExists(path)) {
            return strdup(path);
        }
        
        // Try <doc_dir>/README.md
        snprintf(path, sizeof(path), "%s/README.md", doc_dir);
        if (FileExists(path)) {
            return strdup(path);
        }
    }
    
    // Try DMOD_DMF_DIR
    const char* dmf_dir = Dmod_GetEnv(ENV_DMF_DIR);
    if (!dmf_dir) {
        dmf_dir = DEFAULT_DMF_DIR;
    }
    
    // Try <dmf_dir>/<module>/docs/<module>.md
    snprintf(path, sizeof(path), "%s/%s/docs/%s.md", dmf_dir, module_name, module_name);
    if (FileExists(path)) {
        return strdup(path);
    }
    
    // Try <dmf_dir>/<module>/docs/README.md
    snprintf(path, sizeof(path), "%s/%s/docs/README.md", dmf_dir, module_name);
    if (FileExists(path)) {
        return strdup(path);
    }
    
    // Try <dmf_dir>/<module>/README.md
    snprintf(path, sizeof(path), "%s/%s/README.md", dmf_dir, module_name);
    if (FileExists(path)) {
        return strdup(path);
    }
    
    return NULL;
}

/**
 * @brief Check if line starts with string
 */
static bool StartsWith(const char* line, const char* prefix) {
    return strncmp(line, prefix, strlen(prefix)) == 0;
}

/**
 * @brief Count leading characters
 */
static int CountLeading(const char* line, char c) {
    int count = 0;
    while (line[count] == c) {
        count++;
    }
    return count;
}

/**
 * @brief Trim trailing whitespace
 */
static void TrimTrailing(char* line) {
    int len = strlen(line);
    while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r' || line[len-1] == ' ' || line[len-1] == '\t')) {
        line[--len] = '\0';
    }
}

/**
 * @brief Process inline markdown formatting
 */
static void ProcessInlineFormatting(const char* line, char* output, size_t output_size) {
    size_t i = 0, j = 0;
    bool in_bold = false;
    bool in_italic = false;
    bool in_code = false;
    
    while (line[i] && j < output_size - 50) {
        // Handle inline code `code`
        if (line[i] == '`' && !in_code) {
            in_code = true;
            j += snprintf(output + j, output_size - j, "%s%s", VT100_CYAN, VT100_REVERSE);
            i++;
            continue;
        } else if (line[i] == '`' && in_code) {
            in_code = false;
            j += snprintf(output + j, output_size - j, "%s", VT100_RESET);
            i++;
            continue;
        }
        
        // Handle bold **text** or __text__
        if (!in_code && (line[i] == '*' && line[i+1] == '*') || (line[i] == '_' && line[i+1] == '_')) {
            if (!in_bold) {
                in_bold = true;
                j += snprintf(output + j, output_size - j, "%s", VT100_BOLD);
            } else {
                in_bold = false;
                j += snprintf(output + j, output_size - j, "%s", VT100_RESET);
            }
            i += 2;
            continue;
        }
        
        // Handle italic *text* or _text_ (but not ** or __)
        if (!in_code && (line[i] == '*' && line[i+1] != '*') || (line[i] == '_' && line[i+1] != '_')) {
            // Check if it's not at word boundary for underscore
            if (line[i] == '_' && i > 0 && isalnum(line[i-1])) {
                output[j++] = line[i++];
                continue;
            }
            
            if (!in_italic) {
                in_italic = true;
                j += snprintf(output + j, output_size - j, "%s", VT100_ITALIC);
            } else {
                in_italic = false;
                j += snprintf(output + j, output_size - j, "%s", VT100_RESET);
            }
            i++;
            continue;
        }
        
        // Handle links [text](url) - just show the text underlined
        if (!in_code && line[i] == '[') {
            const char* end_bracket = strchr(line + i, ']');
            if (end_bracket && end_bracket[1] == '(') {
                const char* end_paren = strchr(end_bracket + 1, ')');
                if (end_paren) {
                    // Extract link text
                    size_t text_len = end_bracket - (line + i) - 1;
                    j += snprintf(output + j, output_size - j, "%s", VT100_UNDERLINE);
                    for (size_t k = 0; k < text_len && j < output_size - 1; k++) {
                        output[j++] = line[i + 1 + k];
                    }
                    j += snprintf(output + j, output_size - j, "%s", VT100_RESET);
                    i = end_paren - line + 1;
                    continue;
                }
            }
        }
        
        output[j++] = line[i++];
    }
    
    // Reset any remaining formatting
    if (in_bold || in_italic || in_code) {
        j += snprintf(output + j, output_size - j, "%s", VT100_RESET);
    }
    
    output[j] = '\0';
}

/**
 * @brief Render markdown file to terminal
 */
static bool RenderMarkdown(const char* file_path) {
    FILE* file = fopen(file_path, "r");
    if (!file) {
        DMOD_LOG_ERROR("Failed to open documentation file: %s\n", file_path);
        return false;
    }
    
    char line[4096];
    char formatted[8192];
    bool in_code_block = false;
    bool in_list = false;
    int list_indent = 0;
    
    while (fgets(line, sizeof(line), file)) {
        TrimTrailing(line);
        
        // Handle code blocks ```
        if (StartsWith(line, "```")) {
            in_code_block = !in_code_block;
            if (in_code_block) {
                printf("%s", VT100_DIM);
            } else {
                printf("%s", VT100_RESET);
            }
            continue;
        }
        
        // If in code block, print as-is with dim color
        if (in_code_block) {
            printf("%s\n", line);
            continue;
        }
        
        // Handle headers
        if (StartsWith(line, "#")) {
            int level = CountLeading(line, '#');
            if (level >= 1 && level <= 6 && line[level] == ' ') {
                const char* text = line + level + 1;
                
                // Different formatting for different header levels
                switch (level) {
                    case 1:
                        printf("\n%s%s%s%s\n", VT100_BOLD, VT100_BLUE, text, VT100_RESET);
                        // Print underline
                        for (size_t i = 0; i < strlen(text); i++) {
                            printf("=");
                        }
                        printf("\n");
                        break;
                    case 2:
                        printf("\n%s%s%s%s\n", VT100_BOLD, VT100_CYAN, text, VT100_RESET);
                        // Print underline
                        for (size_t i = 0; i < strlen(text); i++) {
                            printf("-");
                        }
                        printf("\n");
                        break;
                    case 3:
                        printf("\n%s%s%s%s\n", VT100_BOLD, VT100_GREEN, text, VT100_RESET);
                        break;
                    default:
                        printf("\n%s%s%s\n", VT100_BOLD, text, VT100_RESET);
                        break;
                }
                continue;
            }
        }
        
        // Handle horizontal rules
        if (StartsWith(line, "---") || StartsWith(line, "***") || StartsWith(line, "___")) {
            if (strlen(line) >= 3) {
                printf("%s", VT100_DIM);
                for (int i = 0; i < 80; i++) {
                    printf("─");
                }
                printf("%s\n", VT100_RESET);
                continue;
            }
        }
        
        // Handle bullet lists
        if (StartsWith(line, "- ") || StartsWith(line, "* ") || StartsWith(line, "+ ")) {
            in_list = true;
            const char* text = line + 2;
            ProcessInlineFormatting(text, formatted, sizeof(formatted));
            printf("  %s•%s %s\n", VT100_YELLOW, VT100_RESET, formatted);
            continue;
        }
        
        // Handle numbered lists
        if (isdigit(line[0])) {
            const char* dot = strchr(line, '.');
            if (dot && dot[1] == ' ') {
                in_list = true;
                char num[16];
                size_t num_len = dot - line;
                if (num_len < sizeof(num)) {
                    strncpy(num, line, num_len);
                    num[num_len] = '\0';
                    const char* text = dot + 2;
                    ProcessInlineFormatting(text, formatted, sizeof(formatted));
                    printf("  %s%s.%s %s\n", VT100_YELLOW, num, VT100_RESET, formatted);
                    continue;
                }
            }
        }
        
        // Handle indented code (4 spaces or tab)
        if (StartsWith(line, "    ") || line[0] == '\t') {
            printf("%s%s%s\n", VT100_DIM, line, VT100_RESET);
            continue;
        }
        
        // Empty line - reset list mode
        if (strlen(line) == 0) {
            in_list = false;
            printf("\n");
            continue;
        }
        
        // Regular paragraph text with inline formatting
        ProcessInlineFormatting(line, formatted, sizeof(formatted));
        printf("%s\n", formatted);
    }
    
    // Reset formatting at end
    printf("%s", VT100_RESET);
    
    fclose(file);
    return true;
}

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
    const char* module_name = NULL;
    const char* custom_doc_dir = NULL;
    
    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            PrintHelp(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("dmf-man ver. " DMOD_VERSION_STRING "\n");
            return 0;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--doc-dir") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s requires an argument\n", argv[i]);
                PrintUsage(argv[0]);
                return 1;
            }
            custom_doc_dir = argv[++i];
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
            PrintUsage(argv[0]);
            return 1;
        } else {
            if (module_name) {
                fprintf(stderr, "Error: Multiple module names specified\n");
                PrintUsage(argv[0]);
                return 1;
            }
            module_name = argv[i];
        }
    }
    
    if (!module_name) {
        fprintf(stderr, "Error: No module name specified\n");
        PrintUsage(argv[0]);
        return 1;
    }
    
    // Initialize DMOD system
    if (!Dmod_Initialize()) {
        DMOD_LOG_ERROR("Failed to initialize DMOD system\n");
        return 1;
    }
    
    // Find documentation
    char* doc_path = FindDocumentation(module_name, custom_doc_dir);
    if (!doc_path) {
        fprintf(stderr, "Error: No documentation found for module: %s\n", module_name);
        fprintf(stderr, "\nSearched in:\n");
        
        if (custom_doc_dir) {
            fprintf(stderr, "  - %s/%s.md\n", custom_doc_dir, module_name);
            fprintf(stderr, "  - %s/%s/README.md\n", custom_doc_dir, module_name);
            fprintf(stderr, "  - %s/README.md\n", custom_doc_dir);
        }
        
        const char* doc_dir = Dmod_GetEnv(ENV_DOC_DIR);
        if (doc_dir) {
            fprintf(stderr, "  - %s/%s.md\n", doc_dir, module_name);
            fprintf(stderr, "  - %s/%s/README.md\n", doc_dir, module_name);
        }
        
        const char* dmf_dir = Dmod_GetEnv(ENV_DMF_DIR);
        if (!dmf_dir) {
            dmf_dir = DEFAULT_DMF_DIR;
        }
        fprintf(stderr, "  - %s/%s/docs/%s.md\n", dmf_dir, module_name, module_name);
        fprintf(stderr, "  - %s/%s/docs/README.md\n", dmf_dir, module_name);
        fprintf(stderr, "  - %s/%s/README.md\n", dmf_dir, module_name);
        
        Dmod_Finalize();
        return 1;
    }
    
    // Render the documentation
    printf("%s=== Documentation for %s ===%s\n", VT100_BOLD, module_name, VT100_RESET);
    printf("%sFile: %s%s\n\n", VT100_DIM, doc_path, VT100_RESET);
    
    bool success = RenderMarkdown(doc_path);
    
    free(doc_path);
    Dmod_Finalize();
    
    return success ? 0 : 1;
}
