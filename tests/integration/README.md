# Integration Tests

This directory contains integration tests for DMOD tools.

## Running Tests

All tests can be run by passing the build directory as an argument:

```bash
cd tests/integration
bash test_todmd.sh ../../build
bash test_dmf_get.sh ../../build
bash test_todmp_dependencies.sh ../../build
```

## Test Scripts

### test_todmd.sh
Tests the `todmd` tool which generates dependency files (.dmd) from DMF modules.

### test_dmf_get.sh
Tests the `dmf-get` tool which downloads and manages module dependencies.

### test_architecture_check.sh
Tests architecture verification functionality in dmf-get.

### test_todmp_dependencies.sh (New)
Tests the `todmp` dependency resolution functionality:
- Tests `-d` flag for creating packages with dependencies
- Tests dependency resolution from DMD and DMF files
- Tests architecture verification
- Tests `--dmf-dir` and `--dmfc-dir` flags for specifying search paths
- Tests fallback to environment variables (DMOD_DMF_DIR, DMOD_DMFC_DIR)
- Tests system module filtering
- Tests with both .dmf and .dmfc files
- Tests new flag-based command-line interface

## Requirements

Tests require:
- Built DMOD system tools in the build directory
- Built DMOD modules for full test coverage (run with `-DDMOD_MODE=DMOD_MODULE`)

Some tests will be skipped if modules are not available.
