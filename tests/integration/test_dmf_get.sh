#!/bin/bash
# Integration test for dmf-get tool

set -e

echo "=== dmf-get Integration Tests ==="

# Get the build directory (passed as first argument or default to ../build)
BUILD_DIR="${1:-../build}"
DMF_GET="$BUILD_DIR/bin/tools/dmf-get"
TEST_DIR="$BUILD_DIR/test_dmf_get_integration"

# Clean up any previous test artifacts
rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

echo ""
echo "Test 1: Check help output"
if $DMF_GET --help | grep -q "Usage:"; then
    echo "✓ Help output works"
else
    echo "✗ Help output failed"
    exit 1
fi

echo ""
echo "Test 2: Check version output"
if $DMF_GET --version | grep -q "dmf-get"; then
    echo "✓ Version output works"
else
    echo "✗ Version output failed"
    exit 1
fi

echo ""
echo "Test 3: Parse manifest file"
cat > manifest.dmm << 'EOF'
# Test manifest
testmod@1.0 https://example.com/test.dmf
testmod@2.0 https://example.com/test2.dmf
mymod https://example.com/mymod.dmf
EOF

# We can't actually download from example.com, but we can test that the tool
# tries to parse the manifest correctly
mkdir -p output
if $DMF_GET -m manifest.dmm -o output testmod 2>&1 | grep -q "Parsing manifest"; then
    echo "✓ Manifest parsing attempted"
else
    echo "✗ Manifest parsing failed"
    exit 1
fi

echo ""
echo "Test 4: Test with missing module"
if ! $DMF_GET -m manifest.dmm -o output nonexistent 2>&1 | grep -q "Module not found"; then
    echo "✗ Should report module not found"
    exit 1
fi
echo "✓ Correctly reports missing module"

echo ""
echo "Test 5: Test variable substitution in manifest"
cat > manifest_vars.dmm << 'EOF'
module1 https://example.com/<tools_name>/mod.dmf
module2 https://example.com/<arch_name>/mod.dmf
EOF

if $DMF_GET -m manifest_vars.dmm -t arch/x86_64 -o output module1 2>&1 | grep -q "Parsing manifest"; then
    echo "✓ Variable substitution manifest parsed"
else
    echo "✗ Variable substitution failed"
    exit 1
fi

echo ""
echo "=== All dmf-get integration tests passed! ==="
cd ..
rm -rf "$TEST_DIR"
