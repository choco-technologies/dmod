#ifndef DMOD_CONFIG_H
#define DMOD_CONFIG_H

#define dmod_VERSION_MAJOR 0
#define dmod_VERSION_MINOR 1
#define DMOD_VERSION_STRING "0.1"
#define DMOD_VERSION        ((uint32_t)((0 << 8) | (1)))
#define DMOD_MAX_MODULES                30
#define DMOD_MAX_REQUIRED_MODULES       10
#define DMOD_USE_STDLIB                 ON
#define DMOD_USE_GETENV                 ON
#define DMOD_USE_STDIO                  ON
#define DMOD_USE_DIRENT                 ON
#define DMOD_USE_ASSERT                 ON
#define DMOD_USE_PTHREAD                ON
#define DMOD_USE_MMAN                   ON
#define DMOD_USE_ALIGNED_ALLOC          ON
#define DMOD_USE_ALIGNED_MALLOC_MOCK    OFF
#define DMOD_USE_REALLOC                ON
#define DMOD_USE_FASTLZ                 ON
#define DMOD_MODE           "DMOD_SYSTEM"
#define DMOD_SYSTEM_EN      ON
#define DMOD_MODULE_EN      OFF
#define DMOD_SYSTEM_VERSION_MAJOR 0
#define DMOD_SYSTEM_VERSION_MINOR 1
#define DMOD_SYSTEM_VERSION_STRING "0.1"
#define DMOD_SYSTEM_VERSION ((uint32_t)((0 << 8) | (1)))
#define DMOD_REPO_DIR        "/home/runner/work/dmod/dmod/build_cmake/dmf"
#define DMOD_REPO_PATHS      "/home/runner/work/dmod/dmod/build_cmake/dmf:/home/runner/work/dmod/dmod/build_cmake/dmfc"
#define DMOD_CPU_NAME        ""
#define DMOD_ARRAY_SEP       ":"
#define DMOD_PATH_SEP        "/"

// API 
#define DMOD_BUILTIN_COMPRESSION_API        ON

#ifndef ON 
#   define ON 1
#endif

#ifndef OFF
#   define OFF 0
#endif

#ifndef DMOD_BUILD_DIR
#   define DMOD_BUILD_DIR "/home/runner/work/dmod/dmod/build_cmake"
#endif

#endif // DMOD_CONFIG_H
