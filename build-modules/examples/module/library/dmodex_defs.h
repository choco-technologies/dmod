#ifndef DMOD_MOD_DEFS_H_dmodex
#define DMOD_MOD_DEFS_H_dmodex

#include "dmod_defs.h"


#ifdef DMOD_dmodex
#  define dmod_dmodex_api_to_mal(MODULE,NAME)                            \
            DMOD_API_TO_MAL(dmodex, MODULE, NAME)
#  define dmod_dmodex_api_to_mal_ex(NAME_IN, MODULE_MAL, NAME_MAL)       \
            DMOD_API_TO_MAL_EX(dmodex, MODULE_IN, NAME_IN, MODULE_MAL, NAME_MAL)
#  define dmod_dmodex_api(VERSION, RET, NAME, PARAMS)                    \
            DMOD_INPUT_API(dmodex, VERSION, RET, NAME, PARAMS)
#  define dmod_dmodex_global_api(VERSION, RET, NAME, PARAMS)             \
            DMOD_GLOBAL_INPUT_API(dmodex, VERSION, RET, NAME, PARAMS)
#  define dmod_dmodex_mal(VERSION, RET, NAME, PARAMS)                    \
            DMOD_MAL_OUTPUT_API(dmodex , VERSION, RET, NAME, PARAMS)
#  define dmod_dmodex_global_mal(VERSION, RET, NAME, PARAMS)             \
            DMOD_GLOBAL_MAL_OUTPUT_API(dmodex, VERSION, RET, NAME, PARAMS)
#  define dmod_dmodex_api_declaration(VERSION, RET, NAME, PARAMS)        \
            DMOD_INPUT_API_DECLARATION(dmodex, VERSION, RET, NAME, PARAMS)
#  define dmod_dmodex_dif(VERSION, RET, NAME, PARAMS)                    \
            extern const char* dmod_dmodex##NAME; \
            typedef RET (*dmod_dmodex##NAME##_t) PARAMS;
#  define dmod_dmodex_dif_api_declaration(VERSION, IMPL_MODULE, RET, NAME, PARAMS)  \
            DMOD_DIF_API_DECLARATION(dmodex, IMPL_MODULE, VERSION, RET, NAME, PARAMS)
#   define DMOD_MODULE_NAME        "dmodex"
#   define DMOD_MODULE_VERSION     "0.1"
#   define DMOD_AUTHOR_NAME        "Patryk Kubiak"
#   define DMOD_STACK_SIZE         1024
#   define DMOD_PRIORITY           1
#   define DMOD_MODULE_TYPE        Dmod_ModuleType_Library
#   define DMOD_MANUAL_LOAD        OFF
#else
#  ifdef DMOD_MAL_dmodex
#  define dmod_dmodex_mal(VERSION, RET, NAME, PARAMS)            \
                DMOD_MAL_INPUT_API(dmodex, VERSION, RET, NAME, PARAMS)
#  define dmod_dmodex_global_mal(VERSION, RET, NAME, PARAMS)     \
                DMOD_GLOBAL_MAL_INPUT_API(dmodex, VERSION, RET, NAME, PARAMS)
#else 
#  define dmod_dmodex_mal(VERSION, RET, NAME, PARAMS)            \
                DMOD_MAL_OUTPUT_API(dmodex , VERSION, RET, NAME, PARAMS)
#  define dmod_dmodex_global_mal(VERSION, RET, NAME, PARAMS)     \
                DMOD_GLOBAL_MAL_OUTPUT_API(dmodex, VERSION, RET, NAME, PARAMS)
#endif
#  define dmod_dmodex_api(VERSION, RET, NAME, PARAMS)            \
                DMOD_OUTPUT_API(dmodex, VERSION, RET, NAME, PARAMS)
#  define dmod_dmodex_global_api(VERSION, RET, NAME, PARAMS)     \
                DMOD_GLOBAL_OUTPUT_API(dmodex, VERSION, RET, NAME, PARAMS)
#  define dmod_dmodex_dif(VERSION, RET, NAME, PARAMS)            \
                typedef RET (*dmod_dmodex##NAME##_t) PARAMS; \
                static const char* dmod_dmodex##NAME DMOD_SECTION(".dmod.outputs") DMOD_UNUSED = (void*)DMOD_MAKE_DIF_SIGNATURE(dmodex, VERSION, NAME);
#  define dmod_dmodex_dif_api_declaration(VERSION, IMPL_MODULE, RET, NAME, PARAMS)  \
                RET DMOD_MAKE_DIF_API_FUNCTION_NAME(dmodex, IMPL_MODULE, NAME) PARAMS; \
                _DMOD_DIF_API_REGISTRATION(dmodex, IMPL_MODULE, VERSION, NAME) \
                RET DMOD_MAKE_DIF_API_FUNCTION_NAME(dmodex, IMPL_MODULE, NAME) PARAMS
#endif

#endif // DMOD_MOD_DEFS_H_dmodex
