#ifndef DMOD_MOD_DEFS_H_example_app
#define DMOD_MOD_DEFS_H_example_app

#include "dmod_defs.h"


#ifdef DMOD_example_app
#  define dmod_example_app_api_to_mal(MODULE,NAME)                            \
            DMOD_API_TO_MAL(example_app, MODULE, NAME)
#  define dmod_example_app_api_to_mal_ex(NAME_IN, MODULE_MAL, NAME_MAL)       \
            DMOD_API_TO_MAL_EX(example_app, MODULE_IN, NAME_IN, MODULE_MAL, NAME_MAL)
#  define dmod_example_app_api(VERSION, RET, NAME, PARAMS)                    \
            DMOD_INPUT_API(example_app, VERSION, RET, NAME, PARAMS)
#  define dmod_example_app_global_api(VERSION, RET, NAME, PARAMS)             \
            DMOD_GLOBAL_INPUT_API(example_app, VERSION, RET, NAME, PARAMS)
#  define dmod_example_app_mal(VERSION, RET, NAME, PARAMS)                    \
            DMOD_MAL_OUTPUT_API(example_app , VERSION, RET, NAME, PARAMS)
#  define dmod_example_app_global_mal(VERSION, RET, NAME, PARAMS)             \
            DMOD_GLOBAL_MAL_OUTPUT_API(example_app, VERSION, RET, NAME, PARAMS)
#  define dmod_example_app_api_declaration(VERSION, RET, NAME, PARAMS)        \
            DMOD_INPUT_API_DECLARATION(example_app, VERSION, RET, NAME, PARAMS)
#  define dmod_example_app_dif(VERSION, RET, NAME, PARAMS)                    \
            extern const char* dmod_example_app##NAME; \
            typedef RET (*dmod_example_app##NAME##_t) PARAMS;
#  define dmod_example_app_dif_api_declaration(VERSION, IMPL_MODULE, RET, NAME, PARAMS)  \
            DMOD_DIF_API_DECLARATION(example_app, IMPL_MODULE, VERSION, RET, NAME, PARAMS)
#   define DMOD_MODULE_NAME        "example_app"
#   define DMOD_MODULE_VERSION     "0.1"
#   define DMOD_AUTHOR_NAME        "Patryk Kubiak"
#   define DMOD_STACK_SIZE         1024
#   define DMOD_PRIORITY           0
#   define DMOD_MODULE_TYPE        Dmod_ModuleType_Application
#   define DMOD_MANUAL_LOAD        OFF
#else
#  ifdef DMOD_MAL_example_app
#  define dmod_example_app_mal(VERSION, RET, NAME, PARAMS)            \
                DMOD_MAL_INPUT_API(example_app, VERSION, RET, NAME, PARAMS)
#  define dmod_example_app_global_mal(VERSION, RET, NAME, PARAMS)     \
                DMOD_GLOBAL_MAL_INPUT_API(example_app, VERSION, RET, NAME, PARAMS)
#else 
#  define dmod_example_app_mal(VERSION, RET, NAME, PARAMS)            \
                DMOD_MAL_OUTPUT_API(example_app , VERSION, RET, NAME, PARAMS)
#  define dmod_example_app_global_mal(VERSION, RET, NAME, PARAMS)     \
                DMOD_GLOBAL_MAL_OUTPUT_API(example_app, VERSION, RET, NAME, PARAMS)
#endif
#  define dmod_example_app_api(VERSION, RET, NAME, PARAMS)            \
                DMOD_OUTPUT_API(example_app, VERSION, RET, NAME, PARAMS)
#  define dmod_example_app_global_api(VERSION, RET, NAME, PARAMS)     \
                DMOD_GLOBAL_OUTPUT_API(example_app, VERSION, RET, NAME, PARAMS)
#  define dmod_example_app_dif(VERSION, RET, NAME, PARAMS)            \
                typedef RET (*dmod_example_app##NAME##_t) PARAMS; \
                static const char* dmod_example_app##NAME DMOD_SECTION(".dmod.outputs") DMOD_UNUSED = (void*)DMOD_MAKE_DIF_SIGNATURE(example_app, VERSION, NAME);
#  define dmod_example_app_dif_api_declaration(VERSION, IMPL_MODULE, RET, NAME, PARAMS)  \
                RET DMOD_MAKE_DIF_API_FUNCTION_NAME(example_app, IMPL_MODULE, NAME) PARAMS; \
                _DMOD_DIF_API_REGISTRATION(example_app, IMPL_MODULE, VERSION, NAME) \
                RET DMOD_MAKE_DIF_API_FUNCTION_NAME(example_app, IMPL_MODULE, NAME) PARAMS
#endif

#endif // DMOD_MOD_DEFS_H_example_app
