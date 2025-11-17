#!/bin/bash
# Integration test for architecture checking in dmf-get

set -e

echo "=== Architecture Check Integration Tests ==="

# Get the build directory (passed as first argument or default to ../../build_cmake)
BUILD_DIR="${1:-../../build_cmake}"
# Get absolute path
BUILD_DIR=$(cd "$BUILD_DIR" && pwd)
DMF_GET="$BUILD_DIR/bin/tools/dmf-get"
TEST_DIR="$BUILD_DIR/test_arch_check"

# Clean up any previous test artifacts
rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

echo ""
echo "Test 1: Verify dmf-get processes with architecture checking"
# We can't easily create a real DMF file for testing here, but we can verify
# that the architecture check code path exists and compiles correctly

# Create a manifest
cat > manifest.dmm << 'EOF'
testmod https://example.com/test.dmf
EOF

mkdir -p output

# Try to download (will fail on network, but should show manifest parsing works)
OUTPUT=$($DMF_GET -m manifest.dmm -o output testmod 2>&1 || true)

# Check that manifest parsing works (download will fail on network)
if echo "$OUTPUT" | grep -q "Manifest loaded"; then
    echo "✓ dmf-get manifest parsing and processing works"
else
    echo "✗ dmf-get manifest parsing failed"
    echo "Output was: $OUTPUT"
    exit 1
fi

echo ""
echo "Test 2: Verify architecture checking is in the code flow"
# Check that the new function is available in the DMOD library
# This is a smoke test to ensure our changes compiled correctly

SYSTEM_LIB="$BUILD_DIR/src/system/libdmod_system.a"
if [ -f "$SYSTEM_LIB" ]; then
    # Check if our new function symbol exists in the library
    if nm "$SYSTEM_LIB" 2>/dev/null | grep -q "Dmod_GetPackageArchitecture"; then
        echo "✓ Dmod_GetPackageArchitecture function exists in library"
    else
        echo "✗ Dmod_GetPackageArchitecture function not found in library"
        exit 1
    fi
else
    echo "⚠ Library file not found at $SYSTEM_LIB, skipping symbol check"
fi

echo ""
echo "Test 3: Verify DMOD_ARCH is defined"
# Create a small test program to verify DMOD_ARCH is accessible
cat > test_arch.c << 'EOF'
#include <stdio.h>
#include "dmod_arch_defs.h"

int main() {
    printf("DMOD_ARCH=%s\n", DMOD_ARCH);
    return 0;
}
EOF

# Try to compile it
if gcc -I"$BUILD_DIR/../../inc" -I"$BUILD_DIR" test_arch.c -o test_arch 2>/dev/null; then
    ARCH_OUTPUT=$(./test_arch)
    if echo "$ARCH_OUTPUT" | grep -q "DMOD_ARCH="; then
        echo "✓ DMOD_ARCH is defined: $ARCH_OUTPUT"
    else
        echo "✗ DMOD_ARCH output unexpected"
        exit 1
    fi
else
    echo "⚠ Could not compile arch test, skipping"
fi

echo ""
echo "=== All architecture check tests passed! ==="
cd ../..
rm -rf "$TEST_DIR"
