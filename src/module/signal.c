/**
 * @file signal.c
 * @brief Minimal signal.h stub required by libgcc's AEABI helpers.
 *
 * Modules are built with -nostdlib and linked directly against libgcc.a to
 * pull in the soft ARM runtime helpers they need - such as __aeabi_uldivmod/
 * __aeabi_idivmod, used for 64-bit/integer division. libgcc's "Linux" flavor
 * of the division-by-zero trap (__aeabi_ldiv0/__aeabi_idiv0, pulled in from
 * _dvmd_lnx.o) calls raise(SIGFPE) to emulate what glibc would do - but since
 * no libc is linked in, raise() is otherwise undefined, and any module that
 * performs division pulls this trap in whether or not it is ever actually
 * reachable. This stub lives in dmod_module (linked into every DMOD_MODULE
 * build, see src/module/CMakeLists.txt and paths.cmake) rather than in an
 * individual module, so it only needs to exist once for the whole ecosystem
 * instead of being rediscovered and re-stubbed by each module that happens
 * to divide.
 */
int raise(int sig)
{
    (void)sig;
    return 0;
}
