#!/bin/bash
# Integration test for dmod_loader --stack flag

set -e

echo "=== dmod_loader --stack Integration Tests ==="

# Get the build directory (passed as first argument or default to ../build)
BUILD_DIR="${1:-../build}"

# Convert to absolute path if relative
if [[ ! "$BUILD_DIR" = /* ]]; then
    BUILD_DIR="$(cd "$(dirname "$BUILD_DIR")" && pwd)/$(basename "$BUILD_DIR")"
fi

# Try both possible locations
DMOD_LOADER="$BUILD_DIR/examples/system/dmod_loader/dmod_loader"
if [ ! -f "$DMOD_LOADER" ]; then
    DMOD_LOADER="$BUILD_DIR/bin/examples/dmod_loader"
fi
TEST_DIR="$BUILD_DIR/test_dmod_loader_stack_integration"

# Check if dmod_loader exists
if [ ! -f "$DMOD_LOADER" ]; then
    echo "✗ dmod_loader not found at $DMOD_LOADER"
    echo "  Tried: $BUILD_DIR/examples/system/dmod_loader/dmod_loader"
    echo "  Tried: $BUILD_DIR/bin/examples/dmod_loader"
    exit 1
fi

# Clean up any previous test artifacts
rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

echo ""
echo "Test 1: Check --help includes --stack option"
if $DMOD_LOADER --help | grep -q "\-\-stack"; then
    echo "✓ Help output includes --stack option"
else
    echo "✗ Help output does not include --stack option"
    exit 1
fi

echo ""
echo "Test 2: Check --help includes --stack-timeout option"
if $DMOD_LOADER --help | grep -q "\-\-stack-timeout"; then
    echo "✓ Help output includes --stack-timeout option"
else
    echo "✗ Help output does not include --stack-timeout option"
    exit 1
fi

echo ""
echo "Test 3: --stack with non-existent file reports error gracefully"
OUTPUT=$($DMOD_LOADER nonexistent.dmf --stack 2>&1 || true)
if echo "$OUTPUT" | grep -q "Error\|Cannot\|error\|cannot"; then
    echo "✓ Correctly reports error for missing file"
else
    echo "✗ Should report an error for missing file"
    exit 1
fi

# If we have built modules, test with real DMF files
if [ -d "$BUILD_DIR/dmf" ] && [ -n "$(ls -A "$BUILD_DIR/dmf" 2>/dev/null)" ]; then
    MODULE_BUILD_DIR="$BUILD_DIR"
elif [ -d "$BUILD_DIR/../build-module/dmf" ]; then
    MODULE_BUILD_DIR="$BUILD_DIR/../build-module"
else
    MODULE_BUILD_DIR=""
fi

if [ -n "$MODULE_BUILD_DIR" ] && [ -d "$MODULE_BUILD_DIR/dmf" ]; then
    DMF_FILE=$(find "$MODULE_BUILD_DIR/dmf" -name "*.dmf" | head -1)

    if [ -n "$DMF_FILE" ] && [ -f "$DMF_FILE" ]; then
        echo ""
        echo "Test 4: --stack runs module in stack analysis mode"
        OUTPUT=$($DMOD_LOADER "$DMF_FILE" --stack 2>&1 || true)
        if echo "$OUTPUT" | grep -q "STACK ANALYSIS MODE"; then
            echo "✓ Stack analysis mode header printed"
        else
            echo "✗ Stack analysis mode header not printed"
            exit 1
        fi

        echo ""
        echo "Test 5: --stack output contains Stack Analysis Results"
        if echo "$OUTPUT" | grep -q "STACK ANALYSIS RESULTS"; then
            echo "✓ Stack analysis results section present"
        else
            echo "✗ Stack analysis results section missing"
            exit 1
        fi

        echo ""
        echo "Test 6: --stack output contains Stack used field"
        if echo "$OUTPUT" | grep -q "Stack used:"; then
            echo "✓ Stack used field present"
        else
            echo "✗ Stack used field missing"
            exit 1
        fi

        echo ""
        echo "Test 7: --stack output contains Stack allocated field"
        if echo "$OUTPUT" | grep -q "Stack allocated:"; then
            echo "✓ Stack allocated field present"
        else
            echo "✗ Stack allocated field missing"
            exit 1
        fi

        echo ""
        echo "Test 8: --stack with explicit 512k size"
        OUTPUT=$($DMOD_LOADER "$DMF_FILE" --stack 512k 2>&1 || true)
        if echo "$OUTPUT" | grep -q "Stack allocated:"; then
            echo "✓ Stack allocated line present with 512k size"
        else
            echo "✗ Stack allocated line not found"
            exit 1
        fi

        echo ""
        echo "Test 9: --stack with timeout option"
        OUTPUT=$($DMOD_LOADER "$DMF_FILE" --stack --stack-timeout 10 2>&1 || true)
        if echo "$OUTPUT" | grep -q "Timeout:.*10 seconds"; then
            echo "✓ Timeout correctly shown in stack analysis header"
        else
            echo "✗ Timeout not shown in stack analysis header"
            exit 1
        fi
    else
        echo ""
        echo "⊘ Skipping Tests 4-9: No .dmf files found in $MODULE_BUILD_DIR/dmf"
    fi
else
    echo ""
    echo "⊘ Skipping Tests 4-9: Module build directory not found"
    echo "   Run 'cmake -DDMOD_MODE=DMOD_MODULE -B build-module' first"
fi

echo ""
echo "=== All dmod_loader --stack integration tests passed! ==="
cd ..
rm -rf "$TEST_DIR"
