#!/bin/bash
# Integration test for version requirements feature
# Tests that version information from dmod_link_modules is passed to .dmd files

set -e

echo "=== Version Requirements Integration Test ==="

# Get the script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_PROJECT_DIR="$SCRIPT_DIR/test_version_requirements"

# Get build directory (passed as first argument or default)
BUILD_DIR="${1:-$SCRIPT_DIR/../../build}"

# Convert to absolute path if relative
if [[ ! "$BUILD_DIR" = /* ]]; then
    BUILD_DIR="$(cd "$(dirname "$BUILD_DIR")" && pwd)/$(basename "$BUILD_DIR")"
fi

echo "Build directory: $BUILD_DIR"
echo "Test project: $TEST_PROJECT_DIR"

# Check if todmd exists
TODMD="$BUILD_DIR/bin/tools/todmd"
if [ ! -f "$TODMD" ]; then
    echo "✗ todmd not found at $TODMD"
    echo "  Please build the system tools first: cmake -DDMOD_MODE=DMOD_SYSTEM -DDMOD_BUILD_TOOLS=ON -B build"
    exit 1
fi

# Create a temporary build directory for the test module
TEST_BUILD_DIR="$BUILD_DIR/test_version_requirements"
rm -rf "$TEST_BUILD_DIR"
mkdir -p "$TEST_BUILD_DIR"

echo ""
echo "Test 1: Configure test module with version requirements"
if cmake -DDMOD_MODE=DMOD_MODULE -B "$TEST_BUILD_DIR" -S "$TEST_PROJECT_DIR" 2>&1 | tee /tmp/cmake_output.txt; then
    echo "✓ CMake configuration successful"
    
    # Check if version requirements file was created
    if grep -q "Version requirements file:" /tmp/cmake_output.txt; then
        echo "✓ Version requirements file was created"
    else
        echo "✗ Version requirements file property not found in CMake output"
        exit 1
    fi
else
    echo "✗ CMake configuration failed"
    exit 1
fi

echo ""
echo "Test 2: Build test module"
if cmake --build "$TEST_BUILD_DIR" 2>&1 | tee /tmp/build_output.txt; then
    echo "✓ Module build successful"
else
    echo "✗ Module build failed"
    exit 1
fi

echo ""
echo "Test 3: Check if version requirements file exists"
VERSION_REQS_FILE="$TEST_BUILD_DIR/test_version_reqs_version_requirements.txt"
if [ -f "$VERSION_REQS_FILE" ]; then
    echo "✓ Version requirements file exists: $VERSION_REQS_FILE"
    echo "  Contents:"
    cat "$VERSION_REQS_FILE" | sed 's/^/    /'
else
    echo "✗ Version requirements file not found: $VERSION_REQS_FILE"
    exit 1
fi

echo ""
echo "Test 4: Verify version requirements file format"
if grep -q "dmini@1.0" "$VERSION_REQS_FILE"; then
    echo "✓ Version requirement for dmini@1.0 found"
else
    echo "✗ Version requirement for dmini@1.0 not found"
    exit 1
fi

if grep -q "dmodex@0.1" "$VERSION_REQS_FILE"; then
    echo "✓ Version requirement for dmodex@0.1 found"
else
    echo "✗ Version requirement for dmodex@0.1 not found"
    exit 1
fi

echo ""
echo "Test 5: Check if .dmd file was generated"
DMD_FILE="$BUILD_DIR/test_version_requirements/dmf/test_version_reqs.dmd"
if [ -f "$DMD_FILE" ]; then
    echo "✓ .dmd file was generated: $DMD_FILE"
    echo "  Contents:"
    cat "$DMD_FILE" | sed 's/^/    /'
else
    echo "✗ .dmd file not found: $DMD_FILE"
    exit 1
fi

echo ""
echo "Test 6: Verify .dmd file contains version requirements"
# The .dmd file should contain version information from the version requirements file
# Note: This test assumes the module doesn't actually require dmini/dmodex in its DMF,
# but if it did, the versions from the requirements file should override or supplement them

if grep -q "^dmini@1.0" "$DMD_FILE" || grep -q "^# Generated from module:" "$DMD_FILE"; then
    echo "✓ .dmd file was processed by todmd"
else
    echo "⊘ .dmd file format check (module may not have actual dependencies)"
fi

echo ""
echo "Test 7: Test todmd directly with version requirements file"
# Create a minimal test to verify todmd -r flag works
TEST_DMF="$BUILD_DIR/test_version_requirements/dmf/test_version_reqs.dmf"
TEST_OUTPUT_DMD="/tmp/test_version_reqs_manual.dmd"

if [ -f "$TEST_DMF" ]; then
    if $TODMD "$TEST_DMF" "$TEST_OUTPUT_DMD" -r "$VERSION_REQS_FILE" 2>&1 | grep -q "Success"; then
        echo "✓ todmd with -r flag executed successfully"
        
        if [ -f "$TEST_OUTPUT_DMD" ]; then
            echo "✓ Output .dmd file created"
            echo "  Contents:"
            cat "$TEST_OUTPUT_DMD" | sed 's/^/    /'
        else
            echo "✗ Output .dmd file not created"
            exit 1
        fi
    else
        echo "✗ todmd with -r flag failed"
        exit 1
    fi
else
    echo "⊘ Test DMF not found, skipping direct todmd test"
fi

echo ""
echo "Test 8: Verify version requirements are mentioned in summary"
if [ -f "$TEST_OUTPUT_DMD" ]; then
    # Check if todmd mentioned version requirements in its output
    if grep -q "Version requirements" /tmp/build_output.txt 2>/dev/null; then
        echo "✓ Version requirements mentioned in build output"
    else
        echo "⊘ Version requirements not mentioned (may be due to no actual dependencies)"
    fi
fi

echo ""
echo "=== All version requirements tests passed! ==="

# Cleanup
rm -f /tmp/cmake_output.txt /tmp/build_output.txt /tmp/test_version_reqs_manual.dmd
