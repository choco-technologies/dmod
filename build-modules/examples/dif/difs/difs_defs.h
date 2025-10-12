#ifndef DMOD_MOD_DEFS_H_difs
#define DMOD_MOD_DEFS_H_difs

#include "dmod_defs.h"


#ifdef DMOD_difs
#  define dmod_difs_api_to_mal(MODULE,NAME)                            \
            DMOD_API_TO_MAL(difs, MODULE, NAME)
#  define dmod_difs_api_to_mal_ex(NAME_IN, MODULE_MAL, NAME_MAL)       \
            DMOD_API_TO_MAL_EX(difs, MODULE_IN, NAME_IN, MODULE_MAL, NAME_MAL)
#  define dmod_difs_api(VERSION, RET, NAME, PARAMS)                    \
            DMOD_INPUT_API(difs, VERSION, RET, NAME, PARAMS)
#  define dmod_difs_global_api(VERSION, RET, NAME, PARAMS)             \
            DMOD_GLOBAL_INPUT_API(difs, VERSION, RET, NAME, PARAMS)
#  define dmod_difs_mal(VERSION, RET, NAME, PARAMS)                    \
            DMOD_MAL_OUTPUT_API(difs , VERSION, RET, NAME, PARAMS)
#  define dmod_difs_global_mal(VERSION, RET, NAME, PARAMS)             \
            DMOD_GLOBAL_MAL_OUTPUT_API(difs, VERSION, RET, NAME, PARAMS)
#  define dmod_difs_api_declaration(VERSION, RET, NAME, PARAMS)        \
            DMOD_INPUT_API_DECLARATION(difs, VERSION, RET, NAME, PARAMS)
#  define dmod_difs_dif(VERSION, RET, NAME, PARAMS)                    \
            extern const char* dmod_difs##NAME; \
            typedef RET (*dmod_difs##NAME##_t) PARAMS;
#  define dmod_difs_dif_api_declaration(VERSION, IMPL_MODULE, RET, NAME, PARAMS)  \
            DMOD_DIF_API_DECLARATION(difs, IMPL_MODULE, VERSION, RET, NAME, PARAMS)
#   define DMOD_MODULE_NAME        "difs"
#   define DMOD_MODULE_VERSION     "1.0"
#   define DMOD_AUTHOR_NAME        "DMOD Team"
#   define DMOD_STACK_SIZE         1024
#   define DMOD_PRIORITY           1
#   define DMOD_MODULE_TYPE        Dmod_ModuleType_Library
#   define DMOD_MANUAL_LOAD        OFF
#else
#  ifdef DMOD_MAL_difs
#  define dmod_difs_mal(VERSION, RET, NAME, PARAMS)            \
                DMOD_MAL_INPUT_API(difs, VERSION, RET, NAME, PARAMS)
#  define dmod_difs_global_mal(VERSION, RET, NAME, PARAMS)     \
                DMOD_GLOBAL_MAL_INPUT_API(difs, VERSION, RET, NAME, PARAMS)
#else 
#  define dmod_difs_mal(VERSION, RET, NAME, PARAMS)            \
                DMOD_MAL_OUTPUT_API(difs , VERSION, RET, NAME, PARAMS)
#  define dmod_difs_global_mal(VERSION, RET, NAME, PARAMS)     \
                DMOD_GLOBAL_MAL_OUTPUT_API(difs, VERSION, RET, NAME, PARAMS)
#endif
#  define dmod_difs_api(VERSION, RET, NAME, PARAMS)            \
                DMOD_OUTPUT_API(difs, VERSION, RET, NAME, PARAMS)
#  define dmod_difs_global_api(VERSION, RET, NAME, PARAMS)     \
                DMOD_GLOBAL_OUTPUT_API(difs, VERSION, RET, NAME, PARAMS)
#  define dmod_difs_dif(VERSION, RET, NAME, PARAMS)            \
                typedef RET (*dmod_difs##NAME##_t) PARAMS; \
                static const char* dmod_difs##NAME DMOD_SECTION(".dmod.outputs") DMOD_UNUSED = (void*)DMOD_MAKE_DIF_SIGNATURE(difs, VERSION, NAME);
#  define dmod_difs_dif_api_declaration(VERSION, IMPL_MODULE, RET, NAME, PARAMS)  \
                RET DMOD_MAKE_DIF_API_FUNCTION_NAME(difs, IMPL_MODULE, NAME) PARAMS; \
                _DMOD_DIF_API_REGISTRATION(difs, IMPL_MODULE, VERSION, NAME) \
                RET DMOD_MAKE_DIF_API_FUNCTION_NAME(difs, IMPL_MODULE, NAME) PARAMS
#endif

#endif // DMOD_MOD_DEFS_H_difs
