/**
 * @file test_example_test.c
 * @brief Example DMOD test module
 *
 * Demonstrates the dmod_add_test / DMOD_TEST_STEP framework.
 * All steps here must pass so the CI job exits with 0.
 */
#include "dmod_test.h"

void dmod_test_setup(void)
{
    /* runs before every step */
}

void dmod_test_teardown(void)
{
    /* runs after every step */
}

DMOD_TEST_STEP(arithmetic)
{
    DMOD_TEST_EXPECT_EQ( 1 + 1, 2 );
    DMOD_TEST_EXPECT_NE( 1 + 1, 3 );
}

DMOD_TEST_STEP(boolean)
{
    DMOD_TEST_EXPECT_TRUE( 1 );
    DMOD_TEST_EXPECT_FALSE( 0 );
}

DMOD_TEST_STEP(null_checks)
{
    void* p = (void*)0;
    DMOD_TEST_EXPECT_NULL( p );
    p = (void*)1;
    DMOD_TEST_EXPECT_NOT_NULL( p );
}
