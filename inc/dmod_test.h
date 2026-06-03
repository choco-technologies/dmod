/**
 * @file dmod_test.h
 * @brief DMOD Unit Test Framework
 *
 * Provides assertion macros and setup/teardown hooks for writing DMOD module
 * unit tests.  Include this header in your test source files.
 *
 * Test steps are defined with the DMOD_TEST_STEP() macro and are automatically
 * discovered and executed by the test runner that is compiled in by
 * dmod_add_test() in CMake.
 *
 * Basic usage:
 * @code
 *   #include "dmod_test.h"
 *
 *   DMOD_TEST_STEP(addition_works)
 *   {
 *       DMOD_TEST_EXPECT_EQ(1 + 1, 2);
 *   }
 * @endcode
 *
 * Optional per-test lifecycle hooks:
 * @code
 *   void dmod_test_setup(void)    { // runs before every test step }
 *   void dmod_test_teardown(void) { // runs after  every test step }
 * @endcode
 */
#ifndef INC_DMOD_TEST_H_
#define INC_DMOD_TEST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dmod.h"

//==============================================================================
//                      TEST STATE
//==============================================================================

/**
 * @brief Flag indicating whether the current test step has failed.
 *
 * Set to non-zero by EXPECT macros on failure.
 * Reset to zero before each test step by the test runner.
 * Defined in dmod_test_main.c (compiled in by dmod_add_test).
 */
extern volatile int dmod_test_step_failed;

//==============================================================================
//                      SETUP / TEARDOWN HOOKS
//==============================================================================

/**
 * @brief Called by the test runner before each test step.
 *
 * A default empty implementation is provided as a weak symbol.
 * Define this function in your test module to add common setup logic.
 */
void dmod_test_setup(void);

/**
 * @brief Called by the test runner after each test step.
 *
 * A default empty implementation is provided as a weak symbol.
 * Define this function in your test module to add common teardown logic.
 */
void dmod_test_teardown(void);

//==============================================================================
//                      ASSERTION MACROS
//==============================================================================

/**
 * @brief Expect that @p condition is true.
 *
 * Records a failure and prints a diagnostic if the condition is false.
 * The test step continues executing after the failure.
 */
#define DMOD_TEST_EXPECT( condition ) \
    do { \
        if ( !(condition) ) { \
            Dmod_Printf( "\033[31;1m  FAILED\033[0m  " __FILE__ ":%d: EXPECT( " #condition " )\n", __LINE__ ); \
            dmod_test_step_failed = 1; \
        } \
    } while( 0 )

/** @brief Alias for DMOD_TEST_EXPECT(). */
#define DMOD_TEST_EXPECT_TRUE( condition )  DMOD_TEST_EXPECT( condition )

/** @brief Expect that @p condition is false. */
#define DMOD_TEST_EXPECT_FALSE( condition ) DMOD_TEST_EXPECT( !(condition) )

/**
 * @brief Expect that @p a equals @p b.
 *
 * Uses the == operator.  For floating-point comparisons prefer an explicit
 * tolerance check with DMOD_TEST_EXPECT().
 */
#define DMOD_TEST_EXPECT_EQ( a, b ) \
    do { \
        if ( (a) != (b) ) { \
            Dmod_Printf( "\033[31;1m  FAILED\033[0m  " __FILE__ ":%d: EXPECT_EQ( " #a ", " #b " )\n", __LINE__ ); \
            dmod_test_step_failed = 1; \
        } \
    } while( 0 )

/** @brief Expect that @p a does not equal @p b. */
#define DMOD_TEST_EXPECT_NE( a, b ) \
    do { \
        if ( (a) == (b) ) { \
            Dmod_Printf( "\033[31;1m  FAILED\033[0m  " __FILE__ ":%d: EXPECT_NE( " #a ", " #b " )\n", __LINE__ ); \
            dmod_test_step_failed = 1; \
        } \
    } while( 0 )

/** @brief Expect that @p ptr is NULL. */
#define DMOD_TEST_EXPECT_NULL( ptr ) \
    do { \
        if ( (ptr) != NULL ) { \
            Dmod_Printf( "\033[31;1m  FAILED\033[0m  " __FILE__ ":%d: EXPECT_NULL( " #ptr " )\n", __LINE__ ); \
            dmod_test_step_failed = 1; \
        } \
    } while( 0 )

/** @brief Expect that @p ptr is not NULL. */
#define DMOD_TEST_EXPECT_NOT_NULL( ptr ) \
    do { \
        if ( (ptr) == NULL ) { \
            Dmod_Printf( "\033[31;1m  FAILED\033[0m  " __FILE__ ":%d: EXPECT_NOT_NULL( " #ptr " )\n", __LINE__ ); \
            dmod_test_step_failed = 1; \
        } \
    } while( 0 )

/**
 * @brief Unconditionally fail the current test step.
 */
#define DMOD_TEST_FAIL() \
    do { \
        Dmod_Printf( "\033[31;1m  FAILED\033[0m  " __FILE__ ":%d: FAIL()\n", __LINE__ ); \
        dmod_test_step_failed = 1; \
    } while( 0 )

/**
 * @brief Unconditionally fail the current test step with a formatted message.
 *
 * @param msg  printf-style format string (without trailing newline)
 * @param ...  optional format arguments
 */
#define DMOD_TEST_FAIL_MSG( msg, ... ) \
    do { \
        Dmod_Printf( "\033[31;1m  FAILED\033[0m  " __FILE__ ":%d: " msg "\n", __LINE__, ##__VA_ARGS__ ); \
        dmod_test_step_failed = 1; \
    } while( 0 )

#ifdef __cplusplus
}
#endif

#endif /* INC_DMOD_TEST_H_ */
