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
 * Return value of main() equals the number of failed test steps, so the
 * test binary can be used directly in CI pipelines.
 */

#include "dmod_test.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* Defined in the module's generated _header.c - points to ModuleHeader which
 * is at offset 0 in the binary, i.e. equals Context->Data (binary base). */
extern volatile const Dmod_ModuleHeader_t* DMOD_Header;

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

/* Return 1 if sig starts with the test signature prefix, 0 otherwise. */
static int IsTestSignature( const char* sig )
{
    const char prefix[] = DMOD_TEST_SIGNATURE_PREFIX;
    size_t i;

    for( i = 0; i < sizeof( prefix ) - 1; i++ )
    {
        if( sig[i] != prefix[i] )
        {
            return 0;
        }
    }
    return 1;
}

/* Return the step name embedded in the signature (the part after the prefix). */
static const char* GetTestName( const char* sig )
{
    return sig + sizeof( DMOD_TEST_SIGNATURE_PREFIX ) - 1;
}

//==============================================================================
//                      MAIN
//==============================================================================

int main( int argc, char* argv[] )
{
    (void)argc;
    (void)argv;

    /* Obtain the footer via the module header.
     * DMOD_Header == binary base address (ModuleHeader is at offset 0).
     * After loading, Footer.Ptr is the relocated pointer to __footer_start. */
    Dmod_ModuleFooter_t* footer = (Dmod_ModuleFooter_t*)DMOD_Header->Footer.Ptr;
    uint8_t* base               = (uint8_t*)(uintptr_t)DMOD_Header;

    Dmod_ApiRegistration_t* entries =
        (Dmod_ApiRegistration_t*)( base + footer->Inputs.SectionStart );
    uint32_t count = footer->Inputs.SectionSize / sizeof( Dmod_ApiRegistration_t );

    int total_steps  = 0;
    int failed_steps = 0;

    Dmod_Printf( "=== DMOD Test Runner ===\n" );

    for( uint32_t i = 0; i < count; i++ )
    {
        if( entries[i].Function  == NULL ) { continue; }
        if( entries[i].Signature == NULL ) { continue; }
        if( !IsTestSignature( entries[i].Signature ) ) { continue; }

        const char*    name = GetTestName( entries[i].Signature );
        DmodTestStepFn fn   = (DmodTestStepFn)entries[i].Function;

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
