#ifndef INC_DMOD_DEFS_H_
#define INC_DMOD_DEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dmod-config.h"
#include "dmod_arch_defs.h"

#define DMOD_HEADER_SIGNATURE           0x444D4F44
#define DMOD_DMFC_SIGNATURE             0x444D4643
#define DMOD_DMP_SIGNATURE              0x444D5048
#define DMOD_CONTEXT_SIGNATURE          0x646D6F64
#define DMOD_MAX_ARCH_NAME_LENGTH       32
#define DMOD_MAX_MODULE_NAME_LENGTH	32

#define DMOD_DO_PRAGMA(x)                       _Pragma (#x)  
#define DMOD_WEAK_DEFAULT(alias, symbol)        DMOD_DO_PRAGMA( weak alias = symbol )
#define DMOD_WEAK_SYMBOL		__attribute__((weak))
#define DMOD_SECTION( NAME )	        __attribute__((section(NAME)))
#define DMOD_USED                        __attribute__((used))
#define DMOD_UNUSED                      __attribute__((unused))
#define DMOD_USED_SECTION( NAME )        __attribute__((used, section(NAME)))
#define DMOD_UNUSED_SECTION( NAME )      __attribute__((unused, section(NAME)))
#define DMOD_GLOBAL_POINTER             DMOD_SECTION(".got")
#define DMOD_PACKED                      __attribute__((__packed__))

#define DMOD_FUNCTION_REDEFINITION( NEW_FUNCTION, OLD_FUNCTION )  \
        DMOD_WEAK_DEFAULT(NEW_FUNCTION, OLD_FUNCTION)

#ifndef DMOD_STACK_ALIGNMENT
#   define DMOD_STACK_ALIGNMENT		16
#endif

#ifndef DMOD_VERSION
#   define DMOD_VERSION		0x00010000
#endif

#ifndef DMOD_DMFC_VERSION
#   define DMOD_DMFC_VERSION		0x0100
#endif

#ifndef DMOD_DMP_VERSION
#   define DMOD_DMP_VERSION		0x0100
#endif

#ifndef DMOD_MAX_MODULES
#   define DMOD_MAX_MODULES		50
#endif

#ifndef DMOD_MAX_VERSION_LENGTH
#   define DMOD_MAX_VERSION_LENGTH		16
#endif

#ifndef DMOD_MAX_LICENSE_NAME_LENGTH
#   define DMOD_MAX_LICENSE_NAME_LENGTH		32
#endif

#ifndef DMOD_MAX_URL_LENGTH
#   define DMOD_MAX_URL_LENGTH		128
#endif

#ifndef DMOD_MAX_AUTHOR_NAME_LENGTH
#   define DMOD_MAX_AUTHOR_NAME_LENGTH		32
#endif

#ifndef DMOD_MAX_PATH_LENGTH
#   define DMOD_MAX_PATH_LENGTH		256
#endif

#ifndef DMOD_MAX_CPU_NAME_LENGTH
#   define DMOD_MAX_CPU_NAME_LENGTH		32
#endif

#ifndef DMOD_MAX_COMPRESSION_NAME_LENGTH
#   define DMOD_MAX_COMPRESSION_NAME_LENGTH		10
#endif

#ifndef DMOD_MAX_PACKAGE_NAME_LENGTH
#   define DMOD_MAX_PACKAGE_NAME_LENGTH		32
#endif

#ifndef DMOD_MAX_NUMBER_OF_PACKAGES
#   define DMOD_MAX_NUMBER_OF_PACKAGES		20
#endif

/**
 * @brief Check if the version is compatible
 */
#define DMOD_COMPATIBLE_VERSION( version )		(( (version) & 0xFFFF0000 ) == ( (DMOD_VERSION) & 0xFFFF0000 ))
#define DMOD_DMFC_COMPATIBLE_VERSION( version )		(( (version) & 0xFF00 ) == ( (DMOD_DMFC_VERSION) & 0xFF00 ))

//==============================================================================
//                              SIGNATURE definitions
//==============================================================================
#define DMOD_SIGNATURE_PREFIX		"\021DMOD\022"
#define DMOD_IRQ_SIGNATURE_PREFIX	"\021DIRQ\022"
#define DMOD_MAL_SIGNATURE_PREFIX	"\021DMAL\022"
#define DMOD_DIF_SIGNATURE_PREFIX	"\021DDIF\022"
#define DMOD_SIGNATURE_SUFFIX           "\0"       
#define DMOD_MAKE_SIGNATURE( MODULE, VERSION, NAME )		    DMOD_SIGNATURE_PREFIX #NAME "@" #MODULE ":" #VERSION DMOD_SIGNATURE_SUFFIX
#define DMOD_MAKE_IRQ_SIGNATURE( NAME )		                    DMOD_IRQ_SIGNATURE_PREFIX #NAME 
#define DMOD_MAKE_MAL_SIGNATURE( MODULE, VERSION, NAME )	    DMOD_MAL_SIGNATURE_PREFIX #NAME "@" #MODULE ":" #VERSION DMOD_SIGNATURE_SUFFIX
#define DMOD_MAKE_DIF_SIGNATURE( MODULE, VERSION, NAME )	    DMOD_DIF_SIGNATURE_PREFIX #NAME "@" #MODULE ":" #VERSION DMOD_SIGNATURE_SUFFIX
#define DMOD_MAKE_VERSION(API_VERSION, MODULE_VERSION)              API_VERSION/MODULE_VERSION

//==============================================================================
//                              DMOD_API definitions
//==============================================================================
#if defined(DMOD_ENABLE_REGISTRATION) 
#       define _DMOD_API_REGISTRATION( REG_NAME, FUNCTION_NAME, SIGNATURE )        \
        volatile const Dmod_ApiRegistration_t REG_NAME DMOD_USED_SECTION(".dmod.inputs") = \
        { \
                .Function = (void*)FUNCTION_NAME, \
                .Signature = SIGNATURE \
        };
#       define _DMOD_DIF_SIGNATURE_REGISTRATION( SIG_NAME, SIGNATURE )        \
        const char* const SIG_NAME = SIGNATURE;
#else 
#       define _DMOD_API_REGISTRATION( REG_NAME, FUNCTION_NAME, SIGNATURE )     \
        extern Dmod_ApiRegistration_t REG_NAME;
#       define _DMOD_DIF_SIGNATURE_REGISTRATION( SIG_NAME, SIGNATURE )     \
        extern const char* const SIG_NAME;
#endif
       
#define DMOD_MAKE_API_FUNCTION_NAME( MODULE, NAME )		        MODULE##NAME
#define DMOD_MAKE_MAL_API_FUNCTION_NAME( MODULE, NAME )		        MODULE##NAME
#define DMOD_MAKE_DIF_API_FUNCTION_NAME( DIF_MODULE, IMPL_MODULE, NAME )    DIF_MODULE##_##IMPL_MODULE##NAME
#define DMOD_MAKE_API_REG_NAME( MODULE, NAME )			        MODULE##NAME##_registration
#define DMOD_MAKE_MAL_API_REG_NAME( MODULE, NAME )			MODULE##NAME##_registration
#define DMOD_MAKE_DIF_API_REG_NAME( DIF_MODULE, IMPL_MODULE, NAME )	DIF_MODULE##_##IMPL_MODULE##NAME##_registration
#define DMOD_MAKE_DIF_SIG_NAME( MODULE, NAME ) dmod_##MODULE##NAME##_sig
#define _DMOD_INPUT_API( FUNCTION_NAME, SIGNATURE, REG_NAME, RET, PARAMS )        \
        extern RET FUNCTION_NAME PARAMS DMOD_USED;\
        _DMOD_API_REGISTRATION( REG_NAME, FUNCTION_NAME, SIGNATURE );

#define _DMOD_OUTPUT_API( FUNCTION_NAME, SIGNATURE, RET, PARAMS )       \
        static RET (*FUNCTION_NAME) PARAMS DMOD_SECTION(".dmod.outputs") DMOD_UNUSED = (void*)SIGNATURE;
#define DMOD_INPUT_API( MODULE, VERSION, RET, NAME, PARAMS )        \
        _DMOD_INPUT_API( DMOD_MAKE_API_FUNCTION_NAME(MODULE,NAME), DMOD_MAKE_SIGNATURE(MODULE, VERSION, NAME), DMOD_MAKE_API_REG_NAME(MODULE,NAME), RET, PARAMS )
#define DMOD_GLOBAL_INPUT_API( MODULE, VERSION, RET, NAME, PARAMS )        \
        _DMOD_INPUT_API( DMOD_MAKE_API_FUNCTION_NAME(,NAME), DMOD_MAKE_SIGNATURE(MODULE, VERSION, NAME), DMOD_MAKE_API_REG_NAME(MODULE,NAME), RET, PARAMS )
#define DMOD_MAL_INPUT_API( MODULE, VERSION, RET, NAME, PARAMS )        \
        _DMOD_INPUT_API( DMOD_MAKE_MAL_API_FUNCTION_NAME(MODULE,NAME), DMOD_MAKE_MAL_SIGNATURE(MODULE, VERSION, NAME), DMOD_MAKE_MAL_API_REG_NAME(MODULE,NAME), RET, PARAMS )
#define DMOD_GLOBAL_MAL_INPUT_API( MODULE, VERSION, RET, NAME, PARAMS )        \
        _DMOD_INPUT_API( DMOD_MAKE_MAL_API_FUNCTION_NAME(,NAME), DMOD_MAKE_MAL_SIGNATURE(MODULE, VERSION, NAME), DMOD_MAKE_MAL_API_REG_NAME(MODULE,NAME), RET, PARAMS )
#define DMOD_OUTPUT_API( MODULE, VERSION, RET, NAME, PARAMS )       \
        _DMOD_OUTPUT_API( DMOD_MAKE_API_FUNCTION_NAME(MODULE,NAME), DMOD_MAKE_SIGNATURE(MODULE, VERSION, NAME), RET, PARAMS )
#define DMOD_GLOBAL_OUTPUT_API( MODULE, VERSION, RET, NAME, PARAMS )       \
        _DMOD_OUTPUT_API( DMOD_MAKE_API_FUNCTION_NAME(,NAME), DMOD_MAKE_SIGNATURE(MODULE, VERSION, NAME), RET, PARAMS )
#define DMOD_MAL_OUTPUT_API( MODULE, VERSION, RET, NAME, PARAMS )       \
        _DMOD_OUTPUT_API( DMOD_MAKE_MAL_API_FUNCTION_NAME(MODULE,NAME), DMOD_MAKE_MAL_SIGNATURE(MODULE, VERSION, NAME), RET, PARAMS )
 #define DMOD_GLOBAL_MAL_OUTPUT_API( MODULE, VERSION, RET, NAME, PARAMS )       \
        _DMOD_OUTPUT_API( DMOD_MAKE_MAL_API_FUNCTION_NAME(,NAME), DMOD_MAKE_MAL_SIGNATURE(MODULE, VERSION, NAME), RET, PARAMS )

#define DMOD_DIF_INPUT_API( DIF_MODULE, IMPL_MODULE, VERSION, RET, NAME, PARAMS )        \
        _DMOD_INPUT_API( DMOD_MAKE_DIF_API_FUNCTION_NAME(DIF_MODULE,IMPL_MODULE,NAME), DMOD_MAKE_DIF_SIGNATURE(DIF_MODULE, VERSION, NAME), DMOD_MAKE_DIF_API_REG_NAME(DIF_MODULE,IMPL_MODULE,NAME), RET, PARAMS )

#define _DMOD_INPUT_API_REGISTRATION( MODULE, VERSION, NAME )        \
        _DMOD_API_REGISTRATION( DMOD_MAKE_API_REG_NAME(MODULE,NAME), DMOD_MAKE_API_FUNCTION_NAME(MODULE,NAME), DMOD_MAKE_SIGNATURE(MODULE, VERSION, NAME) );\
 
#define _DMOD_MAL_API_REGISTRATION( MODULE, VERSION, NAME )        \
        _DMOD_API_REGISTRATION( DMOD_MAKE_MAL_API_REG_NAME(MODULE,NAME), DMOD_MAKE_MAL_API_FUNCTION_NAME(MODULE,NAME), DMOD_MAKE_MAL_SIGNATURE(MODULE, VERSION, NAME) );\

#define _DMOD_DIF_API_REGISTRATION( DIF_MODULE, IMPL_MODULE, VERSION, NAME )        \
        _DMOD_API_REGISTRATION( DMOD_MAKE_DIF_API_REG_NAME(DIF_MODULE,IMPL_MODULE,NAME), DMOD_MAKE_DIF_API_FUNCTION_NAME(DIF_MODULE,IMPL_MODULE,NAME), DMOD_MAKE_DIF_SIGNATURE(DIF_MODULE, VERSION, NAME) );\

#define DMOD_INPUT_API_DECLARATION( MODULE, VERSION, RET, NAME, PARAMS )        \
        RET DMOD_MAKE_API_FUNCTION_NAME(MODULE,NAME) PARAMS

#define DMOD_INPUT_WEAK_API_DECLARATION( MODULE, VERSION, RET, NAME, PARAMS )        \
        DMOD_WEAK_SYMBOL RET DMOD_MAKE_API_FUNCTION_NAME(MODULE,NAME) PARAMS

#define DMOD_MAL_API_DECLARATION( MODULE, VERSION, RET, NAME, PARAMS )       \
        RET DMOD_MAKE_MAL_API_FUNCTION_NAME(MODULE,NAME) PARAMS

#define DMOD_MAL_WEAK_API_DECLARATION( MODULE, VERSION, RET, NAME, PARAMS )       \
        DMOD_WEAK_SYMBOL RET DMOD_MAKE_MAL_API_FUNCTION_NAME(MODULE,NAME) PARAMS

#define DMOD_GLOBAL_MAL_API_DECLARATION( VERSION, RET, NAME, PARAMS )       \
        RET DMOD_MAKE_MAL_API_FUNCTION_NAME(,NAME) PARAMS

#define DMOD_DIF_API_DECLARATION( DIF_MODULE, IMPL_MODULE, VERSION, RET, NAME, PARAMS )       \
        RET DMOD_MAKE_DIF_API_FUNCTION_NAME(DIF_MODULE,IMPL_MODULE,NAME) PARAMS

#define DMOD_IRQ_MAKE_HANDLER_NAME(NAME)                __irq_##NAME
#define DMOD_IRQ_MAKE_REG_NAME(NAME)                    __irq_##NAME##_registration

#define DMOD_MAL_CONNECT( MODULE, NAME, FUNCTION_NAME )   \
                        DMOD_FUNCTION_REDEFINITION( DMOD_MAKE_MAL_API_FUNCTION_NAME(MODULE,NAME), FUNCTION_NAME )
#define DMOD_CONNECT_API_TO_MAL_EX( MODULE_IN, NAME_IN, MODULE_MAL, NAME_MAL )   \
                        DMOD_MAL_CONNECT( MODULE_MAL, NAME_MAL, DMOD_MAKE_API_FUNCTION_NAME(MODULE_IN,NAME_IN) )
#define DMOD_CONNECT_API_TO_MAL( MODULE_IN, MODULE_MAL, NAME )                   \
                        DMOD_CONNECT_API_TO_MAL_EX( MODULE_IN, NAME, MODULE_MAL, NAME )
#define DMOD_GLOBAL_CONNECT_API_TO_MAL_EX( NAME_IN, MODULE_MAL, NAME_MAL )                       \
                        DMOD_MAL_CONNECT( MODULE_MAL, NAME_MAL, DMOD_MAKE_API_FUNCTION_NAME(,NAME_IN) )
#define DMOD_GLOBAL_CONNECT_API_TO_MAL( MODULE_MAL, NAME )                       \
                        DMOD_GLOBAL_CONNECT_API_TO_MAL_EX( NAME, MODULE_MAL, NAME )

/**
 * @brief Macro for IRQ handling
 * 
 * @param NAME Name of the IRQ handler
 * 
 * @note This macro defines the IRQ handler function and registers it in the DMOD
 */
#define DMOD_IRQ_HANDLER( NAME )        \
        static void DMOD_IRQ_MAKE_HANDLER_NAME(NAME)(void);\
        static Dmod_ApiRegistration_t DMOD_IRQ_MAKE_REG_NAME(NAME) DMOD_SECTION(.inputs) = \
        { \
            .Function = (void*)DMOD_IRQ_MAKE_HANDLER_NAME(NAME), \
            .Signature = DMOD_MAKE_IRQ_SIGNATURE(NAME) \
        };\
        static void DMOD_IRQ_MAKE_HANDLER_NAME(NAME)(void)

#ifdef DOXYGEN
/**
 * @brief Defines Builtin API
 * 
 * Use this macro to define the builtin API function. The function will be
 * available in the DMOD system and can be used by the modules.
 * 
 * @param MODULE Module name
 * @param VERSION Module version
 * @param RET Return type
 * @param NAME Function name
 * 
 * @note This macro defines the builtin API function
 */
#define DMOD_BUILTIN_API( MODULE, VERSION, RET, NAME, PARAMS )      RET NAME PARAMS

/**
 * @brief Defines Module API
 * 
 * Use this macro to define the module API function. The function will be
 * available in the DMOD module and can be used by the system and other modules.
 * 
 * @param MODULE Module name
 * @param VERSION Module version
 * @param RET Return type
 * @param NAME Function name
 * 
 * @note This macro defines the module API function
 */
#define DMOD_MODULE_API( MODULE, VERSION, RET, NAME, PARAMS )       RET NAME PARAMS
#elif DMOD_SYSTEM_EN == ON
#define DMOD_BUILTIN_API( MODULE, VERSION, RET, NAME, PARAMS )      DMOD_INPUT_API( MODULE, VERSION, RET, NAME, PARAMS )
#elif DMOD_MODULE_EN == ON
#define DMOD_BUILTIN_API( MODULE, VERSION, RET, NAME, PARAMS )      DMOD_OUTPUT_API( MODULE, VERSION, RET, NAME, PARAMS )
#endif

#ifdef __cplusplus
}
#endif

#endif /* INC_DMOD_DEFS_H_ */
