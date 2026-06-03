/**
 * @file @DMOD_MODULE_NAME@_test.c
 * @brief Test steps for @DMOD_MODULE_NAME@
 *
 * Each DMOD_TEST_STEP defines one test step.  Steps are discovered
 * automatically at runtime and executed by the test runner.
 *
 * Optionally define dmod_test_setup() / dmod_test_teardown() to run code
 * before / after every step.
 */
#include "dmod_test.h"

/* Optional: runs before every test step */
/* void dmod_test_setup(void)    {} */

/* Optional: runs after every test step */
/* void dmod_test_teardown(void) {} */

DMOD_TEST_STEP(example_pass)
{
    DMOD_TEST_EXPECT_EQ( 1 + 1, 2 );
}

DMOD_TEST_STEP(example_fail)
{
    /* Remove or replace this step - it always fails to demonstrate output */
    DMOD_TEST_FAIL_MSG( "This step intentionally fails - replace with real tests" );
}
