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

// Default paths
#define DEFAULT_DMF_DIR "./dmf"
#define DEFAULT_DMFC_DIR "./dmfc"
#define DEFAULT_MANIFEST "manifest.dmm"

// Environment variables
#define ENV_TOOLS_NAME "DMOD_TOOLS_NAME"
#define ENV_DMF_DIR "DMOD_DMF_DIR"
#define ENV_DMFC_DIR "DMOD_DMFC_DIR"
#define ENV_MANIFEST "DMOD_MANIFEST"

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
    
    char* new_data = realloc(buffer->data, buffer->size + realsize + 1);
    if (!new_data) {
        fprintf(stderr, "Out of memory\n");
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
        fprintf(stderr, "Failed to initialize curl\n");
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
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(download.data);
        curl_easy_cleanup(curl);
        return false;
    }
    
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    curl_easy_cleanup(curl);
    
    if (response_code != 200) {
        fprintf(stderr, "HTTP error %ld for URL: %s\n", response_code, url);
        free(download.data);
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
    printf("Downloading: %s\n", url);
    
    char* buffer = NULL;
    size_t size = 0;
    
    if (!DownloadWithCurl(url, &buffer, &size, NULL)) {
        return false;
    }
    
    FILE* file = fopen(output_path, "wb");
    if (!file) {
        fprintf(stderr, "Cannot create file: %s\n", output_path);
        free(buffer);
        return false;
    }
    
    size_t written = fwrite(buffer, 1, size, file);
    fclose(file);
    free(buffer);
    
    if (written != size) {
        fprintf(stderr, "Failed to write complete file: %s\n", output_path);
        return false;
    }
    
    printf("Downloaded: %s (%zu bytes)\n", output_path, size);
    return true;
}

/**
 * @brief Get environment variable or default value
 */
static const char* GetEnvOrDefault(const char* env_name, const char* default_value) {
    const char* value = getenv(env_name);
    return value ? value : default_value;
}

/**
 * @brief Create directory if it doesn't exist
 */
static bool EnsureDirectory(const char* path) {
    struct stat st = {0};
    
    if (stat(path, &st) == -1) {
        #ifdef _WIN32
        if (mkdir(path) != 0) {
        #else
        if (mkdir(path, 0755) != 0) {
        #endif
            fprintf(stderr, "Failed to create directory: %s\n", path);
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Find manifest file in default locations
 */
static char* FindManifest(const char* dmf_dir, const char* dmfc_dir) {
    char path[512];
    
    // Check dmf directory
    snprintf(path, sizeof(path), "%s/%s", dmf_dir, DEFAULT_MANIFEST);
    if (access(path, R_OK) == 0) {
        return strdup(path);
    }
    
    // Check dmfc directory
    snprintf(path, sizeof(path), "%s/%s", dmfc_dir, DEFAULT_MANIFEST);
    if (access(path, R_OK) == 0) {
        return strdup(path);
    }
    
    // Check current directory
    if (access(DEFAULT_MANIFEST, R_OK) == 0) {
        return strdup(DEFAULT_MANIFEST);
    }
    
    return NULL;
}

/**
 * @brief Print usage information
 */
static void PrintUsage(const char* app_name) {
    printf("Usage: %s [options] <module_name>[@version]\n\n", app_name);
    printf("Options:\n");
    printf("  -m, --manifest <path>     Path or URL to manifest file\n");
    printf("  -o, --output-dir <path>   Output directory for downloaded modules\n");
    printf("  -t, --tools-name <name>   Tools name for variable substitution\n");
    printf("  --no-dependencies         Don't download dependencies\n");
    printf("  -h, --help                Show this help message\n");
    printf("  -v, --version             Show version information\n\n");
    printf("Environment Variables:\n");
    printf("  %s       Tools name (e.g., arch/x86_64)\n", ENV_TOOLS_NAME);
    printf("  %s          DMF output directory\n", ENV_DMF_DIR);
    printf("  %s         DMFC output directory\n", ENV_DMFC_DIR);
    printf("  %s      Default manifest path or URL\n\n", ENV_MANIFEST);
    printf("Examples:\n");
    printf("  %s mymodule              # Download latest version\n", app_name);
    printf("  %s mymodule@1.0          # Download specific version\n", app_name);
    printf("  %s -m http://... module  # Use custom manifest\n", app_name);
}

/**
 * @brief Print version information
 */
static void PrintVersion() {
    printf("dmf-get version " DMOD_VERSION_STRING "\n");
    printf("DMOD Package Manager\n");
}

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
    // Parse command line arguments
    const char* module_spec = NULL;
    const char* manifest_path = NULL;
    const char* output_dir = NULL;
    const char* tools_name = NULL;
    bool no_dependencies = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            PrintUsage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            PrintVersion();
            return 0;
        }
        else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--manifest") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "Error: %s requires an argument\n", argv[i-1]);
                return 1;
            }
            manifest_path = argv[i];
        }
        else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output-dir") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "Error: %s requires an argument\n", argv[i-1]);
                return 1;
            }
            output_dir = argv[i];
        }
        else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--tools-name") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "Error: %s requires an argument\n", argv[i-1]);
                return 1;
            }
            tools_name = argv[i];
        }
        else if (strcmp(argv[i], "--no-dependencies") == 0) {
            no_dependencies = true;
        }
        else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
            PrintUsage(argv[0]);
            return 1;
        }
        else {
            if (module_spec) {
                fprintf(stderr, "Error: Multiple module names specified\n");
                return 1;
            }
            module_spec = argv[i];
        }
    }
    
    if (!module_spec) {
        fprintf(stderr, "Error: No module name specified\n");
        PrintUsage(argv[0]);
        return 1;
    }
    
    // Initialize curl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    // Get configuration
    if (!tools_name) {
        tools_name = GetEnvOrDefault(ENV_TOOLS_NAME, "arch/x86_64");
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
        manifest_path = getenv(ENV_MANIFEST);
        if (!manifest_path) {
            const char* dmf_dir = GetEnvOrDefault(ENV_DMF_DIR, DEFAULT_DMF_DIR);
            const char* dmfc_dir = GetEnvOrDefault(ENV_DMFC_DIR, DEFAULT_DMFC_DIR);
            
            char* found_manifest = FindManifest(dmf_dir, dmfc_dir);
            if (found_manifest) {
                manifest_path = found_manifest;
                printf("Using manifest: %s\n", manifest_path);
            } else {
                fprintf(stderr, "Error: No manifest found. Use -m to specify one.\n");
                curl_global_cleanup();
                return 1;
            }
        }
    }
    
    // Parse module specification
    char module_name[DMOD_MANIFEST_MAX_NAME_LEN];
    char module_version[DMOD_MANIFEST_MAX_VERSION_LEN] = {0};
    
    const char* at_sign = strchr(module_spec, '@');
    if (at_sign) {
        size_t name_len = at_sign - module_spec;
        if (name_len >= sizeof(module_name)) {
            fprintf(stderr, "Error: Module name too long\n");
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
    Dmod_ManifestContext_t* ctx = Dmod_Manifest_Init(tools_name, DownloadWithCurl, NULL);
    if (!ctx) {
        fprintf(stderr, "Error: Failed to initialize manifest parser\n");
        curl_global_cleanup();
        return 1;
    }
    
    // Parse manifest
    printf("Parsing manifest: %s\n", manifest_path);
    bool parse_success = false;
    
    if (strncmp(manifest_path, "http://", 7) == 0 || 
        strncmp(manifest_path, "https://", 8) == 0) {
        parse_success = Dmod_Manifest_ParseUrl(ctx, manifest_path);
    } else {
        parse_success = Dmod_Manifest_ParseFile(ctx, manifest_path);
    }
    
    if (!parse_success) {
        fprintf(stderr, "Error: Failed to parse manifest: %s\n", 
                Dmod_Manifest_GetError(ctx));
        Dmod_Manifest_Free(ctx);
        curl_global_cleanup();
        return 1;
    }
    
    printf("Manifest loaded with %zu entries\n", Dmod_Manifest_GetEntryCount(ctx));
    
    // Find the module
    Dmod_ManifestEntry_t entry;
    if (!Dmod_Manifest_FindEntry(ctx, module_name, 
                                  module_version[0] ? module_version : NULL, 
                                  &entry)) {
        fprintf(stderr, "Error: %s\n", Dmod_Manifest_GetError(ctx));
        Dmod_Manifest_Free(ctx);
        curl_global_cleanup();
        return 1;
    }
    
    printf("Found: %s%s%s at %s\n", 
           entry.name,
           entry.version[0] ? "@" : "",
           entry.version[0] ? entry.version : "",
           entry.url);
    
    // Determine output file name and extension
    const char* url = entry.url;
    const char* ext = strrchr(url, '.');
    char output_file[512];
    
    if (ext && (strcmp(ext, ".dmf") == 0 || strcmp(ext, ".dmfc") == 0 || 
                strcmp(ext, ".zip") == 0 || strcmp(ext, ".dmp") == 0)) {
        snprintf(output_file, sizeof(output_file), "%s/%s%s%s%s",
                output_dir, entry.name,
                entry.version[0] ? "-" : "",
                entry.version[0] ? entry.version : "",
                ext);
    } else {
        // Default to .dmf if no extension
        snprintf(output_file, sizeof(output_file), "%s/%s%s%s.dmf",
                output_dir, entry.name,
                entry.version[0] ? "-" : "",
                entry.version[0] ? entry.version : "");
    }
    
    // Download the file
    if (!DownloadFile(entry.url, output_file)) {
        fprintf(stderr, "Error: Failed to download module\n");
        Dmod_Manifest_Free(ctx);
        curl_global_cleanup();
        return 1;
    }
    
    printf("Successfully downloaded: %s\n", output_file);
    
    // TODO: Handle dependencies if not --no-dependencies
    if (!no_dependencies) {
        // For now, just print a message
        printf("Note: Dependency resolution not yet implemented\n");
    }
    
    // Cleanup
    Dmod_Manifest_Free(ctx);
    curl_global_cleanup();
    
    return 0;
}
