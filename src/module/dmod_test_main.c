/**
 * @file dmod_test_main.c
 * @brief DMOD unit-test runner
 *
 * This file provides the main() entry point for test modules created with
 * dmod_add_test().  It registers three special .dmod.inputs entries so the
 * system-side Dmod_RunTests() can drive the test lifecycle without accessing
 * module internals directly:
 *
 *   __step_failed__ — address of the dmod_test_step_failed flag
 *   __setup__       — address of the dmod_test_setup fixture hook
 *   __teardown__    — address of the dmod_test_teardown fixture hook
 *
 * Actual test steps are registered by the DMOD_TEST_STEP() macro.
 *
 * main() delegates entirely to Dmod_RunTests() and returns its result, so
 * the exit code equals the number of failed steps (0 = all passed).
 */

#include "dmod_test.h"
#include "dmod.h"

//==============================================================================
//                      TEST STATE
//==============================================================================

volatile int dmod_test_step_failed = 0;

/* Register the address of dmod_test_step_failed so Dmod_RunTests() can reset
 * and read it without accessing module internals directly. */
volatile const Dmod_ApiRegistration_t dmod_test_step_failed_registration
        DMOD_USED_SECTION(".dmod.inputs") =
{
    .Function  = (void*)&dmod_test_step_failed,
    .Signature = DMOD_MAKE_TEST_SIGNATURE(__step_failed__)
};

//==============================================================================
//                      DEFAULT SETUP / TEARDOWN (weak)
//==============================================================================

DMOD_WEAK_SYMBOL void dmod_test_setup(void)    {}
DMOD_WEAK_SYMBOL void dmod_test_teardown(void) {}

/* Register the setup and teardown hooks so Dmod_RunTests() can call them.
 * The linker resolves these to the strong definitions if the user provides
 * them, overriding the weak defaults above. */
volatile const Dmod_ApiRegistration_t dmod_test_setup_registration
        DMOD_USED_SECTION(".dmod.inputs") =
{
    .Function  = (void*)dmod_test_setup,
    .Signature = DMOD_MAKE_TEST_SIGNATURE(__setup__)
};

volatile const Dmod_ApiRegistration_t dmod_test_teardown_registration
        DMOD_USED_SECTION(".dmod.inputs") =
{
    .Function  = (void*)dmod_test_teardown,
    .Signature = DMOD_MAKE_TEST_SIGNATURE(__teardown__)
};

//==============================================================================
//                      MAIN
//==============================================================================

int main( int argc, char* argv[] )
{
    /* Dmod_GetCurrentModuleName()/Dmod_GetModuleContext() are not reliable here: they resolve
     * through the OS-level process associated with the calling thread, which only reflects
     * this test module if it was actually spawned into its own process. dmod_add_test's runner
     * may instead run this module synchronously in the caller's own process, in which case that
     * lookup would resolve to the caller's module. Dmod_GetForegroundModule() is set explicitly
     * by Dmod_Main() around this very call, so it always reflects this test module's own
     * context regardless of how it was run. */
    return Dmod_RunTests( Dmod_GetForegroundModule( Dmod_GetCurrentPid() ), argc, argv );
}
