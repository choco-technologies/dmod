/**
 * @file dmod_test_main.c
 * @brief DMOD unit-test runner
 *
 * This file provides the main() entry point for test modules created with
 * dmod_add_test().  It automatically discovers all test steps that were
 * registered via the DMOD_TEST_STEP() macro (stored in the .dmod.inputs
 * section) and executes them one by one, calling dmod_test_setup() before
 * each step and dmod_test_teardown() after each step.
 *
 * Optional command-line filtering: pass one or more step names as arguments
 * to run only those steps.  With no arguments all steps are executed.
 *
 * Return value of main() equals the number of failed test steps, so the
 * test binary can be used directly in CI pipelines.
 */

#include "dmod_test.h"
#include "dmod_module.h"

#include <stdint.h>
#include <string.h>

//==============================================================================
//                      TEST STATE
//==============================================================================

volatile int dmod_test_step_failed = 0;

//==============================================================================
//                      DEFAULT SETUP / TEARDOWN (weak)
//==============================================================================

DMOD_WEAK_SYMBOL void dmod_test_setup(void)    {}
DMOD_WEAK_SYMBOL void dmod_test_teardown(void) {}

//==============================================================================
//                      INTERNAL HELPERS
//==============================================================================

typedef void (*DmodTestStepFn)(void);

/* Return 1 if this step should run given the argv filter, 0 otherwise. */
static int ShouldRunStep( const char* name, int argc, char* argv[] )
{
    int i;

    if( argc <= 1 )
    {
        return 1; /* no filter: run everything */
    }

    for( i = 1; i < argc; i++ )
    {
        if( strcmp( name, argv[i] ) == 0 )
        {
            return 1;
        }
    }

    return 0;
}

//==============================================================================
//                      MAIN
//==============================================================================

int main( int argc, char* argv[] )
{
    Dmod_ApiRegistration_t* entries = NULL;
    uint32_t                count   = 0;

    int total_steps  = 0;
    int failed_steps = 0;

    Dmod_Module_GetInputs( &entries, &count );

    Dmod_Printf( "=== DMOD Test Runner ===\n" );

    for( uint32_t i = 0; i < count; i++ )
    {
        if( entries[i].Function  == NULL ) { continue; }
        if( entries[i].Signature == NULL ) { continue; }
        if( !Dmod_ApiSignature_IsTest( entries[i].Signature ) ) { continue; }

        const char*    name = Dmod_ApiSignature_GetName( entries[i].Signature );
        DmodTestStepFn fn   = (DmodTestStepFn)entries[i].Function;

        if( !ShouldRunStep( name, argc, argv ) ) { continue; }

        Dmod_Printf( "[ RUN  ] %s\n", name );

        dmod_test_step_failed = 0;
        dmod_test_setup();
        fn();
        dmod_test_teardown();

        total_steps++;

        if( dmod_test_step_failed )
        {
            Dmod_Printf( "[FAILED] %s\n", name );
            failed_steps++;
        }
        else
        {
            Dmod_Printf( "[  OK  ] %s\n", name );
        }
    }

    if( total_steps == 0 )
    {
        Dmod_Printf( "\nNo test steps found.\n" );
    }
    else
    {
        Dmod_Printf( "\n=== Results: %d/%d passed ===\n",
                     total_steps - failed_steps, total_steps );
    }

    return failed_steps;
}
