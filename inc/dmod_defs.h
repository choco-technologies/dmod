#ifndef INC_DMOD_DEFS_H_
#define INC_DMOD_DEFS_H_

#define DMOD_HEADER_SIGNATURE           0x444D4F44
#define DMOD_CONTEXT_SIGNATURE          0x646D6F64
#define DMOD_MAX_ARCH_NAME_LENGTH       10
#define DMOD_MAX_MODULE_NAME_LENGTH		32

#define DMOD_WEAK_SYMBOL		__attribute__((weak))
#define DMOD_SECTION( NAME )	__attribute__((section(NAME)))

#ifndef DMOD_STACK_ALIGNMENT
#   define DMOD_STACK_ALIGNMENT		16
#endif

#ifndef DMOD_VERSION
#   define DMOD_VERSION		0x00010000
#endif

//==============================================================================
//                              SIGNATURE definitions
//==============================================================================
#define DMOD_SIGNATURE_PREFIX		"\021DMOD\022"
#define DMOD_IRQ_SIGNATURE_PREFIX	"\021DIRQ\022"
#define DMOD_SIGNATURE_SUFFIX           "\0"       
#define DMOD_MAKE_SIGNATURE( MODULE, VERSION, NAME )		    DMOD_SIGNATURE_PREFIX #NAME "@" #MODULE ":" #VERSION DMOD_SIGNATURE_SUFFIX
#define DMOD_MAKE_IRQ_SIGNATURE( NAME )		                    DMOD_IRQ_SIGNATURE_PREFIX #NAME 

//==============================================================================
//                              DMOD_API definitions
//==============================================================================
#define DMOD_MAKE_API_FUNCTION_NAME( MODULE, NAME )		        MODULE##NAME
#define DMOD_MAKE_API_REG_NAME( MODULE, NAME )			        MODULE##NAME##_registration
#define DMOD_INPUT_API( MODULE, VERSION, RET, NAME, PARAMS )        \
        extern RET DMOD_MAKE_API_FUNCTION_NAME(MODULE,NAME) PARAMS;\
        static Dmod_ApiRegistration_t DMOD_MAKE_API_REG_NAME(MODULE,NAME) DMOD_SECTION(.inputs) = \
        { \
            .Function = (void*)DMOD_MAKE_API_FUNCTION_NAME(MODULE,NAME), \
            .Signature = DMOD_MAKE_SIGNATURE(MODULE, VERSION, NAME) \
        };
#define DMOD_OUTPUT_API( MODULE, VERSION, RET, NAME, PARAMS )       \
        static RET (*DMOD_MAKE_API_FUNCTION_NAME(MODULE,NAME)) PARAMS DMOD_SECTION(.outputs) = DMOD_MAKE_SIGNATURE(MODULE, VERSION, NAME);\

#define DMOD_IRQ_MAKE_HANDLER_NAME(NAME)                __irq_##NAME
#define DMOD_IRQ_MAKE_REG_NAME(NAME)                    __irq_##NAME##_registration

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


//==============================================================================
//                              ARCHITECTURE DEFINITIONS
//==============================================================================
#ifndef DMOD_ARCH
#   if defined(__x86_64__) || defined(_M_X64)
#       define DMOD_ARCH	"x86_64"
#   elif defined(__i386__) || defined(_M_IX86)
#       define DMOD_ARCH	"x86"
#   elif defined(__arm__) || defined(_M_ARM)
#       define DMOD_ARCH	"ARM"
#   elif defined(__aarch64__) || defined(_M_ARM64)
#       define DMOD_ARCH	"ARM64"
#   elif defined(__ppc__) || defined(__powerpc__) || defined(__ppc64__) || defined(__powerpc64__)
#       define DMOD_ARCH	"PPC"    
#   elif defined(__mips__)
#       define DMOD_ARCH	"MIPS"
#   elif defined(__riscv)
#       if __riscv_xlen == 32
#           define DMOD_ARCH	"RISC-V (32-bit)"
#       elif __riscv_xlen == 64
#           define DMOD_ARCH	"RISC-V (64-bit)"
#       else
#           define DMOD_ARCH	"RISC-V"
#       endif
#   else
#       error "Unknown architecture. Please define DMOD_ARCH"
#       define DMOD_ARCH	"Unknown"
#   endif
#endif

#endif /* INC_DMOD_DEFS_H_ */
