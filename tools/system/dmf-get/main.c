/**
 * @file main.c
 * @brief DMF Package Manager - dmf-get tool
 * 
 * This tool downloads and manages DMF packages from manifest files.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <curl/curl.h>
#include "dmod.h"
#include "dmod_manifest.h"
#include "dmod_dependencies.h"
#include "dmod_version.h"
#include "dmod_resource.h"

// Default paths
#define DEFAULT_DMF_DIR "./dmf"
#define DEFAULT_DMFC_DIR "./dmfc"
#define DEFAULT_MANIFEST "manifest.dmm"
#define DEFAULT_MANIFEST_URL "https://raw.githubusercontent.com/choco-technologies/dmod-registry/refs/heads/main/manifest.dmm"

// Environment variables
#define ENV_TOOLS_NAME "DMOD_TOOLS_NAME"
#define ENV_DMF_DIR "DMOD_DMF_DIR"
#define ENV_DMFC_DIR "DMOD_DMFC_DIR"
#define ENV_MANIFEST "DMOD_MANIFEST"
#define ENV_INC_DIR "DMOD_INC_DIR"
#define ENV_DOC_DIR "DMOD_DOC_DIR"

/**
 * @brief Structure to hold download data for curl
 */
typedef struct {
    char* data;
    size_t size;
} DownloadBuffer_t;

/**
 * @brief Curl write callback
 */
static size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    DownloadBuffer_t* buffer = (DownloadBuffer_t*)userp;
    
    char* new_data = Dmod_Realloc(buffer->data, buffer->size + realsize + 1);
    if (!new_data) {
        DMOD_LOG_ERROR("Out of memory\n");
        return 0;
    }
    
    buffer->data = new_data;
    memcpy(&(buffer->data[buffer->size]), contents, realsize);
    buffer->size += realsize;
    buffer->data[buffer->size] = 0;
    
    return realsize;
}

/**
 * @brief Download function using libcurl
 */
static bool DownloadWithCurl(const char* url, char** buffer, size_t* size, void* user_data) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        DMOD_LOG_ERROR("Failed to initialize curl\n");
        return false;
    }
    
    DownloadBuffer_t download = {0};
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &download);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "dmf-get/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        DMOD_LOG_ERROR("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        Dmod_Free(download.data);
        curl_easy_cleanup(curl);
        return false;
    }
    
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    curl_easy_cleanup(curl);
    
    if (response_code != 200) {
        DMOD_LOG_ERROR("HTTP error %ld for URL: %s\n", response_code, url);
        Dmod_Free(download.data);
        return false;
    }
    
    *buffer = download.data;
    *size = download.size;
    
    return true;
}

/**
 * @brief Download a file from URL to local path
 */
static bool DownloadFile(const char* url, const char* output_path) {
    DMOD_LOG_INFO("Downloading: %s\n", url);
    
    char* buffer = NULL;
    size_t size = 0;
    
    if (!DownloadWithCurl(url, &buffer, &size, NULL)) {
        return false;
    }
    
    void* file = Dmod_FileOpen(output_path, "wb");
    if (!file) {
        DMOD_LOG_ERROR("Cannot create file: %s\n", output_path);
        Dmod_Free(buffer);
        return false;
    }
    
    size_t written = Dmod_FileWrite(buffer, 1, size, file);
    Dmod_FileClose(file);
    Dmod_Free(buffer);
    
    if (written != size) {
        DMOD_LOG_ERROR("Failed to write complete file: %s\n", output_path);
        return false;
    }
    
    DMOD_LOG_INFO("Downloaded: %s (%zu bytes)\n", output_path, size);
    return true;
}

/**
 * @brief Structure to track installation counts
 */
typedef struct {
    int success_count;
    int failed_count;
} InstallationCounts_t;

// Forward declarations
static int DownloadModule(const char* module_name, const char* module_version,
                          Dmod_ManifestContext_t* manifest_ctx, 
                          const char* output_dir, const char* tools_name,
                          const char* arch_name, const char* cpu_name,
                          const char* cpu_family, const char* preferred_type,
                          bool download_dependencies, const char* default_manifest,
                          bool ignore_missing, bool skip_dmod_ver_check,
                          bool mini_mode, bool auto_accept_license, 
                          InstallationCounts_t* counts);

/**
 * @brief Sanitize path for use in shell commands
 * 
 * Checks that the path doesn't contain characters that could be used for
 * command injection (quotes, semicolons, pipes, etc.)
 * 
 * @param path Path to validate
 * @return true if path is safe, false otherwise
 */
static bool IsPathSafe(const char* path) {
    if (!path) return false;
    
    // Check for dangerous characters
    const char* dangerous = "'\";|&$`<>(){}[]!\\*?";
    for (const char* p = path; *p; p++) {
        if (strchr(dangerous, *p)) {
            return false;
        }
    }
    
    // Path should not start with - (to avoid being interpreted as option)
    if (path[0] == '-') {
        return false;
    }
    
    return true;
}

/**
 * @brief Convert DMOD_VERSION hex to semantic version
 */
static void GetCurrentDmodVersion(Dmod_SemanticVersion_t* version) {
    // DMOD_VERSION format: 0xMMmmpppp (Major, minor, patch)
    version->major = dmod_VERSION_MAJOR;
    version->minor = dmod_VERSION_MINOR;
    version->patch = 0; // Patch not defined in dmod_VERSION
}

/**
 * @brief Extract specific resource (headers or docs) from ZIP file
 * 
 * @param zip_path Path to the ZIP file
 * @param output_dir Directory to copy the extracted resource to
 * @param module_name Module name to search for
 * @param resource_key Resource key to extract ("inc" for headers, "docs" for documentation)
 * @return true if extraction succeeded and resource was found and copied
 */
static bool ExtractResourceFromZip(const char* zip_path, const char* output_dir, 
                                   const char* module_name, const char* resource_key) {
    // Create temporary extraction directory in /tmp
    char extract_dir[512];
    Dmod_SnPrintf(extract_dir, sizeof(extract_dir), "/tmp/dmod_extract_%s_%d", module_name, (int)getpid());
    
    if (Dmod_MakeDir(extract_dir, 0755) != 0) {
        DMOD_LOG_ERROR("Failed to create temporary directory: %s\n", extract_dir);
        return false;
    }
    
    // Validate paths for safety
    if (!IsPathSafe(zip_path) || !IsPathSafe(output_dir)) {
        DMOD_LOG_ERROR("Invalid path detected (contains unsafe characters)\n");
        char rm_cmd[1024];
        Dmod_SnPrintf(rm_cmd, sizeof(rm_cmd), "rm -rf \"%s\"", extract_dir);
        system(rm_cmd);
        return false;
    }
    
    // Use system unzip command
    char unzip_cmd[1024];
    Dmod_SnPrintf(unzip_cmd, sizeof(unzip_cmd), "unzip -o -q \"%s\" -d \"%s\"", zip_path, extract_dir);
    
    int result = system(unzip_cmd);
    if (result != 0) {
        DMOD_LOG_ERROR("Failed to extract ZIP file: %s\n", zip_path);
        // Clean up temp directory
        char rm_cmd[1024];
        Dmod_SnPrintf(rm_cmd, sizeof(rm_cmd), "rm -rf \"%s\"", extract_dir);
        system(rm_cmd);
        return false;
    }
    
    DMOD_LOG_INFO("Extracted ZIP to temporary directory\n");
    
    // Look for .dmr file first
    char dmr_file[512] = "";
    void* dir = Dmod_OpenDir(extract_dir);
    if (dir) {
        const char* entry_name;
        while ((entry_name = Dmod_ReadDir(dir)) != NULL) {
            if (strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0) {
                continue;
            }
            
            size_t len = strlen(entry_name);
            bool is_dmr = (len > 4 && strcmp(entry_name + len - 4, ".dmr") == 0);
            
            if (is_dmr) {
                char expected_dmr[256];
                Dmod_SnPrintf(expected_dmr, sizeof(expected_dmr), "%s.dmr", module_name);
                if (strcmp(entry_name, expected_dmr) == 0) {
                    Dmod_SnPrintf(dmr_file, sizeof(dmr_file), "%s/%s", extract_dir, entry_name);
                    DMOD_LOG_INFO("Found resource file: %s\n", entry_name);
                    break;
                }
            }
        }
        Dmod_CloseDir(dir);
    }
    
    char source_path[1024] = "";
    char destination_path[1024] = "";
    bool found_resource = false;
    
    // Try to get resource path from .dmr file
    if (dmr_file[0] != '\0') {
        DMOD_LOG_INFO("Reading resource path from .dmr file\n");
        Dmod_ResourceContext_t* res_ctx = Dmod_Resource_Init(output_dir, module_name);
        if (res_ctx) {
            if (Dmod_Resource_ParseFile(res_ctx, dmr_file)) {
                size_t res_count = Dmod_Resource_GetEntryCount(res_ctx);
                for (size_t i = 0; i < res_count; i++) {
                    Dmod_ResourceEntry_t res_entry;
                    if (Dmod_Resource_GetEntry(res_ctx, i, &res_entry)) {
                        if (strcmp(res_entry.key, resource_key) == 0) {
                            // Build full source path
                            Dmod_SnPrintf(source_path, sizeof(source_path), "%s/%s", 
                                        extract_dir, res_entry.source);
                            // Store the resolved destination from DMR
                            Dmod_SnPrintf(destination_path, sizeof(destination_path), "%s", res_entry.destination);
                            found_resource = true;
                            DMOD_LOG_INFO("Found %s resource in .dmr: %s => %s\n", 
                                        resource_key, res_entry.source, res_entry.destination);
                            break;
                        }
                    }
                }
            }
            Dmod_Resource_Free(res_ctx);
        }
    }
    
    // Fall back to standard structure: <module>/<resource_key>
    if (!found_resource) {
        DMOD_LOG_INFO("No .dmr file or resource not found in .dmr, using default structure\n");
        Dmod_SnPrintf(source_path, sizeof(source_path), "%s/%s/%s", 
                    extract_dir, module_name, resource_key);
        // Use output_dir as destination when no DMR entry found
        Dmod_SnPrintf(destination_path, sizeof(destination_path), "%s", output_dir);
    }
    
    // Validate destination path for safety
    if (!IsPathSafe(destination_path)) {
        DMOD_LOG_ERROR("Invalid destination path detected (contains unsafe characters)\n");
        char rm_cmd[1024];
        Dmod_SnPrintf(rm_cmd, sizeof(rm_cmd), "rm -rf \"%s\"", extract_dir);
        system(rm_cmd);
        return false;
    }
    
    // Check if source exists
    struct stat st;
    if (stat(source_path, &st) != 0) {
        DMOD_LOG_ERROR("Resource '%s' not found in package at: %s\n", resource_key, source_path);
        // Clean up temp directory
        char rm_cmd[1024];
        Dmod_SnPrintf(rm_cmd, sizeof(rm_cmd), "rm -rf \"%s\"", extract_dir);
        system(rm_cmd);
        return false;
    }
    
    // Ensure output directory exists
    char mkdir_cmd[2048];
    Dmod_SnPrintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\"", destination_path);
    int mkdir_result = system(mkdir_cmd);
    if (mkdir_result != 0) {
        DMOD_LOG_ERROR("Failed to create output directory: %s\n", destination_path);
        // Clean up temp directory
        char rm_cmd[1024];
        Dmod_SnPrintf(rm_cmd, sizeof(rm_cmd), "rm -rf \"%s\"", extract_dir);
        system(rm_cmd);
        return false;
    }
    
    // Copy the resource
    char cp_cmd[2048];
    if (S_ISDIR(st.st_mode)) {
        // For directories, copy contents (not the directory itself) to avoid nested structure
        Dmod_SnPrintf(cp_cmd, sizeof(cp_cmd), "cp -r \"%s/.\" \"%s\"", source_path, destination_path);
    } else {
        // For files, copy the file
        Dmod_SnPrintf(cp_cmd, sizeof(cp_cmd), "cp \"%s\" \"%s\"", source_path, destination_path);
    }
    
    int cp_result = system(cp_cmd);
    if (cp_result == 0) {
        DMOD_LOG_INFO("Resource '%s' installed successfully to: %s\n", resource_key, destination_path);
    } else {
        DMOD_LOG_ERROR("Failed to install resource '%s'\n", resource_key);
        // Clean up temp directory
        char rm_cmd[1024];
        Dmod_SnPrintf(rm_cmd, sizeof(rm_cmd), "rm -rf \"%s\"", extract_dir);
        system(rm_cmd);
        return false;
    }
    
    // Clean up temp directory
    char rm_temp_cmd[1024];
    Dmod_SnPrintf(rm_temp_cmd, sizeof(rm_temp_cmd), "rm -rf \"%s\"", extract_dir);
    system(rm_temp_cmd);
    
    return true;
}

/**
 * @brief Display license file content to the user
 * 
 * @param license_path Path to the license file
 */
static void DisplayLicense(const char* license_path) {
    FILE* file = fopen(license_path, "r");
    if (!file) {
        DMOD_LOG_ERROR("Failed to open license file: %s\n", license_path);
        return;
    }
    
    Dmod_Printf("\n");
    Dmod_Printf("================================================================================\n");
    Dmod_Printf("LICENSE\n");
    Dmod_Printf("================================================================================\n");
    
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), file)) {
        Dmod_Printf("%s", buffer);
    }
    
    Dmod_Printf("================================================================================\n");
    Dmod_Printf("\n");
    
    fclose(file);
}

/**
 * @brief Prompt user to accept license
 * 
 * @return true if user accepts (y/Y), false otherwise
 */
static bool PromptLicenseAcceptance(void) {
    Dmod_Printf("Do you accept the license terms? [y/N]: ");
    fflush(stdout);
    
    char response[10];
    if (fgets(response, sizeof(response), stdin) == NULL) {
        return false;  // Error reading input
    }
    
    // Trim newline
    size_t len = strlen(response);
    if (len > 0 && response[len - 1] == '\n') {
        response[len - 1] = '\0';
    }
    
    // Accept only 'y' or 'Y'
    if (strcmp(response, "y") == 0 || strcmp(response, "Y") == 0) {
        return true;
    }
    
    return false;
}

/**
 * @brief Check and handle license acceptance for a module
 * 
 * @param extract_dir Temporary directory where ZIP was extracted
 * @param module_name Module name
 * @param output_dir Output directory for the module
 * @param auto_accept If true, automatically accept license without prompting
 * @return true if license is accepted or not found, false if rejected
 */
static bool CheckLicenseAcceptance(const char* extract_dir, const char* module_name,
                                   const char* output_dir, bool auto_accept) {
    // Look for .dmr file to find license path
    char dmr_file[512] = "";
    void* dir = Dmod_OpenDir(extract_dir);
    if (dir) {
        const char* entry_name;
        while ((entry_name = Dmod_ReadDir(dir)) != NULL) {
            if (strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0) {
                continue;
            }
            
            size_t len = strlen(entry_name);
            bool is_dmr = (len > 4 && strcmp(entry_name + len - 4, ".dmr") == 0);
            
            if (is_dmr) {
                char expected_dmr[256];
                Dmod_SnPrintf(expected_dmr, sizeof(expected_dmr), "%s.dmr", module_name);
                if (strcmp(entry_name, expected_dmr) == 0) {
                    Dmod_SnPrintf(dmr_file, sizeof(dmr_file), "%s/%s", extract_dir, entry_name);
                    break;
                }
            }
        }
        Dmod_CloseDir(dir);
    }
    
    // If no .dmr file found, no license to check
    if (dmr_file[0] == '\0') {
        return true;
    }
    
    // Parse .dmr file to find license resource
    char license_source[1024] = "";
    Dmod_ResourceContext_t* res_ctx = Dmod_Resource_Init(output_dir, module_name);
    if (res_ctx) {
        if (Dmod_Resource_ParseFile(res_ctx, dmr_file)) {
            size_t res_count = Dmod_Resource_GetEntryCount(res_ctx);
            for (size_t i = 0; i < res_count; i++) {
                Dmod_ResourceEntry_t res_entry;
                if (Dmod_Resource_GetEntry(res_ctx, i, &res_entry)) {
                    if (strcmp(res_entry.key, "license") == 0) {
                        // Found license entry
                        Dmod_SnPrintf(license_source, sizeof(license_source), "%s/%s", 
                                    extract_dir, res_entry.source);
                        break;
                    }
                }
            }
        }
        Dmod_Resource_Free(res_ctx);
    }
    
    // If no license found in .dmr, no license to check
    if (license_source[0] == '\0') {
        return true;
    }
    
    // Check if license file exists
    if (Dmod_Access(license_source, DMOD_R_OK) != 0) {
        DMOD_LOG_WARN("License file specified in .dmr but not found: %s\n", license_source);
        return true;  // Continue installation if license file is missing
    }
    
    // If auto-accept is enabled, skip prompt
    if (auto_accept) {
        DMOD_LOG_INFO("License found - automatically accepting (--yes flag)\n");
        return true;
    }
    
    // Display license and prompt for acceptance
    DisplayLicense(license_source);
    
    if (PromptLicenseAcceptance()) {
        DMOD_LOG_INFO("License accepted\n");
        return true;
    } else {
        DMOD_LOG_INFO("License rejected - installation cancelled\n");
        return false;
    }
}

/**
 * @brief Extract ZIP file and find DMF/DMFC file
 * 
 * @param zip_path Path to the ZIP file
 * @param output_dir Directory to copy the extracted file to
 * @param module_name Module name to search for
 * @param preferred_type Preferred file type (dmf or dmfc)
 * @param mini_mode If true, only install dmf/dmfc files (ignore other resources from .dmr)
 * @param auto_accept_license If true, automatically accept license without prompting
 * @param output_file Buffer to store the path to the final installed file
 * @param output_file_size Size of output_file buffer
 * @param output_dmd_file Buffer to store the path to the .dmd file if found (can be NULL)
 * @param output_dmd_file_size Size of output_dmd_file buffer
 * @return true if extraction succeeded and DMF/DMFC file was found and copied
 */
static bool ExtractZipAndFindModule(const char* zip_path, const char* output_dir, 
                                     const char* module_name, const char* preferred_type,
                                     bool mini_mode, bool auto_accept_license,
                                     char* output_file, size_t output_file_size,
                                     char* output_dmd_file, size_t output_dmd_file_size) {
    // Create temporary extraction directory in /tmp
    char extract_dir[512];
    Dmod_SnPrintf(extract_dir, sizeof(extract_dir), "/tmp/dmod_extract_%s_%d", module_name, (int)getpid());
    
    if (Dmod_MakeDir(extract_dir, 0755) != 0) {
        DMOD_LOG_ERROR("Failed to create temporary directory: %s\n", extract_dir);
        return false;
    }
    
    // Use system unzip command
    char unzip_cmd[1024];
    Dmod_SnPrintf(unzip_cmd, sizeof(unzip_cmd), "unzip -o -q \"%s\" -d \"%s\"", zip_path, extract_dir);
    
    int result = system(unzip_cmd);
    if (result != 0) {
        DMOD_LOG_ERROR("Failed to extract ZIP file: %s\n", zip_path);
        // Clean up temp directory
        char rm_cmd[1024];
        Dmod_SnPrintf(rm_cmd, sizeof(rm_cmd), "rm -rf \"%s\"", extract_dir);
        system(rm_cmd);
        return false;
    }
    
    DMOD_LOG_INFO("Extracted ZIP to temporary directory\n");
    
    // Search for .dmf or .dmfc files in the extracted directory
    void* dir = Dmod_OpenDir(extract_dir);
    if (!dir) {
        DMOD_LOG_ERROR("Cannot open extracted directory: %s\n", extract_dir);
        // Clean up temp directory
        char rm_cmd[1024];
        Dmod_SnPrintf(rm_cmd, sizeof(rm_cmd), "rm -rf \"%s\"", extract_dir);
        system(rm_cmd);
        return false;
    }
    
    const char* entry_name;
    char best_match[512] = "";
    char fallback_match[512] = "";
    char dmd_file[512] = "";
    char dmr_file[512] = "";
    int best_priority = 0;
    
    while ((entry_name = Dmod_ReadDir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0) {
            continue;
        }
        
        // Check if it's a .dmf, .dmfc, .dmd or .dmr file
        size_t len = strlen(entry_name);
        bool is_dmf = (len > 4 && strcmp(entry_name + len - 4, ".dmf") == 0);
        bool is_dmfc = (len > 5 && strcmp(entry_name + len - 5, ".dmfc") == 0);
        bool is_dmd = (len > 4 && strcmp(entry_name + len - 4, ".dmd") == 0);
        bool is_dmr = (len > 4 && strcmp(entry_name + len - 4, ".dmr") == 0);
        
        // If it's a .dmd file, check if it matches the module name
        if (is_dmd) {
            char expected_dmd[256];
            Dmod_SnPrintf(expected_dmd, sizeof(expected_dmd), "%s.dmd", module_name);
            if (strcmp(entry_name, expected_dmd) == 0) {
                Dmod_SnPrintf(dmd_file, sizeof(dmd_file), "%s/%s", extract_dir, entry_name);
                DMOD_LOG_INFO("Found dependencies file: %s\n", entry_name);
            }
            continue;
        }
        
        // If it's a .dmr file, check if it matches the module name
        if (is_dmr) {
            char expected_dmr[256];
            Dmod_SnPrintf(expected_dmr, sizeof(expected_dmr), "%s.dmr", module_name);
            if (strcmp(entry_name, expected_dmr) == 0) {
                Dmod_SnPrintf(dmr_file, sizeof(dmr_file), "%s/%s", extract_dir, entry_name);
                DMOD_LOG_INFO("Found resource file: %s\n", entry_name);
            }
            continue;
        }
        
        if (!is_dmf && !is_dmfc) continue;
        
        // Save as fallback if we don't have one yet
        if (fallback_match[0] == '\0') {
            Dmod_SnPrintf(fallback_match, sizeof(fallback_match), "%s/%s", extract_dir, entry_name);
        }
        
        int priority = 0;
        
        // Priority 4: Exact match with module name and preferred type
        if (preferred_type) {
            if ((strcmp(preferred_type, "dmf") == 0 && is_dmf) || 
                (strcmp(preferred_type, "dmfc") == 0 && is_dmfc)) {
                // Check if filename matches module name
                char expected_name[256];
                Dmod_SnPrintf(expected_name, sizeof(expected_name), "%s.%s", 
                             module_name, preferred_type);
                if (strcmp(entry_name, expected_name) == 0) {
                    priority = 4;
                }
            }
        }
        
        // Priority 3: Exact match with module name (any type)
        if (priority == 0) {
            char expected_dmf[256];
            char expected_dmfc[256];
            Dmod_SnPrintf(expected_dmf, sizeof(expected_dmf), "%s.dmf", module_name);
            Dmod_SnPrintf(expected_dmfc, sizeof(expected_dmfc), "%s.dmfc", module_name);
            if (strcmp(entry_name, expected_dmf) == 0 || strcmp(entry_name, expected_dmfc) == 0) {
                priority = 3;
            }
        }
        
        // Priority 2: Preferred type match
        if (priority == 0 && preferred_type) {
            if ((strcmp(preferred_type, "dmf") == 0 && is_dmf) || 
                (strcmp(preferred_type, "dmfc") == 0 && is_dmfc)) {
                priority = 2;
            }
        }
        
        // Priority 1: Any valid file (already handled as fallback)
        
        if (priority > best_priority) {
            best_priority = priority;
            Dmod_SnPrintf(best_match, sizeof(best_match), "%s/%s", extract_dir, entry_name);
        }
    }
    
    Dmod_CloseDir(dir);
    
    // Check license acceptance before proceeding
    if (!CheckLicenseAcceptance(extract_dir, module_name, output_dir, auto_accept_license)) {
        DMOD_LOG_INFO("Installation cancelled by user\n");
        // Clean up temp directory
        char rm_cmd[1024];
        Dmod_SnPrintf(rm_cmd, sizeof(rm_cmd), "rm -rf \"%s\"", extract_dir);
        system(rm_cmd);
        return false;
    }
    
    // Use best match or fallback
    const char* selected_temp_file = (best_match[0] != '\0') ? best_match : fallback_match;
    
    if (selected_temp_file[0] == '\0') {
        DMOD_LOG_ERROR("No .dmf or .dmfc file found in ZIP archive\n");
        // Clean up temp directory
        char rm_cmd[1024];
        Dmod_SnPrintf(rm_cmd, sizeof(rm_cmd), "rm -rf \"%s\"", extract_dir);
        system(rm_cmd);
        return false;
    }
    
    // Get just the filename from the selected path
    const char* filename = strrchr(selected_temp_file, '/');
    if (filename) {
        filename++; // Skip the '/'
    } else {
        filename = selected_temp_file;
    }
    
    // Build destination path directly in output_dir
    char dest_path[512];
    Dmod_SnPrintf(dest_path, sizeof(dest_path), "%s/%s", output_dir, filename);
    
    // Copy the file to the destination
    char cp_cmd[1024];
    Dmod_SnPrintf(cp_cmd, sizeof(cp_cmd), "cp \"%s\" \"%s\"", selected_temp_file, dest_path);
    result = system(cp_cmd);
    
    if (result != 0) {
        DMOD_LOG_ERROR("Failed to copy module file to destination\n");
        // Clean up temp directory
        char rm_cmd[1024];
        Dmod_SnPrintf(rm_cmd, sizeof(rm_cmd), "rm -rf \"%s\"", extract_dir);
        system(rm_cmd);
        return false;
    }
    
    // Store the final destination path
    strncpy(output_file, dest_path, output_file_size - 1);
    output_file[output_file_size - 1] = '\0';
    
    // Copy the .dmd file if found and if caller wants it
    if (dmd_file[0] != '\0' && output_dmd_file != NULL && output_dmd_file_size > 0) {
        const char* dmd_filename = strrchr(dmd_file, '/');
        if (dmd_filename) {
            dmd_filename++; // Skip the '/'
        } else {
            dmd_filename = dmd_file;
        }
        
        char dmd_dest_path[512];
        Dmod_SnPrintf(dmd_dest_path, sizeof(dmd_dest_path), "%s/%s", output_dir, dmd_filename);
        
        char cp_dmd_cmd[1024];
        Dmod_SnPrintf(cp_dmd_cmd, sizeof(cp_dmd_cmd), "cp \"%s\" \"%s\"", dmd_file, dmd_dest_path);
        result = system(cp_dmd_cmd);
        
        if (result == 0) {
            strncpy(output_dmd_file, dmd_dest_path, output_dmd_file_size - 1);
            output_dmd_file[output_dmd_file_size - 1] = '\0';
            DMOD_LOG_INFO("Dependencies file copied to: %s\n", output_dmd_file);
        }
    }
    
    // Process .dmr file if found
    if (dmr_file[0] != '\0') {
        DMOD_LOG_INFO("Processing resource file: %s\n", dmr_file);
        
        // Initialize resource parser
        Dmod_ResourceContext_t* res_ctx = Dmod_Resource_Init(output_dir, module_name);
        if (!res_ctx) {
            DMOD_LOG_ERROR("Failed to initialize resource parser\n");
        } else {
            // Parse the .dmr file
            if (!Dmod_Resource_ParseFile(res_ctx, dmr_file)) {
                DMOD_LOG_ERROR("Failed to parse resource file: %s\n", 
                        Dmod_Resource_GetError(res_ctx));
                Dmod_Resource_Free(res_ctx);
            } else {
                size_t res_count = Dmod_Resource_GetEntryCount(res_ctx);
                DMOD_LOG_INFO("Found %zu resource entries in .dmr file\n", res_count);
                
                // Install resources based on mini_mode
                for (size_t i = 0; i < res_count; i++) {
                    Dmod_ResourceEntry_t res_entry;
                    if (!Dmod_Resource_GetEntry(res_ctx, i, &res_entry)) {
                        DMOD_LOG_ERROR("Failed to get resource entry %zu\n", i);
                        continue;
                    }
                    
                    // In mini mode, only install dmf/dmfc resources
                    if (mini_mode && !res_entry.is_dmf_dmfc) {
                        DMOD_LOG_INFO("  Skipping resource '%s' (mini mode)\n", res_entry.key);
                        continue;
                    }
                    
                    DMOD_LOG_INFO("  Installing resource '%s': %s => %s\n", 
                           res_entry.key, res_entry.source, res_entry.destination);
                    
                    // Build source path (relative to extract_dir)
                    char full_source[1024];
                    Dmod_SnPrintf(full_source, sizeof(full_source), "%s/%s", 
                                 extract_dir, res_entry.source);
                    
                    // Validate paths for safety
                    if (!IsPathSafe(full_source) || !IsPathSafe(res_entry.destination)) {
                        DMOD_LOG_ERROR("  Invalid path detected (contains unsafe characters)\n");
                        continue;
                    }
                    
                    // Check if source is a file or directory
                    struct stat st;
                    if (stat(full_source, &st) != 0) {
                        DMOD_LOG_WARN("  Resource source not found: %s\n", full_source);
                        continue;
                    }
                    
                    // Create destination directory if needed
                    char dest_parent[1024];
                    strncpy(dest_parent, res_entry.destination, sizeof(dest_parent) - 1);
                    dest_parent[sizeof(dest_parent) - 1] = '\0';
                    
                    char* last_slash = strrchr(dest_parent, '/');
                    if (last_slash) {
                        *last_slash = '\0';
                        
                        // Validate parent path
                        if (!IsPathSafe(dest_parent)) {
                            DMOD_LOG_ERROR("  Invalid destination path (contains unsafe characters)\n");
                            continue;
                        }
                        
                        char mkdir_cmd[2048];
                        Dmod_SnPrintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\"", dest_parent);
                        int mkdir_result = system(mkdir_cmd);
                        if (mkdir_result != 0) {
                            DMOD_LOG_ERROR("  Failed to create destination directory: %s\n", dest_parent);
                            continue;
                        }
                    }
                    
                    // Copy the resource
                    char cp_res_cmd[2048];
                    if (S_ISDIR(st.st_mode)) {
                        Dmod_SnPrintf(cp_res_cmd, sizeof(cp_res_cmd), 
                                     "cp -r \"%s\" \"%s\"", full_source, res_entry.destination);
                    } else {
                        Dmod_SnPrintf(cp_res_cmd, sizeof(cp_res_cmd), 
                                     "cp \"%s\" \"%s\"", full_source, res_entry.destination);
                    }
                    
                    int cp_result = system(cp_res_cmd);
                    if (cp_result == 0) {
                        DMOD_LOG_INFO("  Resource installed successfully\n");
                    } else {
                        DMOD_LOG_ERROR("  Failed to install resource\n");
                    }
                }
                
                Dmod_Resource_Free(res_ctx);
            }
        }
    }
    
    // Clean up: remove ZIP file and temp directory
    char rm_zip_cmd[1024];
    Dmod_SnPrintf(rm_zip_cmd, sizeof(rm_zip_cmd), "rm -f \"%s\"", zip_path);
    system(rm_zip_cmd);
    
    char rm_temp_cmd[1024];
    Dmod_SnPrintf(rm_temp_cmd, sizeof(rm_temp_cmd), "rm -rf \"%s\"", extract_dir);
    system(rm_temp_cmd);
    
    DMOD_LOG_INFO("Module installed to: %s\n", output_file);
    return true;
}

/**
 * @brief Process and download dependencies for a module
 * 
 * @param module_file_path Path to the DMF/DMFC file
 * @param dmd_file_path Path to the .dmd file (can be NULL)
 * @param output_dir Output directory for dependencies
 * @param tools_name Tools name for substitution
 * @param arch_name Architecture name for substitution
 * @param cpu_name CPU name for substitution (can be NULL)
 * @param cpu_family CPU family for substitution (can be NULL)
 * @param preferred_type Preferred file type
 * @param default_manifest Default manifest URL
 * @param ignore_missing Whether to ignore missing dependencies
 * @param auto_accept_license If true, automatically accept license without prompting
 * @return 0 on success, non-zero on failure
 */
static int ProcessModuleDependencies(const char* module_file_path, const char* dmd_file_path,
                                      const char* output_dir, const char* tools_name,
                                      const char* arch_name, const char* cpu_name,
                                      const char* cpu_family, const char* preferred_type,
                                      const char* default_manifest, bool ignore_missing,
                                      bool skip_dmod_ver_check, bool mini_mode,
                                      bool auto_accept_license,
                                      InstallationCounts_t* counts) {
    int failed_count = 0;
    
    // First, try to load dependencies from .dmd file if provided
    if (dmd_file_path != NULL && Dmod_Access(dmd_file_path, DMOD_R_OK) == 0) {
        DMOD_LOG_INFO("Processing dependencies from: %s\n", dmd_file_path);
        
        // Initialize dependencies parser
        Dmod_DependenciesContext_t* dep_ctx = Dmod_Dependencies_Init(default_manifest, DownloadWithCurl, NULL);
        if (!dep_ctx) {
            DMOD_LOG_ERROR("Failed to initialize dependencies parser\n");
            return 1;
        }
        
        // Parse the .dmd file
        if (!Dmod_Dependencies_ParseFile(dep_ctx, dmd_file_path)) {
            DMOD_LOG_ERROR("Failed to parse dependencies file: %s\n", 
                    Dmod_Dependencies_GetError(dep_ctx));
            Dmod_Dependencies_Free(dep_ctx);
            return 1;
        }
        
        size_t dep_count = Dmod_Dependencies_GetEntryCount(dep_ctx);
        DMOD_LOG_INFO("Found %zu dependencies in .dmd file\n", dep_count);
        
        // Download each dependency
        for (size_t i = 0; i < dep_count; i++) {
            Dmod_DependencyEntry_t dep_entry;
            if (!Dmod_Dependencies_GetEntry(dep_ctx, i, &dep_entry)) {
                DMOD_LOG_ERROR("Failed to get dependency entry %zu\n", i);
                failed_count++;
                continue;
            }
            
            DMOD_LOG_INFO("  [%zu/%zu] %s%s%s\n", 
                   i + 1, dep_count,
                   dep_entry.name,
                   dep_entry.version[0] ? "@" : "",
                   dep_entry.version[0] ? dep_entry.version : "");
            
            // Initialize manifest parser for this dependency
            Dmod_ManifestContext_t* man_ctx = Dmod_Manifest_Init(tools_name, arch_name, cpu_name, cpu_family, DownloadWithCurl, NULL);
            if (!man_ctx) {
                DMOD_LOG_ERROR("    Failed to initialize manifest parser\n");
                failed_count++;
                continue;
            }
            
            // Parse manifest
            bool man_parse_success = false;
            if (strncmp(dep_entry.manifest, "http://", 7) == 0 || 
                strncmp(dep_entry.manifest, "https://", 8) == 0) {
                man_parse_success = Dmod_Manifest_ParseUrl(man_ctx, dep_entry.manifest);
            } else {
                man_parse_success = Dmod_Manifest_ParseFile(man_ctx, dep_entry.manifest);
            }
            
            if (!man_parse_success) {
                DMOD_LOG_ERROR("    Failed to parse manifest: %s\n", 
                        Dmod_Manifest_GetError(man_ctx));
                Dmod_Manifest_Free(man_ctx);
                failed_count++;
                continue;
            }
            
            // Download the dependency (with its own dependencies)
            int download_result = DownloadModule(
                dep_entry.name,
                dep_entry.version[0] ? dep_entry.version : NULL,
                man_ctx,
                output_dir,
                tools_name,
                arch_name,
                cpu_name,
                cpu_family,
                preferred_type,
                true,  // download dependencies recursively
                default_manifest,
                ignore_missing,
                skip_dmod_ver_check,
                mini_mode,
                auto_accept_license,
                counts
            );
            
            Dmod_Manifest_Free(man_ctx);
            
            if (download_result != 0) {
                failed_count++;
            }
        }
        
        Dmod_Dependencies_Free(dep_ctx);
    }
    // If no .dmd file, try to extract dependencies from the module itself
    else if (module_file_path != NULL) {
        DMOD_LOG_INFO("No .dmd file found, checking module for dependencies\n");
        
        // Initialize Dmod system if not already initialized
        static bool dmod_initialized = false;
        if (!dmod_initialized) {
            if (!Dmod_Initialize()) {
                DMOD_LOG_ERROR("Failed to initialize Dmod system\n");
                return 1;
            }
            dmod_initialized = true;
        }
        
        // Enable crossplatform mode as required
        Dmod_SetCrossplatformMode(true);
        
        // Try to read dependencies from the module file
        Dmod_RequiredModule_t required_modules[DMOD_MAX_REQUIRED_MODULES];
        memset(required_modules, 0, sizeof(required_modules));
        
        if (Dmod_ReadRequiredModules(module_file_path, required_modules, DMOD_MAX_REQUIRED_MODULES)) {
            DMOD_LOG_INFO("Reading dependencies from module file\n");
            
            // Count non-system dependencies
            int dep_count = 0;
            for (size_t i = 0; i < DMOD_MAX_REQUIRED_MODULES; i++) {
                if (required_modules[i].Name[0] != '\0' && !required_modules[i].SystemModule) {
                    dep_count++;
                }
            }
            
            if (dep_count > 0) {
                DMOD_LOG_INFO("Found %d non-system dependencies in module\n", dep_count);
                
                // Download each dependency
                int processed = 0;
                for (size_t i = 0; i < DMOD_MAX_REQUIRED_MODULES; i++) {
                    if (required_modules[i].Name[0] == '\0') continue;
                    if (required_modules[i].SystemModule) {
                        DMOD_LOG_INFO("  Skipping system module: %s\n", required_modules[i].Name);
                        continue;
                    }
                    
                    processed++;
                    DMOD_LOG_INFO("  [%d/%d] %s%s%s\n", 
                           processed, dep_count,
                           required_modules[i].Name,
                           required_modules[i].Version[0] ? "@" : "",
                           required_modules[i].Version[0] ? required_modules[i].Version : "");
                    
                    // Initialize manifest parser
                    Dmod_ManifestContext_t* man_ctx = Dmod_Manifest_Init(tools_name, arch_name, cpu_name, cpu_family, DownloadWithCurl, NULL);
                    if (!man_ctx) {
                        DMOD_LOG_ERROR("    Failed to initialize manifest parser\n");
                        failed_count++;
                        continue;
                    }
                    
                    // Parse default manifest
                    bool man_parse_success = false;
                    if (strncmp(default_manifest, "http://", 7) == 0 || 
                        strncmp(default_manifest, "https://", 8) == 0) {
                        man_parse_success = Dmod_Manifest_ParseUrl(man_ctx, default_manifest);
                    } else {
                        man_parse_success = Dmod_Manifest_ParseFile(man_ctx, default_manifest);
                    }
                    
                    if (!man_parse_success) {
                        DMOD_LOG_ERROR("    Failed to parse manifest: %s\n", 
                                Dmod_Manifest_GetError(man_ctx));
                        Dmod_Manifest_Free(man_ctx);
                        failed_count++;
                        continue;
                    }
                    
                    // Download the dependency (with its own dependencies)
                    int download_result = DownloadModule(
                        required_modules[i].Name,
                        required_modules[i].Version[0] ? required_modules[i].Version : NULL,
                        man_ctx,
                        output_dir,
                        tools_name,
                        arch_name,
                        cpu_name,
                        cpu_family,
                        preferred_type,
                        true,  // download dependencies recursively
                        default_manifest,
                        ignore_missing,
                        skip_dmod_ver_check,
                        mini_mode,
                        auto_accept_license,
                        counts
                    );
                    
                    Dmod_Manifest_Free(man_ctx);
                    
                    if (download_result != 0) {
                        failed_count++;
                    }
                }
            } else {
                DMOD_LOG_INFO("No non-system dependencies found in module\n");
            }
        } else {
            DMOD_LOG_INFO("Could not read dependencies from module (may not have any)\n");
        }
    }
    
    return failed_count;
}

/**
 * @brief Get environment variable or default value
 */
static const char* GetEnvOrDefault(const char* env_name, const char* default_value) {
    const char* value = Dmod_GetEnv(env_name);
    return value ? value : default_value;
}

/**
 * @brief Create directory if it doesn't exist
 */
static bool EnsureDirectory(const char* path) {
    // Check if directory exists by trying to access it
    if (Dmod_Access(path, DMOD_F_OK) == 0) {
        return true;  // Directory already exists
    }
    
    // Create directory
    if (Dmod_MakeDir(path, 0755) != 0) {
        DMOD_LOG_ERROR("Failed to create directory: %s\n", path);
        return false;
    }
    
    return true;
}

/**
 * @brief Find manifest file in default locations
 */
static char* FindManifest(const char* dmf_dir, const char* dmfc_dir) {
    char path[512];
    
    // Check dmf directory
    Dmod_SnPrintf(path, sizeof(path), "%s/%s", dmf_dir, DEFAULT_MANIFEST);
    if (Dmod_Access(path, DMOD_R_OK) == 0) {
        size_t len = strlen(path);
        char* result = Dmod_Malloc(len + 1);
        if (result) strcpy(result, path);
        return result;
    }
    
    // Check dmfc directory
    Dmod_SnPrintf(path, sizeof(path), "%s/%s", dmfc_dir, DEFAULT_MANIFEST);
    if (Dmod_Access(path, DMOD_R_OK) == 0) {
        size_t len = strlen(path);
        char* result = Dmod_Malloc(len + 1);
        if (result) strcpy(result, path);
        return result;
    }
    
    // Check current directory
    if (Dmod_Access(DEFAULT_MANIFEST, DMOD_R_OK) == 0) {
        size_t len = strlen(DEFAULT_MANIFEST);
        char* result = Dmod_Malloc(len + 1);
        if (result) strcpy(result, DEFAULT_MANIFEST);
        return result;
    }
    
    return NULL;
}

/**
 * @brief Print usage information
 */
static void PrintUsage(const char* app_name) {
    Dmod_Printf("Usage: %s [options] <command> [<module_name>[@version]]\n\n", app_name);
    Dmod_Printf("Commands:\n");
    Dmod_Printf("  install <module>          Download and install module (default command)\n");
    Dmod_Printf("  headers <module>          Extract module headers to output directory\n");
    Dmod_Printf("  docs <module>             Extract module documentation to output directory\n\n");
    Dmod_Printf("Options:\n");
    Dmod_Printf("  -d, --dependencies <path> Path or URL to dependencies (.dmd) file\n");
    Dmod_Printf("  -m, --manifest <path>     Path or URL to manifest file\n");
    Dmod_Printf("  -o, --output-dir <path>   Output directory for downloaded modules\n");
    Dmod_Printf("  -t, --tools-name <name>   Tools name for variable substitution\n");
    Dmod_Printf("  -a, --arch-name <name>    Architecture name for variable substitution\n");
    Dmod_Printf("  --cpu-name <name>         CPU name for variable substitution (e.g., stm32f746ngh6)\n");
    Dmod_Printf("  --cpu-family <name>       CPU family for variable substitution (e.g., stm32f7)\n");
    Dmod_Printf("  --type <dmf|dmfc>         Prefer dmf or dmfc file type\n");
    Dmod_Printf("  --no-dependencies         Don't download dependencies\n");
    Dmod_Printf("  --ignore-missing          Ignore missing dependencies and continue\n");
    Dmod_Printf("  --skip-arch-check         Skip architecture compatibility check\n");
    Dmod_Printf("  --skip-dmod-ver-check     Skip DMOD version compatibility check\n");
    Dmod_Printf("  --mini                    Install only dmf/dmfc files (skip other resources from .dmr)\n");
    Dmod_Printf("  -y, --yes                 Automatic yes to license prompts (non-interactive mode)\n");
    Dmod_Printf("  -h, --help                Show this help message\n");
    Dmod_Printf("  -v, --version             Show version information\n\n");
    Dmod_Printf("Environment Variables:\n");
    Dmod_Printf("  %s       Tools name (e.g., arch/x86_64)\n", ENV_TOOLS_NAME);
    Dmod_Printf("  %s          DMF output directory\n", ENV_DMF_DIR);
    Dmod_Printf("  %s         DMFC output directory\n", ENV_DMFC_DIR);
    Dmod_Printf("  %s      Default manifest path or URL\n", ENV_MANIFEST);
    Dmod_Printf("  %s       Include/headers output directory\n", ENV_INC_DIR);
    Dmod_Printf("  %s       Documentation output directory\n\n", ENV_DOC_DIR);
    Dmod_Printf("Examples:\n");
    Dmod_Printf("  %s mymodule              # Download latest version\n", app_name);
    Dmod_Printf("  %s install mymodule      # Same as above (install is explicit)\n", app_name);
    Dmod_Printf("  %s headers dmini -o ./dmini/inc  # Extract headers to specified path\n", app_name);
    Dmod_Printf("  %s headers dmini         # Extract headers to $DMOD_INC_DIR or $DMOD_DMF_DIR/dmini/inc\n", app_name);
    Dmod_Printf("  %s docs dmini -o ./dmini/docs    # Extract docs to specified path\n", app_name);
    Dmod_Printf("  %s docs dmini            # Extract docs to $DMOD_DOC_DIR or $DMOD_DMF_DIR/dmini/docs\n", app_name);
    Dmod_Printf("  %s mymodule@1.0          # Download specific version\n", app_name);
    Dmod_Printf("  %s mymodule@>=1.0        # Download version >= 1.0\n", app_name);
    Dmod_Printf("  %s mymodule@>=1.0<=2.0   # Download version in range [1.0, 2.0]\n", app_name);
    Dmod_Printf("  %s -d deps.dmd           # Download all modules from deps.dmd\n", app_name);
    Dmod_Printf("  %s -m http://... module  # Use custom manifest\n", app_name);
    Dmod_Printf("  %s --type dmfc module    # Prefer dmfc files\n", app_name);
    Dmod_Printf("  %s -a armv7-cortex-m7 module  # Use arch name directly\n", app_name);
    Dmod_Printf("  %s --cpu-name stm32f746ngh6 --cpu-family stm32f7 module  # Use CPU-specific module\n", app_name);
}

/**
 * @brief Print version information
 */
static void PrintVersion() {
    Dmod_Printf("dmf-get version " DMOD_VERSION_STRING "\n");
    Dmod_Printf("DMOD Package Manager\n");
}

/**
 * @brief Extract and install resource (headers or docs) from a module package
 * 
 * @param module_name Module name
 * @param module_version Module version (can be NULL)
 * @param manifest_ctx Manifest context
 * @param output_dir Output directory
 * @param tools_name Tools name for substitution
 * @param arch_name Architecture name for substitution
 * @param cpu_name CPU name for substitution (can be NULL)
 * @param cpu_family CPU family for substitution (can be NULL)
 * @param resource_key Resource key ("inc" for headers, "docs" for documentation)
 * @return 0 on success, non-zero on failure
 */
static int ExtractResourceCommand(const char* module_name, const char* module_version,
                                  Dmod_ManifestContext_t* manifest_ctx,
                                  const char* output_dir, const char* tools_name,
                                  const char* arch_name, const char* cpu_name,
                                  const char* cpu_family, const char* resource_key) {
    // Find module in manifest
    Dmod_ManifestEntry_t entry;
    Dmod_ManifestNode_t* node = Dmod_Manifest_FindEntry(manifest_ctx, module_name, module_version, NULL, &entry);
    if (!node) {
        DMOD_LOG_ERROR("Error: Module not found: %s\n", module_name);
        DMOD_LOG_ERROR("  %s\n", Dmod_Manifest_GetError(manifest_ctx));
        return 1;
    }
    
    // Determine which version to use for URL substitution
    const char* version_to_use = module_version ? module_version : 
                                (entry.version[0] ? entry.version : NULL);
    
    // Substitute <version> in URL if needed
    char final_url[1024];
    const char* url_ptr = entry.url;
    const char* version_placeholder = strstr(entry.url, "<version>");
    
    if (version_placeholder && !version_to_use) {
        version_to_use = "latest";
    }
    
    if (version_placeholder && version_to_use) {
        size_t prefix_len = version_placeholder - entry.url;
        const char* suffix = version_placeholder + strlen("<version>");
        Dmod_SnPrintf(final_url, sizeof(final_url), "%.*s%s%s",
                     (int)prefix_len, entry.url, version_to_use, suffix);
        url_ptr = final_url;
    }
    
    DMOD_LOG_INFO("Downloading package for %s from: %s\n", resource_key, url_ptr);
    
    // Download the package
    char* download_buffer = NULL;
    size_t download_size = 0;
    if (!DownloadWithCurl(url_ptr, &download_buffer, &download_size, NULL)) {
        DMOD_LOG_ERROR("Failed to download package\n");
        return 1;
    }
    
    // Validate module_name to prevent path traversal
    if (!IsPathSafe(module_name)) {
        DMOD_LOG_ERROR("Invalid module name (contains unsafe characters)\n");
        Dmod_Free(download_buffer);
        return 1;
    }
    
    // Save to temporary file
    char zip_path[512];
    Dmod_SnPrintf(zip_path, sizeof(zip_path), "/tmp/dmod_resource_%s_%d.zip", module_name, (int)getpid());
    
    FILE* zip_file = fopen(zip_path, "wb");
    if (!zip_file) {
        DMOD_LOG_ERROR("Failed to create temporary file: %s\n", zip_path);
        Dmod_Free(download_buffer);
        return 1;
    }
    
    size_t written = fwrite(download_buffer, 1, download_size, zip_file);
    fclose(zip_file);
    
    if (written != download_size) {
        DMOD_LOG_ERROR("Failed to write complete package to temporary file\n");
        Dmod_Free(download_buffer);
        // Clean up incomplete file
        char rm_cmd[1024];
        Dmod_SnPrintf(rm_cmd, sizeof(rm_cmd), "rm -f \"%s\"", zip_path);
        system(rm_cmd);
        return 1;
    }
    
    Dmod_Free(download_buffer);
    
    DMOD_LOG_INFO("Package downloaded, extracting %s...\n", resource_key);
    
    // Extract the resource
    bool success = ExtractResourceFromZip(zip_path, output_dir, module_name, resource_key);
    
    // Clean up zip file
    char rm_cmd[1024];
    Dmod_SnPrintf(rm_cmd, sizeof(rm_cmd), "rm -f \"%s\"", zip_path);
    system(rm_cmd);
    
    if (!success) {
        DMOD_LOG_ERROR("Failed to extract %s from package\n", resource_key);
        return 1;
    }
    
    DMOD_LOG_INFO("Successfully extracted %s to: %s\n", resource_key, output_dir);
    return 0;
}

/**
 * @brief Download a single module
 * 
 * @param module_name Module name
 * @param module_version Module version (can be NULL)
 * @param manifest_ctx Manifest context
 * @param output_dir Output directory
 * @param tools_name Tools name for substitution
 * @param arch_name Architecture name for substitution
 * @param cpu_name CPU name for substitution (can be NULL)
 * @param cpu_family CPU family for substitution (can be NULL)
 * @param preferred_type Preferred file type (dmf or dmfc)
 * @param download_dependencies Whether to download dependencies
 * @param default_manifest Default manifest URL
 * @param ignore_missing Whether to ignore missing modules (continue on arch mismatch)
 * @param mini_mode If true, only install dmf/dmfc files (ignore other resources from .dmr)
 * @param auto_accept_license If true, automatically accept license without prompting
 * @param counts Pointer to installation counts (can be NULL)
 * @return 0 on success, non-zero on failure
 */
static int DownloadModule(const char* module_name, const char* module_version,
                          Dmod_ManifestContext_t* manifest_ctx, 
                          const char* output_dir, const char* tools_name,
                          const char* arch_name, const char* cpu_name,
                          const char* cpu_family, const char* preferred_type,
                          bool download_dependencies, const char* default_manifest,
                          bool ignore_missing, bool skip_dmod_ver_check,
                          bool mini_mode, bool auto_accept_license,
                          InstallationCounts_t* counts) {
    // Try to find entries for this module, checking architecture for each
    Dmod_ManifestNode_t* last_node = NULL;
    Dmod_ManifestEntry_t entry;
    bool found_any = false;
    
    // Get current DMOD version for compatibility checking
    Dmod_SemanticVersion_t current_dmod_version;
    GetCurrentDmodVersion(&current_dmod_version);
    
    while (true) {
        // Find next entry matching the module name
        Dmod_ManifestNode_t* current_node = Dmod_Manifest_FindEntry(manifest_ctx, module_name, module_version, last_node, &entry);
        if (!current_node) {
            // No more entries found
            if (!found_any) {
                DMOD_LOG_ERROR("Error: %s\n", Dmod_Manifest_GetError(manifest_ctx));
                return ignore_missing ? 0 : 1;
            } else {
                // We tried all entries but none matched architecture
                DMOD_LOG_ERROR("Error: No compatible architecture found for module: %s\n", module_name);
                return ignore_missing ? 0 : 1;
            }
        }
        
        found_any = true;
        last_node = current_node;
        
        // Check DMOD version compatibility
        if (!skip_dmod_ver_check) {
            if (!Dmod_Manifest_IsEntryCompatible(&entry, &current_dmod_version)) {
                char entry_ver_str[32];
                char current_ver_str[32];
                Dmod_Version_ToString(&entry.dmod_version, entry_ver_str, sizeof(entry_ver_str));
                Dmod_Version_ToString(&current_dmod_version, current_ver_str, sizeof(current_ver_str));
                
                DMOD_LOG_WARN("Skipping entry for %s: requires DMOD %s but current is %s (incompatible major version)\n",
                             module_name, entry_ver_str, current_ver_str);
                continue; // Try next entry
            }
        }
    
        // Determine which version to use for URL substitution
        const char* version_to_use = module_version ? module_version : 
                                    (entry.version[0] ? entry.version : NULL);
        
        // Substitute <version> in URL if needed
        char final_url[1024];
        const char* url_ptr = entry.url;
        const char* version_placeholder = strstr(entry.url, "<version>");
        
        // Check if version is required but not provided
        if (version_placeholder && !version_to_use) {
            // Use "latest" as default version when required
            version_to_use = "latest";
            DMOD_LOG_INFO("Version required but not specified, using 'latest'\n");
        }
        
        if (version_placeholder && version_to_use) {
            // Need to substitute <version>
            char* dst = final_url;
            const char* src = entry.url;
            char* dst_end = final_url + sizeof(final_url) - 1;
            
            while (*src && dst < dst_end) {
                if (strncmp(src, "<version>", 9) == 0) {
                    size_t len = strlen(version_to_use);
                    if (dst + len >= dst_end) break;
                    strcpy(dst, version_to_use);
                    dst += len;
                    src += 9;
                } else {
                    *dst++ = *src++;
                }
            }
            *dst = '\0';
            url_ptr = final_url;
        }
        
        DMOD_LOG_INFO("Found: %s%s%s at %s\n", 
            entry.name,
            version_to_use ? "@" : "",
            version_to_use ? version_to_use : "",
            url_ptr);
        
        // Determine output file name and extension
        const char* url = url_ptr;
        const char* ext = strrchr(url, '.');
        char output_file[512];
        
        if (ext && (strcmp(ext, ".dmf") == 0 || strcmp(ext, ".dmfc") == 0 || 
                    strcmp(ext, ".zip") == 0 || strcmp(ext, ".dmp") == 0)) {
            Dmod_SnPrintf(output_file, sizeof(output_file), "%s/%s%s%s%s",
                    output_dir, entry.name,
                    version_to_use ? "-" : "",
                    version_to_use ? version_to_use : "",
                    ext);
        } else {
            // Default to .dmf if no extension
            Dmod_SnPrintf(output_file, sizeof(output_file), "%s/%s%s%s.dmf",
                    output_dir, entry.name,
                    version_to_use ? "-" : "",
                    version_to_use ? version_to_use : "");
        }
        
        // Check if file already exists
        bool already_exists = (Dmod_Access(output_file, DMOD_F_OK) == 0);
        if (already_exists) {
            DMOD_LOG_INFO("File already exists, skipping download: %s\n", output_file);
        } else {
            // Download the file
            if (!DownloadFile(url, output_file)) {
                DMOD_LOG_ERROR("Error: Failed to download module\n");
                return 1;
            }
        }
        
        char final_module_path[512];
        char dmd_file_path[512] = "";
        
        // If it's a ZIP file, extract it and find the DMF/DMFC file
        if (ext && strcmp(ext, ".zip") == 0) {
            if (!ExtractZipAndFindModule(output_file, output_dir, entry.name, preferred_type,
                                        mini_mode, auto_accept_license,
                                        final_module_path, sizeof(final_module_path),
                                        dmd_file_path, sizeof(dmd_file_path))) {
                DMOD_LOG_ERROR("Error: Failed to extract and find module from ZIP\n");
                return 1;
            }
            // ExtractZipAndFindModule already prints where it was installed
        } else {
            DMOD_LOG_INFO("Module installed to: %s\n", output_file);
            strncpy(final_module_path, output_file, sizeof(final_module_path) - 1);
            final_module_path[sizeof(final_module_path) - 1] = '\0';
        }
        
        // Verify architecture matches expected architecture
        if (!already_exists && arch_name != NULL && arch_name[0] != '\0') {
            char package_arch[DMOD_MAX_ARCH_NAME_LENGTH];
            bool arch_read = Dmod_GetFileArchitecture(final_module_path, package_arch, sizeof(package_arch));
            bool arch_valid = arch_read && strcmp(package_arch, arch_name) == 0;
            bool can_use = arch_valid;
            if (!can_use) {
                if(!arch_read) {
                    DMOD_LOG_WARN("Could not read architecture from module file: %s\n", final_module_path);
                }
                else if(!arch_valid) {
                    DMOD_LOG_WARN("Module architecture '%s' does not match expected '%s'\n", 
                        package_arch, arch_name);
                }
                DMOD_LOG_INFO("Removing incompatible module file and continuing search: %s\n", final_module_path);

                // Delete the incompatible file
                remove(final_module_path);
                
                // Also remove .dmd file if it exists
                if (dmd_file_path[0] != '\0') {
                    remove(dmd_file_path);
                }
                continue;  // Try next entry in manifest
            } 
            DMOD_LOG_VERBOSE("Module architecture '%s' matches expected '%s'\n", 
                package_arch, arch_name);
        }
        
        // Architecture matches or check not required - process dependencies and return success
        if (download_dependencies && !already_exists) {
            DMOD_LOG_INFO("Processing dependencies for %s\n", entry.name);
            int dep_result = ProcessModuleDependencies(
                final_module_path,
                dmd_file_path[0] != '\0' ? dmd_file_path : NULL,
                output_dir,
                tools_name,
                arch_name,
                cpu_name,
                cpu_family,
                preferred_type,
                default_manifest,
                ignore_missing,
                skip_dmod_ver_check,
                mini_mode,
                auto_accept_license,
                counts
            );
            
            if (dep_result > 0) {
                DMOD_LOG_WARN("%d dependencies failed to download\n", dep_result);
            }
        }
        
        // Print installed package and update counts (only if not already existed)
        if (!already_exists && counts) {
            Dmod_Printf("Installed: %s", entry.name);
            if (version_to_use && version_to_use[0]) {
                Dmod_Printf("@%s", version_to_use);
            }
            Dmod_Printf("\n");
            counts->success_count++;
        }
        
        return 0;  // Success - found and installed compatible module
    } // end while loop
    return -1;  // Should not reach here
}

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
    // Parse command line arguments
    const char* module_spec = NULL;
    const char* dependencies_path = NULL;
    const char* manifest_path = NULL;
    const char* output_dir = NULL;
    const char* tools_name = NULL;
    const char* arch_name = NULL;
    const char* cpu_name = NULL;
    const char* cpu_family = NULL;
    const char* preferred_type = NULL;
    const char* command = NULL;  // Command: install (default), headers, or docs
    bool no_dependencies = false;
    bool ignore_missing = false;
    bool skip_arch_check = false;
    bool skip_dmod_ver_check = false;
    bool mini_mode = false;
    bool auto_accept_license = false;  // -y flag for automatic license acceptance
    
    Dmod_SetLogLevel(Dmod_LogLevel_Info);
    Dmod_SetCrossplatformMode(true);
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            PrintUsage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            PrintVersion();
            return 0;
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dependencies") == 0) {
            if (++i >= argc) {
                DMOD_LOG_ERROR("Error: %s requires an argument\n", argv[i-1]);
                return 1;
            }
            dependencies_path = argv[i];
        }
        else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--manifest") == 0) {
            if (++i >= argc) {
                DMOD_LOG_ERROR("Error: %s requires an argument\n", argv[i-1]);
                return 1;
            }
            manifest_path = argv[i];
        }
        else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output-dir") == 0) {
            if (++i >= argc) {
                DMOD_LOG_ERROR("Error: %s requires an argument\n", argv[i-1]);
                return 1;
            }
            output_dir = argv[i];
        }
        else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--tools-name") == 0) {
            if (++i >= argc) {
                DMOD_LOG_ERROR("Error: %s requires an argument\n", argv[i-1]);
                return 1;
            }
            tools_name = argv[i];
        }
        else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--arch-name") == 0) {
            if (++i >= argc) {
                DMOD_LOG_ERROR("Error: %s requires an argument\n", argv[i-1]);
                return 1;
            }
            arch_name = argv[i];
        }
        else if (strcmp(argv[i], "--cpu-name") == 0) {
            if (++i >= argc) {
                DMOD_LOG_ERROR("Error: %s requires an argument\n", argv[i-1]);
                return 1;
            }
            cpu_name = argv[i];
        }
        else if (strcmp(argv[i], "--cpu-family") == 0) {
            if (++i >= argc) {
                DMOD_LOG_ERROR("Error: %s requires an argument\n", argv[i-1]);
                return 1;
            }
            cpu_family = argv[i];
        }
        else if (strcmp(argv[i], "--type") == 0) {
            if (++i >= argc) {
                DMOD_LOG_ERROR("Error: %s requires an argument\n", argv[i-1]);
                return 1;
            }
            if (strcmp(argv[i], "dmf") != 0 && strcmp(argv[i], "dmfc") != 0) {
                DMOD_LOG_ERROR("Error: --type must be either 'dmf' or 'dmfc'\n");
                return 1;
            }
            preferred_type = argv[i];
        }
        else if (strcmp(argv[i], "--no-dependencies") == 0) {
            no_dependencies = true;
        }
        else if (strcmp(argv[i], "--ignore-missing") == 0) {
            ignore_missing = true;
        }
        else if (strcmp(argv[i], "--skip-arch-check") == 0) {
            skip_arch_check = true;
        }
        else if (strcmp(argv[i], "--skip-dmod-ver-check") == 0) {
            skip_dmod_ver_check = true;
        }
        else if (strcmp(argv[i], "--mini") == 0) {
            mini_mode = true;
        }
        else if (strcmp(argv[i], "-y") == 0 || strcmp(argv[i], "--yes") == 0) {
            auto_accept_license = true;
        }
        else if (strcmp(argv[i], "--verbose") == 0) {
            Dmod_SetLogLevel(Dmod_LogLevel_Verbose);
        }
        else if (argv[i][0] == '-') {
            DMOD_LOG_ERROR("Error: Unknown option: %s\n", argv[i]);
            PrintUsage(argv[0]);
            return 1;
        }
        else {
            // Handle subcommands: install, headers, docs
            // Check if this is a command keyword
            if (!command && !module_spec) {
                if (strcmp(argv[i], "install") == 0 || 
                    strcmp(argv[i], "headers") == 0 || 
                    strcmp(argv[i], "docs") == 0) {
                    // Check if there's another positional argument after this command
                    bool has_more_positional = false;
                    for (int j = i + 1; j < argc; j++) {
                        if (argv[j][0] != '-') {
                            has_more_positional = true;
                            break;
                        }
                    }
                    if (has_more_positional) {
                        command = argv[i];
                        continue;  // Skip the command keyword
                    }
                    // If no more positional args, treat as module name
                }
            }
            
            if (module_spec) {
                DMOD_LOG_ERROR("Error: Multiple module names specified\n");
                return 1;
            }
            module_spec = argv[i];
        }
    }
    
    if (!module_spec && !dependencies_path) {
        DMOD_LOG_ERROR("Error: No module name or dependencies file specified\n");
        PrintUsage(argv[0]);
        return 1;
    }
    
    if (module_spec && dependencies_path) {
        DMOD_LOG_ERROR("Error: Cannot specify both module name and dependencies file\n");
        PrintUsage(argv[0]);
        return 1;
    }
    
    // Initialize curl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    // Handle headers and docs commands
    if (command && (strcmp(command, "headers") == 0 || strcmp(command, "docs") == 0)) {
        if (!module_spec) {
            DMOD_LOG_ERROR("Error: No module name specified for %s command\n", command);
            PrintUsage(argv[0]);
            curl_global_cleanup();
            return 1;
        }
        
        if (dependencies_path) {
            DMOD_LOG_ERROR("Error: Cannot use %s command with dependencies file\n", command);
            curl_global_cleanup();
            return 1;
        }
        
        // Parse module name and version first
        char module_name[256];
        char module_version[128] = "";
        strncpy(module_name, module_spec, sizeof(module_name) - 1);
        module_name[sizeof(module_name) - 1] = '\0';
        
        char* version_sep = strchr(module_name, '@');
        if (version_sep) {
            *version_sep = '\0';
            strncpy(module_version, version_sep + 1, sizeof(module_version) - 1);
            module_version[sizeof(module_version) - 1] = '\0';
        }
        
        // Determine output directory based on command
        const char* resource_key;
        char default_output_dir[1024];
        if (strcmp(command, "headers") == 0) {
            resource_key = "inc";
            if (!output_dir) {
                output_dir = Dmod_GetEnv(ENV_INC_DIR);
                if (!output_dir) {
                    // Default to $DMOD_DMF_DIR/<module_name>/inc
                    const char* dmf_dir = GetEnvOrDefault(ENV_DMF_DIR, DEFAULT_DMF_DIR);
                    Dmod_SnPrintf(default_output_dir, sizeof(default_output_dir), "%s/%s/inc", dmf_dir, module_name);
                    output_dir = default_output_dir;
                    DMOD_LOG_INFO("No output directory specified, using default: %s\n", output_dir);
                }
            }
        } else { // docs
            resource_key = "docs";
            if (!output_dir) {
                output_dir = Dmod_GetEnv(ENV_DOC_DIR);
                if (!output_dir) {
                    // Default to $DMOD_DMF_DIR/<module_name>/docs
                    const char* dmf_dir = GetEnvOrDefault(ENV_DMF_DIR, DEFAULT_DMF_DIR);
                    Dmod_SnPrintf(default_output_dir, sizeof(default_output_dir), "%s/%s/docs", dmf_dir, module_name);
                    output_dir = default_output_dir;
                    DMOD_LOG_INFO("No output directory specified, using default: %s\n", output_dir);
                }
            }
        }
        
        // Get manifest path
        if (!manifest_path) {
            manifest_path = Dmod_GetEnv(ENV_MANIFEST);
            if (!manifest_path) {
                const char* dmf_dir = GetEnvOrDefault(ENV_DMF_DIR, DEFAULT_DMF_DIR);
                const char* dmfc_dir = GetEnvOrDefault(ENV_DMFC_DIR, DEFAULT_DMFC_DIR);
                
                char* found_manifest = FindManifest(dmf_dir, dmfc_dir);
                if (found_manifest) {
                    manifest_path = found_manifest;
                    DMOD_LOG_INFO("Using manifest: %s\n", manifest_path);
                } else {
                    manifest_path = DEFAULT_MANIFEST_URL;
                    DMOD_LOG_INFO("Using default manifest: %s\n", manifest_path);
                }
            }
        }
        
        // Get configuration
        if (!tools_name) {
            tools_name = GetEnvOrDefault(ENV_TOOLS_NAME, "arch/x86_64");
        }
        
        char arch_buffer[DMOD_MAX_ARCH_NAME_LENGTH];
        if (tools_name && !arch_name) {
            strcpy(arch_buffer, tools_name);
            char* start = arch_buffer;
            if (strncmp(start, "arch/", 5) == 0) {
                start += 5;
            }
            for (char* p = start; *p; p++) {
                if (*p == '/') {
                    *p = '-';
                }
            }
            arch_name = start;
        }
        
        // Initialize manifest parser
        Dmod_ManifestContext_t* manifest_ctx = Dmod_Manifest_Init(tools_name, arch_name, cpu_name, cpu_family, DownloadWithCurl, NULL);
        if (!manifest_ctx) {
            DMOD_LOG_ERROR("Error: Failed to initialize manifest parser\n");
            curl_global_cleanup();
            return 1;
        }
        
        // Parse manifest
        bool manifest_parse_success = false;
        if (strncmp(manifest_path, "http://", 7) == 0 || 
            strncmp(manifest_path, "https://", 8) == 0) {
            manifest_parse_success = Dmod_Manifest_ParseUrl(manifest_ctx, manifest_path);
        } else {
            manifest_parse_success = Dmod_Manifest_ParseFile(manifest_ctx, manifest_path);
        }
        
        if (!manifest_parse_success) {
            DMOD_LOG_ERROR("Error: Failed to parse manifest: %s\n", 
                    Dmod_Manifest_GetError(manifest_ctx));
            Dmod_Manifest_Free(manifest_ctx);
            curl_global_cleanup();
            return 1;
        }
        
        // Extract the resource
        int result = ExtractResourceCommand(
            module_name,
            module_version[0] ? module_version : NULL,
            manifest_ctx,
            output_dir,
            tools_name,
            arch_name,
            cpu_name,
            cpu_family,
            resource_key
        );
        
        Dmod_Manifest_Free(manifest_ctx);
        curl_global_cleanup();
        return result;
    }
    
    // If arch_name not specified, default to system architecture
    if (!arch_name && !skip_arch_check && !tools_name) {
        arch_name = DMOD_ARCH;
    }

    // Get configuration
    if (!tools_name) {
        tools_name = GetEnvOrDefault(ENV_TOOLS_NAME, "arch/x86_64");
    }

    char arch_buffer[DMOD_MAX_ARCH_NAME_LENGTH];
    if(tools_name && !arch_name && !skip_arch_check) {
        strcpy(arch_buffer, tools_name);
        char* start = arch_buffer;
        if (strncmp(start, "arch/", 5) == 0) {
            start += 5;
        }
        // replace any remaining '/' with '-'
        for(char* p = start; *p; p++) {
            if(*p == '/') {
                *p = '-';
            }
        }
        arch_name = start;
    }
    
    if (!output_dir) {
        output_dir = GetEnvOrDefault(ENV_DMF_DIR, DEFAULT_DMF_DIR);
    }
    
    // Ensure output directory exists
    if (!EnsureDirectory(output_dir)) {
        curl_global_cleanup();
        return 1;
    }
    
    // Get manifest path
    if (!manifest_path) {
        manifest_path = Dmod_GetEnv(ENV_MANIFEST);
        if (!manifest_path) {
            const char* dmf_dir = GetEnvOrDefault(ENV_DMF_DIR, DEFAULT_DMF_DIR);
            const char* dmfc_dir = GetEnvOrDefault(ENV_DMFC_DIR, DEFAULT_DMFC_DIR);
            
            char* found_manifest = FindManifest(dmf_dir, dmfc_dir);
            if (found_manifest) {
                manifest_path = found_manifest;
                DMOD_LOG_INFO("Using manifest: %s\n", manifest_path);
            } else {
                // Use default manifest URL from dmod-registry
                manifest_path = DEFAULT_MANIFEST_URL;
                DMOD_LOG_INFO("Using default manifest: %s\n", manifest_path);
            }
        }
    }
    
    int result = 0;
    
    // Handle dependencies file if provided
    if (dependencies_path) {
        DMOD_LOG_INFO("Loading dependencies from: %s\n", dependencies_path);
        
        // Initialize dependencies parser
        Dmod_DependenciesContext_t* dep_ctx = Dmod_Dependencies_Init(manifest_path, DownloadWithCurl, NULL);
        if (!dep_ctx) {
            DMOD_LOG_ERROR("Error: Failed to initialize dependencies parser\n");
            curl_global_cleanup();
            return 1;
        }
        
        // Parse dependencies file
        bool dep_parse_success = false;
        if (strncmp(dependencies_path, "http://", 7) == 0 || 
            strncmp(dependencies_path, "https://", 8) == 0) {
            dep_parse_success = Dmod_Dependencies_ParseUrl(dep_ctx, dependencies_path);
        } else {
            dep_parse_success = Dmod_Dependencies_ParseFile(dep_ctx, dependencies_path);
        }
        
        if (!dep_parse_success) {
            DMOD_LOG_ERROR("Error: Failed to parse dependencies: %s\n", 
                    Dmod_Dependencies_GetError(dep_ctx));
            Dmod_Dependencies_Free(dep_ctx);
            curl_global_cleanup();
            return 1;
        }
        
        size_t dep_count = Dmod_Dependencies_GetEntryCount(dep_ctx);
        DMOD_LOG_INFO("Dependencies loaded with %zu modules\n", dep_count);
        
        // Initialize installation counts
        InstallationCounts_t counts = {0, 0};
        
        for (size_t i = 0; i < dep_count; i++) {
            Dmod_DependencyEntry_t dep_entry;
            if (!Dmod_Dependencies_GetEntry(dep_ctx, i, &dep_entry)) {
                DMOD_LOG_ERROR("Error: Failed to get dependency entry %zu\n", i);
                counts.failed_count++;
                continue;
            }
            
            DMOD_LOG_INFO("\n[%zu/%zu] Downloading module: %s%s%s\n", 
                   i + 1, dep_count,
                   dep_entry.name,
                   dep_entry.version[0] ? "@" : "",
                   dep_entry.version[0] ? dep_entry.version : "");
            DMOD_LOG_INFO("  Using manifest: %s\n", dep_entry.manifest);
            
            // Initialize manifest parser for this module
            Dmod_ManifestContext_t* man_ctx = Dmod_Manifest_Init(tools_name, arch_name, cpu_name, cpu_family, DownloadWithCurl, NULL);
            if (!man_ctx) {
                DMOD_LOG_ERROR("  Error: Failed to initialize manifest parser\n");
                counts.failed_count++;
                continue;
            }
            
            // Parse manifest
            bool man_parse_success = false;
            if (strncmp(dep_entry.manifest, "http://", 7) == 0 || 
                strncmp(dep_entry.manifest, "https://", 8) == 0) {
                man_parse_success = Dmod_Manifest_ParseUrl(man_ctx, dep_entry.manifest);
            } else {
                man_parse_success = Dmod_Manifest_ParseFile(man_ctx, dep_entry.manifest);
            }
            
            if (!man_parse_success) {
                DMOD_LOG_ERROR("  Error: Failed to parse manifest: %s\n", 
                        Dmod_Manifest_GetError(man_ctx));
                Dmod_Manifest_Free(man_ctx);
                counts.failed_count++;
                continue;
            }
            
            // Download the module (with dependencies)
            int download_result = DownloadModule(
                dep_entry.name,
                dep_entry.version[0] ? dep_entry.version : NULL,
                man_ctx,
                output_dir,
                tools_name,
                arch_name,
                cpu_name,
                cpu_family,
                preferred_type,
                true,  // download dependencies
                manifest_path,
                ignore_missing,
                skip_dmod_ver_check,
                mini_mode,
                auto_accept_license,
                &counts
            );
            
            Dmod_Manifest_Free(man_ctx);
            
            if (download_result != 0) {
                counts.failed_count++;
            }
        }
        
        // Print installation summary
        Dmod_Printf("\n");
        Dmod_Printf("=== Installation Summary ===\n");
        Dmod_Printf("Successfully installed: %d package%s\n", 
                   counts.success_count, counts.success_count == 1 ? "" : "s");
        if (counts.failed_count > 0) {
            Dmod_Printf("Failed: %d package%s\n", 
                       counts.failed_count, counts.failed_count == 1 ? "" : "s");
        }
        Dmod_Printf("\n");
        
        Dmod_Dependencies_Free(dep_ctx);
        
        result = (counts.failed_count > 0) ? 1 : 0;
    }
    // Handle single module download
    else {
        // Parse module specification
        char module_name[DMOD_MANIFEST_MAX_NAME_LEN];
        char module_version[DMOD_MANIFEST_MAX_VERSION_LEN] = {0};
        
        const char* at_sign = strchr(module_spec, '@');
        if (at_sign) {
            size_t name_len = at_sign - module_spec;
            if (name_len >= sizeof(module_name)) {
                DMOD_LOG_ERROR("Error: Module name too long\n");
                curl_global_cleanup();
                return 1;
            }
            strncpy(module_name, module_spec, name_len);
            module_name[name_len] = '\0';
            strncpy(module_version, at_sign + 1, sizeof(module_version) - 1);
        } else {
            strncpy(module_name, module_spec, sizeof(module_name) - 1);
            module_name[sizeof(module_name) - 1] = '\0';
        }
        
        // Initialize manifest parser
        Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init(tools_name, arch_name, cpu_name, cpu_family, DownloadWithCurl, NULL);
        if (!ctx) {
            DMOD_LOG_ERROR("Error: Failed to initialize manifest parser\n");
            curl_global_cleanup();
            return 1;
        }
        
        // Parse manifest
        DMOD_LOG_INFO("Parsing manifest: %s\n", manifest_path);
        bool parse_success = false;
        
        if (strncmp(manifest_path, "http://", 7) == 0 || 
            strncmp(manifest_path, "https://", 8) == 0) {
            parse_success = Dmod_Manifest_ParseUrl(ctx, manifest_path);
        } else {
            parse_success = Dmod_Manifest_ParseFile(ctx, manifest_path);
        }
        
        if (!parse_success) {
            DMOD_LOG_ERROR("Error: Failed to parse manifest: %s\n", 
                    Dmod_Manifest_GetError(ctx));
            Dmod_Manifest_Free(ctx);
            curl_global_cleanup();
            return 1;
        }
        
        DMOD_LOG_INFO("Manifest loaded with %zu entries\n", Dmod_Manifest_GetEntryCount(ctx));
        
        // Initialize installation counts
        InstallationCounts_t counts = {0, 0};
        
        // Download the module (with or without dependencies based on flag)
        result = DownloadModule(
            module_name,
            module_version[0] ? module_version : NULL,
            ctx,
            output_dir,
            tools_name,
            arch_name,
            cpu_name,
            cpu_family,
            preferred_type,
            !no_dependencies,  // download dependencies unless --no-dependencies is set
            manifest_path,
            ignore_missing,
            skip_dmod_ver_check,
            mini_mode,
            auto_accept_license,
            &counts
        );
        
        if (result != 0) {
            counts.failed_count++;
        }
        
        // Print installation summary
        Dmod_Printf("\n");
        Dmod_Printf("=== Installation Summary ===\n");
        Dmod_Printf("Successfully installed: %d package%s\n", 
                   counts.success_count, counts.success_count == 1 ? "" : "s");
        if (counts.failed_count > 0) {
            Dmod_Printf("Failed: %d package%s\n", 
                       counts.failed_count, counts.failed_count == 1 ? "" : "s");
        }
        Dmod_Printf("\n");
        
        // Cleanup
        Dmod_Manifest_Free(ctx);
    }
    
    // Cleanup
    curl_global_cleanup();
    
    return result;
}
