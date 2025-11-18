#ifndef DMOD_MOD_DEFS_H_flashfs
#define DMOD_MOD_DEFS_H_flashfs

#include "dmod_defs.h"

#ifndef dmod_flashfs_version
#  define dmod_flashfs_version "1.0"
#endif

#ifdef DMOD_flashfs
#  define dmod_flashfs_api_to_mal(MODULE,NAME)                            \
            DMOD_API_TO_MAL(flashfs, MODULE, NAME)
#  define dmod_flashfs_api_to_mal_ex(NAME_IN, MODULE_MAL, NAME_MAL)       \
            DMOD_API_TO_MAL_EX(flashfs, MODULE_IN, NAME_IN, MODULE_MAL, NAME_MAL)
#  define dmod_flashfs_api(VERSION, RET, NAME, PARAMS)                    \
            DMOD_INPUT_API(flashfs, DMOD_MAKE_VERSION(VERSION,1.0), RET, NAME, PARAMS)
#  define dmod_flashfs_global_api(VERSION, RET, NAME, PARAMS)             \
            DMOD_GLOBAL_INPUT_API(flashfs, DMOD_MAKE_VERSION(VERSION,1.0), RET, NAME, PARAMS)
#  define dmod_flashfs_mal(VERSION, RET, NAME, PARAMS)                    \
            DMOD_MAL_OUTPUT_API(flashfs , DMOD_MAKE_VERSION(VERSION,1.0), RET, NAME, PARAMS)
#  define dmod_flashfs_global_mal(VERSION, RET, NAME, PARAMS)             \
            DMOD_GLOBAL_MAL_OUTPUT_API(flashfs, DMOD_MAKE_VERSION(VERSION,1.0), RET, NAME, PARAMS)
#  define dmod_flashfs_api_declaration(VERSION, RET, NAME, PARAMS)        \
            DMOD_INPUT_API_DECLARATION(flashfs, DMOD_MAKE_VERSION(VERSION,1.0), RET, NAME, PARAMS)
#  define dmod_flashfs_dif(VERSION, RET, NAME, PARAMS)                    \
    typedef RET (*dmod_flashfs##NAME##_t) PARAMS; \
    extern const char* const DMOD_MAKE_DIF_SIG_NAME(flashfs, NAME);
#  define dmod_flashfs_dif_api_declaration(VERSION, IMPL_MODULE, RET, NAME, PARAMS)  \
            DMOD_DIF_API_DECLARATION(flashfs, IMPL_MODULE, DMOD_MAKE_VERSION(VERSION,1.0), RET, NAME, PARAMS)
#   ifndef DMOD_MODULE_NAME
#       define DMOD_MODULE_NAME        "flashfs"
#   endif
#   ifndef DMOD_MODULE_VERSION
#       define DMOD_MODULE_VERSION     "1.0"
#   endif
#   define DMOD_AUTHOR_NAME        "DMOD Team"
#   define DMOD_STACK_SIZE         1024
#   define DMOD_PRIORITY           1
#   define DMOD_MODULE_TYPE        Dmod_ModuleType_Library
#   define DMOD_MANUAL_LOAD        OFF
#else
#  ifdef DMOD_MAL_flashfs
#  define dmod_flashfs_mal(VERSION, RET, NAME, PARAMS)            \
                DMOD_MAL_INPUT_API(flashfs, DMOD_MAKE_VERSION(VERSION,1.0), RET, NAME, PARAMS)
#  define dmod_flashfs_global_mal(VERSION, RET, NAME, PARAMS)     \
                DMOD_GLOBAL_MAL_INPUT_API(flashfs, DMOD_MAKE_VERSION(VERSION,1.0), RET, NAME, PARAMS)
#else 
#  define dmod_flashfs_mal(VERSION, RET, NAME, PARAMS)            \
                DMOD_MAL_OUTPUT_API(flashfs , DMOD_MAKE_VERSION(VERSION,1.0), RET, NAME, PARAMS)
#  define dmod_flashfs_global_mal(VERSION, RET, NAME, PARAMS)     \
                DMOD_GLOBAL_MAL_OUTPUT_API(flashfs, DMOD_MAKE_VERSION(VERSION,1.0), RET, NAME, PARAMS)
#endif
#  define dmod_flashfs_api(VERSION, RET, NAME, PARAMS)            \
                DMOD_OUTPUT_API(flashfs, DMOD_MAKE_VERSION(VERSION,1.0), RET, NAME, PARAMS)
#  define dmod_flashfs_global_api(VERSION, RET, NAME, PARAMS)     \
                DMOD_GLOBAL_OUTPUT_API(flashfs, DMOD_MAKE_VERSION(VERSION,1.0), RET, NAME, PARAMS)
# ifdef ENABLE_DIF_REGISTRATIONS
#  define dmod_flashfs_dif(VERSION, RET, NAME, PARAMS)            \
                typedef RET (*dmod_flashfs##NAME##_t) PARAMS; \
                const char* const DMOD_MAKE_DIF_SIG_NAME(flashfs, NAME) = DMOD_MAKE_DIF_SIGNATURE(flashfs, DMOD_MAKE_VERSION(VERSION,1.0), NAME);
#  else
#  define dmod_flashfs_dif(VERSION, RET, NAME, PARAMS)            \
                typedef RET (*dmod_flashfs##NAME##_t) PARAMS; \
                _DMOD_DIF_SIGNATURE_REGISTRATION(DMOD_MAKE_DIF_SIG_NAME(flashfs, NAME), DMOD_MAKE_DIF_SIGNATURE(flashfs, DMOD_MAKE_VERSION(VERSION,1.0), NAME))
#  endif
#  define dmod_flashfs_dif_api_declaration(VERSION, IMPL_MODULE, RET, NAME, PARAMS)  \
                RET DMOD_MAKE_DIF_API_FUNCTION_NAME(flashfs, IMPL_MODULE, NAME) PARAMS; \
                _DMOD_DIF_API_REGISTRATION(flashfs, IMPL_MODULE, DMOD_MAKE_VERSION(VERSION,1.0), NAME) \
                RET DMOD_MAKE_DIF_API_FUNCTION_NAME(flashfs, IMPL_MODULE, NAME) PARAMS
#endif

#endif // DMOD_MOD_DEFS_H_flashfs
