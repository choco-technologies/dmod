#ifndef DMOD_MOD_DEFS_H_example_app
#define DMOD_MOD_DEFS_H_example_app

#include "dmod_defs.h"

#ifndef dmod_example_app_version
#  define dmod_example_app_version "0.1"
#endif

#ifdef DMOD_example_app
#  define dmod_example_app_api_to_mal(MODULE,NAME)                            \
            DMOD_API_TO_MAL(example_app, MODULE, NAME)
#  define dmod_example_app_api_to_mal_ex(NAME_IN, MODULE_MAL, NAME_MAL)       \
            DMOD_API_TO_MAL_EX(example_app, MODULE_IN, NAME_IN, MODULE_MAL, NAME_MAL)
#  define dmod_example_app_api(VERSION, RET, NAME, PARAMS)                    \
            DMOD_INPUT_API(example_app, DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#  define dmod_example_app_global_api(VERSION, RET, NAME, PARAMS)             \
            DMOD_GLOBAL_INPUT_API(example_app, DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#  define dmod_example_app_mal(VERSION, RET, NAME, PARAMS)                    \
            DMOD_MAL_OUTPUT_API(example_app , DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#  define dmod_example_app_global_mal(VERSION, RET, NAME, PARAMS)             \
            DMOD_GLOBAL_MAL_OUTPUT_API(example_app, DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#  define dmod_example_app_api_declaration(VERSION, RET, NAME, PARAMS)        \
            DMOD_INPUT_API_DECLARATION(example_app, DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#  define dmod_example_app_dif(VERSION, RET, NAME, PARAMS)                    \
    typedef RET (*dmod_example_app##NAME##_t) PARAMS; \
    extern const char* const DMOD_MAKE_DIF_SIG_NAME(example_app, NAME);
#  define dmod_example_app_dif_api_declaration(VERSION, IMPL_MODULE, RET, NAME, PARAMS)  \
            DMOD_DIF_API_DECLARATION(example_app, IMPL_MODULE, DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#   ifndef DMOD_MODULE_NAME
#       define DMOD_MODULE_NAME        "example_app"
#   endif
#   ifndef DMOD_MODULE_VERSION
#       define DMOD_MODULE_VERSION     "0.1"
#   endif
#   define DMOD_AUTHOR_NAME        "Patryk Kubiak"
#   define DMOD_STACK_SIZE         1024
#   define DMOD_PRIORITY           0
#   define DMOD_MODULE_TYPE        Dmod_ModuleType_Application
#   define DMOD_MANUAL_LOAD        OFF
#else
#  ifdef DMOD_MAL_example_app
#  define dmod_example_app_mal(VERSION, RET, NAME, PARAMS)            \
                DMOD_MAL_INPUT_API(example_app, DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#  define dmod_example_app_global_mal(VERSION, RET, NAME, PARAMS)     \
                DMOD_GLOBAL_MAL_INPUT_API(example_app, DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#else 
#  define dmod_example_app_mal(VERSION, RET, NAME, PARAMS)            \
                DMOD_MAL_OUTPUT_API(example_app , DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#  define dmod_example_app_global_mal(VERSION, RET, NAME, PARAMS)     \
                DMOD_GLOBAL_MAL_OUTPUT_API(example_app, DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#endif
#  define dmod_example_app_api(VERSION, RET, NAME, PARAMS)            \
                DMOD_OUTPUT_API(example_app, DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#  define dmod_example_app_global_api(VERSION, RET, NAME, PARAMS)     \
                DMOD_GLOBAL_OUTPUT_API(example_app, DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
# ifdef ENABLE_DIF_REGISTRATIONS
#  define dmod_example_app_dif(VERSION, RET, NAME, PARAMS)            \
                typedef RET (*dmod_example_app##NAME##_t) PARAMS; \
                const char* const DMOD_MAKE_DIF_SIG_NAME(example_app, NAME) = DMOD_MAKE_DIF_SIGNATURE(example_app, DMOD_MAKE_VERSION(VERSION,0.1), NAME);
#  else
#  define dmod_example_app_dif(VERSION, RET, NAME, PARAMS)            \
                typedef RET (*dmod_example_app##NAME##_t) PARAMS; \
                _DMOD_DIF_SIGNATURE_REGISTRATION(DMOD_MAKE_DIF_SIG_NAME(example_app, NAME), DMOD_MAKE_DIF_SIGNATURE(example_app, DMOD_MAKE_VERSION(VERSION,0.1), NAME))
#  endif
#  define dmod_example_app_dif_api_declaration(VERSION, IMPL_MODULE, RET, NAME, PARAMS)  \
                RET DMOD_MAKE_DIF_API_FUNCTION_NAME(example_app, IMPL_MODULE, NAME) PARAMS; \
                _DMOD_DIF_API_REGISTRATION(example_app, IMPL_MODULE, DMOD_MAKE_VERSION(VERSION,0.1), NAME) \
                RET DMOD_MAKE_DIF_API_FUNCTION_NAME(example_app, IMPL_MODULE, NAME) PARAMS
#endif

#endif // DMOD_MOD_DEFS_H_example_app
