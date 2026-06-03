# DMOD Test Module Template

This template creates a DMOD test module using the built-in unit test framework.

## Quick Start

1. Copy this directory to your module directory.
2. Rename `@DMOD_MODULE_NAME@_test.c` to `<your_module>_test.c` and adapt it.
3. Update `CMakeLists.txt` (or `Makefile`) with your module name and test sources.
4. Build and run the resulting `.dmf` with `dmod_loader`.

## Usage

Test steps are defined with the `DMOD_TEST_STEP()` macro from `dmod_test.h`:

```c
#include "dmod_test.h"

DMOD_TEST_STEP(my_feature_works)
{
    DMOD_TEST_EXPECT_EQ(compute_result(), EXPECTED_VALUE);
    DMOD_TEST_EXPECT_NOT_NULL(some_pointer);
}
```

The test runner provided by `dmod_add_test` discovers all steps automatically
and prints a summary.  The exit code equals the number of failed steps.

## Lifecycle Hooks

Define `dmod_test_setup()` and/or `dmod_test_teardown()` in any source file
to run code before/after **every** test step:

```c
void dmod_test_setup(void)
{
    // reset state, open resources, ...
}

void dmod_test_teardown(void)
{
    // cleanup, close resources, ...
}
```

## Assertion Macros

| Macro | Description |
|-------|-------------|
| `DMOD_TEST_EXPECT(cond)` | Fail if condition is false |
| `DMOD_TEST_EXPECT_TRUE(cond)` | Alias for DMOD_TEST_EXPECT |
| `DMOD_TEST_EXPECT_FALSE(cond)` | Fail if condition is true |
| `DMOD_TEST_EXPECT_EQ(a, b)` | Fail if a != b |
| `DMOD_TEST_EXPECT_NE(a, b)` | Fail if a == b |
| `DMOD_TEST_EXPECT_NULL(ptr)` | Fail if ptr != NULL |
| `DMOD_TEST_EXPECT_NOT_NULL(ptr)` | Fail if ptr == NULL |
| `DMOD_TEST_FAIL()` | Unconditionally fail |
| `DMOD_TEST_FAIL_MSG(msg, ...)` | Unconditionally fail with message |
