#ifndef DMOD_ARCH_DEFS_H
#define DMOD_ARCH_DEFS_H

#ifdef __cplusplus
extern "C" {
#endif

//==============================================================================
//                              ARCHITECTURE DEFINITIONS
//==============================================================================
#ifndef DMOD_ARCH
#   if defined(__x86_64__) || defined(_M_X64)
#       define DMOD_ARCH	"x86_64"
#   elif defined(__ARM_ARCH_7__)
#       define DMOD_ARCH	"ARMv7" 
#   elif defined(__ARM_ARCH_4__)
#       define DMOD_ARCH	"ARMv4"
#   elif defined(__ARM_ARCH)
#       define DMOD_ARCH_HELPER(arch)  "ARMv" #arch
#       define DMOD_ARCH_STRING(arch) DMOD_ARCH_HELPER(arch)
#       define DMOD_ARCH	DMOD_ARCH_STRING(__ARM_ARCH)
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
#           define DMOD_ARCH	"RISCV32"
#       elif __riscv_xlen == 64
#           define DMOD_ARCH	"RISCV64"
#       else
#           define DMOD_ARCH	"RISCV"
#       endif
#   else
#       error "Unknown architecture. Please define DMOD_ARCH"
#       define DMOD_ARCH	"Unknown"
#   endif
#endif

#ifdef __cplusplus
}
#endif

#endif // DMOD_ARCH_DEFS_H