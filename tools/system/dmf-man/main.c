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

// Buffer safety margin for VT100 codes
#define VT100_SAFETY_MARGIN 50

// Colors
#define VT100_BLACK "\033[30m"
#define VT100_RED "\033[31m"
#define VT100_GREEN "\033[32m"
#define VT100_YELLOW "\033[33m"
#define VT100_BLUE "\033[34m"
#define VT100_MAGENTA "\033[35m"
#define VT100_CYAN "\033[36m"
#define VT100_WHITE "\033[37m"

// Terminal control
#define VT100_CLEAR_SCREEN "\033[2J"
#define VT100_CURSOR_HOME "\033[H"
#define VT100_CURSOR_SAVE "\0337"
#define VT100_CURSOR_RESTORE "\0338"
#define VT100_HIDE_CURSOR "\033[?25l"
#define VT100_SHOW_CURSOR "\033[?25h"

// Paging defaults
#define DEFAULT_PAGE_HEIGHT 24  // Standard terminal height minus status line

// Dynamic buffer for paged output
typedef struct {
    char** lines;
    size_t count;
    size_t capacity;
} PageBuffer_t;

/**
 * @brief Initialize page buffer
 */
static void PageBuffer_Init(PageBuffer_t* buffer) {
    buffer->lines = NULL;
    buffer->count = 0;
    buffer->capacity = 0;
}

/**
 * @brief Add line to page buffer
 */
static bool PageBuffer_AddLine(PageBuffer_t* buffer, const char* line) {
    if (buffer->count >= buffer->capacity) {
        size_t new_capacity = buffer->capacity == 0 ? 128 : buffer->capacity * 2;
        char** new_lines = (char**)Dmod_Realloc(buffer->lines, new_capacity * sizeof(char*));
        if (!new_lines) {
            return false;
        }
        buffer->lines = new_lines;
        buffer->capacity = new_capacity;
    }
    
    buffer->lines[buffer->count] = Dmod_StrDup(line);
    if (!buffer->lines[buffer->count]) {
        return false;
    }
    buffer->count++;
    return true;
}

/**
 * @brief Free page buffer
 */
static void PageBuffer_Free(PageBuffer_t* buffer) {
    for (size_t i = 0; i < buffer->count; i++) {
        Dmod_Free(buffer->lines[i]);
    }
    Dmod_Free(buffer->lines);
    buffer->lines = NULL;
    buffer->count = 0;
    buffer->capacity = 0;
}

/**
 * @brief Read a single character in raw mode
 */
static int ReadKey(void) {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        // Handle escape sequences for arrow keys
        if (c == '\033') {
            char seq[3];
            if (read(STDIN_FILENO, &seq[0], 1) != 1) return c;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) return c;
            
            if (seq[0] == '[') {
                switch (seq[1]) {
                    case 'A': return 'k';  // Up arrow -> k
                    case 'B': return 'j';  // Down arrow -> j
                    case 'C': return 'l';  // Right arrow -> l
                    case 'D': return 'h';  // Left arrow -> h
                    case '5': // Page Up
                        if (read(STDIN_FILENO, &seq[2], 1) == 1 && seq[2] == '~') {
                            return 'b';  // Page up -> b
                        }
                        break;
                    case '6': // Page Down
                        if (read(STDIN_FILENO, &seq[2], 1) == 1 && seq[2] == '~') {
                            return 'f';  // Page down -> f
                        }
                        break;
                    case 'H': return 'g';  // Home -> g
                    case 'F': return 'G';  // End -> G
                }
            }
        }
        return c;
    }
    return -1;
}

/**
 * @brief Display paged content
 */
static void DisplayPaged(const PageBuffer_t* buffer) {
    if (buffer->count == 0) {
        return;
    }
    
    size_t current_line = 0;
    size_t page_height = DEFAULT_PAGE_HEIGHT;
    bool quit = false;
    
    // Save original stdin flags
    uint32_t orig_flags = Dmod_Stdin_GetFlags();
    
    // Set raw mode (no echo, no canonical)
    Dmod_Stdin_SetFlags(0);
    
    while (!quit && current_line < buffer->count) {
        // Clear screen and move cursor to home
        Dmod_Printf("%s%s", VT100_CLEAR_SCREEN, VT100_CURSOR_HOME);
        
        // Display current page
        size_t lines_displayed = 0;
        for (size_t i = current_line; i < buffer->count && lines_displayed < page_height; i++, lines_displayed++) {
            Dmod_Printf("%s\n", buffer->lines[i]);
        }
        
        // Display status line
        if (current_line + page_height < buffer->count) {
            Dmod_Printf("%s%s-- MORE -- (%.0f%%) [j/k=line, space/b=page, q=quit]%s", 
                       VT100_REVERSE, VT100_BOLD,
                       (100.0 * (current_line + page_height)) / buffer->count,
                       VT100_RESET);
        } else {
            Dmod_Printf("%s%s-- END -- [q=quit]%s", 
                       VT100_REVERSE, VT100_BOLD, VT100_RESET);
        }
        
        fflush(stdout);
        
        // Read key
        int key = ReadKey();
        
        switch (key) {
            case 'q':
            case 'Q':
                quit = true;
                break;
            case ' ':  // Space - next page
            case 'f':  // Page down
                if (current_line + page_height < buffer->count) {
                    current_line += page_height;
                }
                break;
            case 'b':  // Page up
                if (current_line >= page_height) {
                    current_line -= page_height;
                } else {
                    current_line = 0;
                }
                break;
            case 'j':  // Down one line
            case '\n':
            case '\r':
                if (current_line < buffer->count) {
                    current_line++;
                }
                break;
            case 'k':  // Up one line
                if (current_line > 0) {
                    current_line--;
                }
                break;
            case 'g':  // Home - go to top
                current_line = 0;
                break;
            case 'G':  // End - go to bottom
                if (buffer->count > page_height) {
                    current_line = buffer->count - page_height;
                } else {
                    current_line = 0;
                }
                break;
        }
        
        // If we're at the end, wait for quit
        if (current_line + page_height >= buffer->count && key != 'q' && key != 'Q') {
            // Already at the end, only allow quit or up movement
            if (key != 'k' && key != 'b' && key != 'g') {
                continue;  // Don't advance, wait for valid key
            }
        }
    }
    
    // Clear screen one final time
    Dmod_Printf("%s%s", VT100_CLEAR_SCREEN, VT100_CURSOR_HOME);
    
    // Restore original stdin flags
    Dmod_Stdin_SetFlags(orig_flags);
}

/**
 * @brief Print usage message
 */
static void PrintUsage(const char* app_name) {
    Dmod_Printf("Usage: %s [OPTIONS] <module_name>\n", app_name);
    Dmod_Printf("\n");
    Dmod_Printf("Options:\n");
    Dmod_Printf("  -d, --doc-dir <path>  Path to documentation directory\n");
    Dmod_Printf("  -p, --paged           Enable paged output (default)\n");
    Dmod_Printf("  -a, --all             Show all content at once (no paging)\n");
    Dmod_Printf("  -h, --help            Show this help message\n");
    Dmod_Printf("  -v, --version         Show version information\n");
    Dmod_Printf("\n");
    Dmod_Printf("Environment Variables:\n");
    Dmod_Printf("  %s     Documentation directory (checked first)\n", ENV_DOC_DIR);
    Dmod_Printf("  %s        DMF directory (used as fallback: <dir>/<module>/docs)\n", ENV_DMF_DIR);
    Dmod_Printf("\n");
    Dmod_Printf("Examples:\n");
    Dmod_Printf("  %s mymodule                    # Search in default locations\n", app_name);
    Dmod_Printf("  %s -d /path/to/docs mymodule   # Use custom documentation directory\n", app_name);
    Dmod_Printf("  %s -a mymodule                 # Show all at once without paging\n", app_name);
    Dmod_Printf("\n");
    Dmod_Printf("Navigation (when paged):\n");
    Dmod_Printf("  Arrow Up/Down    Scroll one line\n");
    Dmod_Printf("  Page Up/Down     Scroll one page\n");
    Dmod_Printf("  Home/End         Go to start/end\n");
    Dmod_Printf("  q or Q           Quit\n");
}

/**
 * @brief Print help message
 */
static void PrintHelp(const char* app_name) {
    Dmod_Printf("-- dmf-man - DMOD Documentation Viewer ver. " DMOD_VERSION_STRING " --\n\n");
    Dmod_Printf("This tool displays documentation for DMOD modules.\n");
    Dmod_Printf("Documentation is rendered from Markdown with basic VT100 formatting.\n\n");
    PrintUsage(app_name);
}

/**
 * @brief Check if a file exists
 */
static bool FileExists(const char* path) {
    return Dmod_FileAvailable(path);
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
        Dmod_SnPrintf(path, sizeof(path), "%s/%s.md", custom_doc_dir, module_name);
        if (FileExists(path)) {
            return Dmod_StrDup(path);
        }
        
        // Try <custom_doc_dir>/<module>/README.md
        Dmod_SnPrintf(path, sizeof(path), "%s/%s/README.md", custom_doc_dir, module_name);
        if (FileExists(path)) {
            return Dmod_StrDup(path);
        }
        
        // Try <custom_doc_dir>/README.md
        Dmod_SnPrintf(path, sizeof(path), "%s/README.md", custom_doc_dir);
        if (FileExists(path)) {
            return Dmod_StrDup(path);
        }
    }
    
    // Try DMOD_DOC_DIR
    const char* doc_dir = Dmod_GetEnv(ENV_DOC_DIR);
    if (doc_dir) {
        // Try <doc_dir>/<module>.md
        Dmod_SnPrintf(path, sizeof(path), "%s/%s.md", doc_dir, module_name);
        if (FileExists(path)) {
            return Dmod_StrDup(path);
        }
        
        // Try <doc_dir>/<module>/README.md
        Dmod_SnPrintf(path, sizeof(path), "%s/%s/README.md", doc_dir, module_name);
        if (FileExists(path)) {
            return Dmod_StrDup(path);
        }
        
        // Try <doc_dir>/README.md
        Dmod_SnPrintf(path, sizeof(path), "%s/README.md", doc_dir);
        if (FileExists(path)) {
            return Dmod_StrDup(path);
        }
    }
    
    // Try DMOD_DMF_DIR
    const char* dmf_dir = Dmod_GetEnv(ENV_DMF_DIR);
    if (!dmf_dir) {
        dmf_dir = DEFAULT_DMF_DIR;
    }
    
    // Try <dmf_dir>/<module>/docs/<module>.md
    Dmod_SnPrintf(path, sizeof(path), "%s/%s/docs/%s.md", dmf_dir, module_name, module_name);
    if (FileExists(path)) {
        return Dmod_StrDup(path);
    }
    
    // Try <dmf_dir>/<module>/docs/README.md
    Dmod_SnPrintf(path, sizeof(path), "%s/%s/docs/README.md", dmf_dir, module_name);
    if (FileExists(path)) {
        return Dmod_StrDup(path);
    }
    
    // Try <dmf_dir>/<module>/README.md
    Dmod_SnPrintf(path, sizeof(path), "%s/%s/README.md", dmf_dir, module_name);
    if (FileExists(path)) {
        return Dmod_StrDup(path);
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
    if (!line) {
        return 0;
    }
    
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
    
    while (line[i] && j < output_size - VT100_SAFETY_MARGIN) {
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
                if (j < output_size - 1) {
                    output[j++] = line[i++];
                }
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
                        if (j < output_size - 1) {
                            output[j++] = line[i + 1 + k];
                        }
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
static bool RenderMarkdown(const char* file_path, bool paged) {
    FILE* file = fopen(file_path, "r");
    if (!file) {
        DMOD_LOG_ERROR("Failed to open documentation file: %s\n", file_path);
        return false;
    }
    
    // Initialize page buffer if paged mode
    PageBuffer_t page_buffer;
    if (paged) {
        PageBuffer_Init(&page_buffer);
    }
    
    char line[4096];
    char formatted[8192];
    char output_line[8192];
    bool in_code_block = false;
    bool in_list = false;
    int list_indent = 0;
    
    while (fgets(line, sizeof(line), file)) {
        TrimTrailing(line);
        
        // Handle code blocks ```
        if (StartsWith(line, "```")) {
            in_code_block = !in_code_block;
            if (in_code_block) {
                Dmod_SnPrintf(output_line, sizeof(output_line), "%s", VT100_DIM);
            } else {
                Dmod_SnPrintf(output_line, sizeof(output_line), "%s", VT100_RESET);
            }
            if (paged) {
                PageBuffer_AddLine(&page_buffer, output_line);
            } else {
                Dmod_Printf("%s", output_line);
            }
            continue;
        }
        
        // If in code block, print as-is with dim color
        if (in_code_block) {
            Dmod_SnPrintf(output_line, sizeof(output_line), "%s", line);
            if (paged) {
                PageBuffer_AddLine(&page_buffer, output_line);
            } else {
                Dmod_Printf("%s\n", output_line);
            }
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
                        Dmod_SnPrintf(output_line, sizeof(output_line), "");
                        if (paged) PageBuffer_AddLine(&page_buffer, output_line); else Dmod_Printf("\n");
                        
                        Dmod_SnPrintf(output_line, sizeof(output_line), "%s%s%s%s", VT100_BOLD, VT100_BLUE, text, VT100_RESET);
                        if (paged) PageBuffer_AddLine(&page_buffer, output_line); else Dmod_Printf("%s\n", output_line);
                        
                        // Print underline
                        char underline[256];
                        size_t text_len = strlen(text);
                        for (size_t i = 0; i < text_len && i < sizeof(underline) - 1; i++) {
                            underline[i] = '=';
                        }
                        underline[text_len < sizeof(underline) ? text_len : sizeof(underline) - 1] = '\0';
                        if (paged) PageBuffer_AddLine(&page_buffer, underline); else Dmod_Printf("%s\n", underline);
                        break;
                    case 2:
                        Dmod_SnPrintf(output_line, sizeof(output_line), "");
                        if (paged) PageBuffer_AddLine(&page_buffer, output_line); else Dmod_Printf("\n");
                        
                        Dmod_SnPrintf(output_line, sizeof(output_line), "%s%s%s%s", VT100_BOLD, VT100_CYAN, text, VT100_RESET);
                        if (paged) PageBuffer_AddLine(&page_buffer, output_line); else Dmod_Printf("%s\n", output_line);
                        
                        // Print underline
                        text_len = strlen(text);
                        for (size_t i = 0; i < text_len && i < sizeof(underline) - 1; i++) {
                            underline[i] = '-';
                        }
                        underline[text_len < sizeof(underline) ? text_len : sizeof(underline) - 1] = '\0';
                        if (paged) PageBuffer_AddLine(&page_buffer, underline); else Dmod_Printf("%s\n", underline);
                        break;
                    case 3:
                        Dmod_SnPrintf(output_line, sizeof(output_line), "");
                        if (paged) PageBuffer_AddLine(&page_buffer, output_line); else Dmod_Printf("\n");
                        
                        Dmod_SnPrintf(output_line, sizeof(output_line), "%s%s%s%s", VT100_BOLD, VT100_GREEN, text, VT100_RESET);
                        if (paged) PageBuffer_AddLine(&page_buffer, output_line); else Dmod_Printf("%s\n", output_line);
                        break;
                    default:
                        Dmod_SnPrintf(output_line, sizeof(output_line), "");
                        if (paged) PageBuffer_AddLine(&page_buffer, output_line); else Dmod_Printf("\n");
                        
                        Dmod_SnPrintf(output_line, sizeof(output_line), "%s%s%s", VT100_BOLD, text, VT100_RESET);
                        if (paged) PageBuffer_AddLine(&page_buffer, output_line); else Dmod_Printf("%s\n", output_line);
                        break;
                }
                continue;
            }
        }
        
        // Handle horizontal rules
        if (StartsWith(line, "---") || StartsWith(line, "***") || StartsWith(line, "___")) {
            if (strlen(line) >= 3) {
                Dmod_SnPrintf(output_line, sizeof(output_line), "%s", VT100_DIM);
                for (int i = 0; i < 80 && strlen(output_line) < sizeof(output_line) - 2; i++) {
                    strcat(output_line, "─");
                }
                strcat(output_line, VT100_RESET);
                if (paged) PageBuffer_AddLine(&page_buffer, output_line); else Dmod_Printf("%s\n", output_line);
                continue;
            }
        }
        
        // Handle bullet lists
        if (StartsWith(line, "- ") || StartsWith(line, "* ") || StartsWith(line, "+ ")) {
            in_list = true;
            const char* text = line + 2;
            ProcessInlineFormatting(text, formatted, sizeof(formatted));
            Dmod_SnPrintf(output_line, sizeof(output_line), "  %s•%s %s", VT100_YELLOW, VT100_RESET, formatted);
            if (paged) PageBuffer_AddLine(&page_buffer, output_line); else Dmod_Printf("%s\n", output_line);
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
                    Dmod_SnPrintf(output_line, sizeof(output_line), "  %s%s.%s %s", VT100_YELLOW, num, VT100_RESET, formatted);
                    if (paged) PageBuffer_AddLine(&page_buffer, output_line); else Dmod_Printf("%s\n", output_line);
                    continue;
                }
            }
        }
        
        // Handle indented code (4 spaces or tab)
        if (StartsWith(line, "    ") || line[0] == '\t') {
            Dmod_SnPrintf(output_line, sizeof(output_line), "%s%s%s", VT100_DIM, line, VT100_RESET);
            if (paged) PageBuffer_AddLine(&page_buffer, output_line); else Dmod_Printf("%s\n", output_line);
            continue;
        }
        
        // Empty line - reset list mode
        if (strlen(line) == 0) {
            in_list = false;
            if (paged) PageBuffer_AddLine(&page_buffer, ""); else Dmod_Printf("\n");
            continue;
        }
        
        // Regular paragraph text with inline formatting
        ProcessInlineFormatting(line, formatted, sizeof(formatted));
        if (paged) PageBuffer_AddLine(&page_buffer, formatted); else Dmod_Printf("%s\n", formatted);
    }
    
    // Reset formatting at end
    Dmod_SnPrintf(output_line, sizeof(output_line), "%s", VT100_RESET);
    if (paged) {
        PageBuffer_AddLine(&page_buffer, output_line);
        // Display paged
        DisplayPaged(&page_buffer);
        PageBuffer_Free(&page_buffer);
    } else {
        Dmod_Printf("%s", output_line);
    }
    
    fclose(file);
    return true;
}

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
    const char* module_name = NULL;
    const char* custom_doc_dir = NULL;
    bool paged = true;  // Paging enabled by default
    
    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            PrintHelp(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            Dmod_Printf("dmf-man ver. " DMOD_VERSION_STRING "\n");
            return 0;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--doc-dir") == 0) {
            if (i + 1 >= argc) {
                DMOD_LOG_ERROR("Error: %s requires an argument\n", argv[i]);
                PrintUsage(argv[0]);
                return 1;
            }
            custom_doc_dir = argv[++i];
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--all") == 0) {
            paged = false;
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--paged") == 0) {
            paged = true;
        } else if (argv[i][0] == '-') {
            DMOD_LOG_ERROR("Error: Unknown option: %s\n", argv[i]);
            PrintUsage(argv[0]);
            return 1;
        } else {
            if (module_name) {
                DMOD_LOG_ERROR("Error: Multiple module names specified\n");
                PrintUsage(argv[0]);
                return 1;
            }
            module_name = argv[i];
        }
    }
    
    if (!module_name) {
        DMOD_LOG_ERROR("Error: No module name specified\n");
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
        DMOD_LOG_ERROR("Error: No documentation found for module: %s\n", module_name);
        DMOD_LOG_ERROR("\nSearched in:\n");
        
        if (custom_doc_dir) {
            DMOD_LOG_ERROR("  - %s/%s.md\n", custom_doc_dir, module_name);
            DMOD_LOG_ERROR("  - %s/%s/README.md\n", custom_doc_dir, module_name);
            DMOD_LOG_ERROR("  - %s/README.md\n", custom_doc_dir);
        }
        
        const char* doc_dir = Dmod_GetEnv(ENV_DOC_DIR);
        if (doc_dir) {
            DMOD_LOG_ERROR("  - %s/%s.md\n", doc_dir, module_name);
            DMOD_LOG_ERROR("  - %s/%s/README.md\n", doc_dir, module_name);
        }
        
        const char* dmf_dir = Dmod_GetEnv(ENV_DMF_DIR);
        if (!dmf_dir) {
            dmf_dir = DEFAULT_DMF_DIR;
        }
        DMOD_LOG_ERROR("  - %s/%s/docs/%s.md\n", dmf_dir, module_name, module_name);
        DMOD_LOG_ERROR("  - %s/%s/docs/README.md\n", dmf_dir, module_name);
        DMOD_LOG_ERROR("  - %s/%s/README.md\n", dmf_dir, module_name);
        
        DMOD_LOG_ERROR("\nTip: You can try to install documentation with:\n");
        DMOD_LOG_ERROR("  dmf-get docs %s\n", module_name);
        
        Dmod_Deinitialize();
        return 1;
    }
    
    // Render the documentation
    Dmod_Printf("%s=== Documentation for %s ===%s\n", VT100_BOLD, module_name, VT100_RESET);
    Dmod_Printf("%sFile: %s%s\n\n", VT100_DIM, doc_path, VT100_RESET);
    
    bool success = RenderMarkdown(doc_path, paged);
    
    Dmod_Free(doc_path);
    Dmod_Deinitialize();
    
    return success ? 0 : 1;
}
