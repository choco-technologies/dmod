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
echo "Test 6: Parse .dmd dependencies file"
cat > deps.dmd << 'EOF'
# Test dependencies file
testmod@1.0
mymod
EOF

# Test that dmf-get can parse a .dmd file
if $DMF_GET -d deps.dmd -m manifest.dmm -o output 2>&1 | grep -q "Loading dependencies"; then
    echo "✓ Dependencies file parsing attempted"
else
    echo "✗ Dependencies file parsing failed"
    exit 1
fi

echo ""
echo "Test 7: Test .dmd with from: directive"
cat > deps_with_from.dmd << 'EOF'
# Dependencies with source change
testmod@1.0
from: https://example.com/manifest.dmm
mymod
EOF

if $DMF_GET -d deps_with_from.dmd -m manifest.dmm -o output 2>&1 | grep -q "Loading dependencies"; then
    echo "✓ Dependencies with from: directive parsed"
else
    echo "✗ Dependencies with from: directive failed"
    exit 1
fi

echo ""
echo "Test 8: Test .dmd with comments and blank lines"
cat > deps_comments.dmd << 'EOF'
# Main dependencies

testmod@1.0

# Another module
mymod
EOF

if $DMF_GET -d deps_comments.dmd -m manifest.dmm -o output 2>&1 | grep -q "Loading dependencies"; then
    echo "✓ Dependencies with comments parsed"
else
    echo "✗ Dependencies with comments failed"
    exit 1
fi

echo ""
echo "Test 9: Test error when both module and dependencies file specified"
if $DMF_GET -d deps.dmd -m manifest.dmm -o output testmod 2>&1 | grep -q "Cannot specify both"; then
    echo "✓ Correctly rejects both module and dependencies file"
else
    echo "✗ Should reject both module and dependencies file"
    exit 1
fi

echo ""
echo "=== All dmf-get integration tests passed! ==="
cd ..
rm -rf "$TEST_DIR"
