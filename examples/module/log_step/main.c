#define DMOD_ENABLE_REGISTRATION ON
#include "dmod.h"

/**
 * @brief Example module demonstrating DMOD_LOG_STEP_* macros.
 *
 * This module exercises:
 *   - DMOD_LOG_STEP_BEGIN    : show an empty progress bar at the start of an operation
 *   - DMOD_LOG_STEP_PROGRESS : update the progress bar with a percentage
 *   - DMOD_LOG_STEP          : conclude with [  OK  ] or [ FAIL ] depending on result
 *
 * The DMOD_LOG_STEP_* macros concatenate their argument with a colour-escape
 * prefix at compile time, so string literals must be used.
 */
int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    /* ---------------------------------------------------------------
     * Simulate a successful multi-step operation
     * --------------------------------------------------------------- */
    DMOD_LOG_STEP_BEGIN("Initializing subsystem\n");
    DMOD_LOG_STEP_PROGRESS(25, "Initializing subsystem\n");
    DMOD_LOG_STEP_PROGRESS(50, "Initializing subsystem\n");
    DMOD_LOG_STEP_PROGRESS(75, "Initializing subsystem\n");
    DMOD_LOG_STEP(0, "Initializing subsystem\n");

    /* ---------------------------------------------------------------
     * Simulate a failing step (non-zero result → [ FAIL ])
     * --------------------------------------------------------------- */
    DMOD_LOG_STEP_BEGIN("Running diagnostics\n");
    DMOD_LOG_STEP_PROGRESS(50, "Running diagnostics\n");
    DMOD_LOG_STEP(1, "Running diagnostics\n");

    return 0;
}
