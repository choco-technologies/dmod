#ifndef DMOD_MOD_DEFS_H_MyLibrary
#define DMOD_MOD_DEFS_H_MyLibrary

#include "dmod_defs.h"

#ifndef dmod_mylibrary_version
#  define dmod_mylibrary_version "0.1"
#endif

#ifdef DMOD_MyLibrary
#  define dmod_mylibrary_api_to_mal(MODULE,NAME)                            \
            DMOD_API_TO_MAL(MyLibrary, MODULE, NAME)
#  define dmod_mylibrary_api_to_mal_ex(NAME_IN, MODULE_MAL, NAME_MAL)       \
            DMOD_API_TO_MAL_EX(MyLibrary, MODULE_IN, NAME_IN, MODULE_MAL, NAME_MAL)
#  define dmod_mylibrary_api(VERSION, RET, NAME, PARAMS)                    \
            DMOD_INPUT_API(MyLibrary, DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#  define dmod_mylibrary_global_api(VERSION, RET, NAME, PARAMS)             \
            DMOD_GLOBAL_INPUT_API(MyLibrary, DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#  define dmod_mylibrary_mal(VERSION, RET, NAME, PARAMS)                    \
            DMOD_MAL_OUTPUT_API(MyLibrary , DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#  define dmod_mylibrary_global_mal(VERSION, RET, NAME, PARAMS)             \
            DMOD_GLOBAL_MAL_OUTPUT_API(MyLibrary, DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#  define dmod_mylibrary_api_declaration(VERSION, RET, NAME, PARAMS)        \
            DMOD_INPUT_API_DECLARATION(MyLibrary, DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#  define dmod_mylibrary_dif(VERSION, RET, NAME, PARAMS)                    \
    typedef RET (*dmod_mylibrary##NAME##_t) PARAMS; \
    extern const char* const DMOD_MAKE_DIF_SIG_NAME(mylibrary, NAME);
#  define dmod_mylibrary_dif_api_declaration(VERSION, IMPL_MODULE, RET, NAME, PARAMS)  \
            DMOD_DIF_API_DECLARATION(MyLibrary, IMPL_MODULE, DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#   ifndef DMOD_MODULE_NAME
#       define DMOD_MODULE_NAME        "MyLibrary"
#   endif
#   ifndef DMOD_MODULE_VERSION
#       define DMOD_MODULE_VERSION     "0.1"
#   endif
#   define DMOD_AUTHOR_NAME        "John;Doe"
#   define DMOD_STACK_SIZE         1024
#   define DMOD_PRIORITY           1
#   define DMOD_MODULE_TYPE        Dmod_ModuleType_Library
#   define DMOD_MANUAL_LOAD        OFF
#else
#  ifdef DMOD_MAL_MyLibrary
#  define dmod_mylibrary_mal(VERSION, RET, NAME, PARAMS)            \
                DMOD_MAL_INPUT_API(MyLibrary, DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#  define dmod_mylibrary_global_mal(VERSION, RET, NAME, PARAMS)     \
                DMOD_GLOBAL_MAL_INPUT_API(MyLibrary, DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#else 
#  define dmod_mylibrary_mal(VERSION, RET, NAME, PARAMS)            \
                DMOD_MAL_OUTPUT_API(MyLibrary , DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#  define dmod_mylibrary_global_mal(VERSION, RET, NAME, PARAMS)     \
                DMOD_GLOBAL_MAL_OUTPUT_API(MyLibrary, DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#endif
#  define dmod_mylibrary_api(VERSION, RET, NAME, PARAMS)            \
                DMOD_OUTPUT_API(MyLibrary, DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
#  define dmod_mylibrary_global_api(VERSION, RET, NAME, PARAMS)     \
                DMOD_GLOBAL_OUTPUT_API(MyLibrary, DMOD_MAKE_VERSION(VERSION,0.1), RET, NAME, PARAMS)
# ifdef ENABLE_DIF_REGISTRATIONS
#  define dmod_mylibrary_dif(VERSION, RET, NAME, PARAMS)            \
                typedef RET (*dmod_mylibrary##NAME##_t) PARAMS; \
                const char* const DMOD_MAKE_DIF_SIG_NAME(mylibrary, NAME) = DMOD_MAKE_DIF_SIGNATURE(MyLibrary, DMOD_MAKE_VERSION(VERSION,0.1), NAME);
#  else
#  define dmod_mylibrary_dif(VERSION, RET, NAME, PARAMS)            \
                typedef RET (*dmod_mylibrary##NAME##_t) PARAMS; \
                _DMOD_DIF_SIGNATURE_REGISTRATION(DMOD_MAKE_DIF_SIG_NAME(mylibrary, NAME), DMOD_MAKE_DIF_SIGNATURE(MyLibrary, DMOD_MAKE_VERSION(VERSION,0.1), NAME))
#  endif
#  define dmod_mylibrary_dif_api_declaration(VERSION, IMPL_MODULE, RET, NAME, PARAMS)  \
                RET DMOD_MAKE_DIF_API_FUNCTION_NAME(MyLibrary, IMPL_MODULE, NAME) PARAMS; \
                _DMOD_DIF_API_REGISTRATION(MyLibrary, IMPL_MODULE, DMOD_MAKE_VERSION(VERSION,0.1), NAME) \
                RET DMOD_MAKE_DIF_API_FUNCTION_NAME(MyLibrary, IMPL_MODULE, NAME) PARAMS
#endif

#endif // DMOD_MOD_DEFS_H_MyLibrary
