/**
 * @file @DMOD_MODULE_NAME@_test.c
 * @brief Test steps for @DMOD_MODULE_NAME@
 *
 * Each DMOD_TEST_STEP defines one test step.  Steps are discovered
 * automatically at runtime and executed by the test runner.
 *
 * dmod_test_setup() runs before every step and dmod_test_teardown()
 * runs after every step.  Remove them if no common fixture is needed.
 */
#include "dmod_test.h"

void dmod_test_setup(void)
{
    /* initialise fixtures before every step */
}

void dmod_test_teardown(void)
{
    /* clean up after every step */
}

DMOD_TEST_STEP(example_pass)
{
    DMOD_TEST_EXPECT_EQ( 1 + 1, 2 );
}

DMOD_TEST_STEP(example_fail)
{
    /* Remove or replace this step - it always fails to demonstrate output */
    DMOD_TEST_FAIL_MSG( "This step intentionally fails - replace with real tests" );
}
