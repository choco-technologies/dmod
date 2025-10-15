# CI Test Templates

This directory contains templates for testing external module building in CI/CD.

## Structure

- `external_module_cmake/` - Template for testing CMake-based external modules
  - `CMakeLists.txt` - CMake configuration with `@DMOD_DIR@` placeholder
  - `external_test.c` - Simple test module source code

- `external_module_make/` - Template for testing Make-based external modules
  - `Makefile` - Make configuration with `@DMOD_DIR@` placeholder
  - `external_test.c` - Simple test module source code

## Usage

The CI workflow copies these templates to a temporary directory and replaces `@DMOD_DIR@` with the actual path to the dmod repository using `sed`.

Example:
```bash
cp -r .github/test_templates/external_module_cmake/* /tmp/test_module/
sed -i "s|@DMOD_DIR@|${GITHUB_WORKSPACE}|g" /tmp/test_module/CMakeLists.txt
```

## Purpose

These templates verify that:
1. External modules can be built outside the dmod repository tree
2. The `dmod_add_executable()` function works correctly
3. Modules can be loaded and executed with `dmod_loader`
4. Both CMake and Make build systems work with external modules
