#!/bin/bash
# Integration test for dmod_loader --info flag

set -e

echo "=== dmod_loader --info Integration Tests ==="

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
TEST_DIR="$BUILD_DIR/test_dmod_loader_info_integration"

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
echo "Test 1: Check --help includes --info option"
if $DMOD_LOADER --help | grep -q "\-\-info"; then
    echo "✓ Help output includes --info option"
else
    echo "✗ Help output does not include --info option"
    exit 1
fi

echo ""
echo "Test 2: --info with non-existent file reports error gracefully"
OUTPUT=$($DMOD_LOADER nonexistent.dmf --info 2>&1 || true)
if echo "$OUTPUT" | grep -q "Error\|Cannot\|error\|cannot"; then
    echo "✓ Correctly reports error for missing file"
else
    echo "✗ Should report an error for missing file"
    exit 1
fi

# If we have built modules, test with real DMF files
# Check in the same build directory first (CI case), then in build-module (local dev case)
if [ -d "$BUILD_DIR/dmf" ] && [ -n "$(ls -A "$BUILD_DIR/dmf" 2>/dev/null)" ]; then
    MODULE_BUILD_DIR="$BUILD_DIR"
elif [ -d "$BUILD_DIR/../build-module/dmf" ]; then
    MODULE_BUILD_DIR="$BUILD_DIR/../build-module"
else
    MODULE_BUILD_DIR=""
fi

if [ -n "$MODULE_BUILD_DIR" ] && [ -d "$MODULE_BUILD_DIR/dmf" ]; then
    # Find a DMF file to test with
    DMF_FILE=$(find "$MODULE_BUILD_DIR/dmf" -name "*.dmf" | head -1)

    if [ -n "$DMF_FILE" ] && [ -f "$DMF_FILE" ]; then
        echo ""
        echo "Test 3: --info with DMF file prints module header"
        OUTPUT=$($DMOD_LOADER "$DMF_FILE" --info 2>&1)
        if echo "$OUTPUT" | grep -q "DMF Module Information:"; then
            echo "✓ DMF header information printed"
        else
            echo "✗ DMF header information not printed"
            exit 1
        fi

        if echo "$OUTPUT" | grep -q "Name:"; then
            echo "✓ Module name field present"
        else
            echo "✗ Module name field missing"
            exit 1
        fi

        if echo "$OUTPUT" | grep -q "Architecture:"; then
            echo "✓ Architecture field present"
        else
            echo "✗ Architecture field missing"
            exit 1
        fi

        if echo "$OUTPUT" | grep -q "Module Type:"; then
            echo "✓ Module type field present"
        else
            echo "✗ Module type field missing"
            exit 1
        fi
    else
        echo ""
        echo "⊘ Skipping Test 3: No .dmf files found in $MODULE_BUILD_DIR/dmf"
    fi

    # Test with DMFC file if available
    DMFC_FILE=$(find "$MODULE_BUILD_DIR/dmf" -name "*.dmfc" | head -1)
    if [ -n "$DMFC_FILE" ] && [ -f "$DMFC_FILE" ]; then
        echo ""
        echo "Test 4: --info with DMFC file prints compressed header"
        OUTPUT=$($DMOD_LOADER "$DMFC_FILE" --info 2>&1)
        if echo "$OUTPUT" | grep -q "DMFC Compressed Module Information:"; then
            echo "✓ DMFC header information printed"
        else
            echo "✗ DMFC header information not printed"
            exit 1
        fi

        if echo "$OUTPUT" | grep -q "Compression:"; then
            echo "✓ Compression field present"
        else
            echo "✗ Compression field missing"
            exit 1
        fi

        if echo "$OUTPUT" | grep -q "Original Size:"; then
            echo "✓ Original size field present"
        else
            echo "✗ Original size field missing"
            exit 1
        fi
    else
        echo ""
        echo "⊘ Skipping Test 4: No .dmfc files found in $MODULE_BUILD_DIR/dmf"
    fi

    # Test with DMP package file if available
    DMP_FILE=$(find "$MODULE_BUILD_DIR" -name "*.dmp" | head -1)
    if [ -n "$DMP_FILE" ] && [ -f "$DMP_FILE" ]; then
        echo ""
        echo "Test 5: --info with DMP file prints package header"
        OUTPUT=$($DMOD_LOADER "$DMP_FILE" --info 2>&1)
        if echo "$OUTPUT" | grep -q "DMP Package Information:"; then
            echo "✓ DMP package information printed"
        else
            echo "✗ DMP package information not printed"
            exit 1
        fi

        if echo "$OUTPUT" | grep -q "Module Count:"; then
            echo "✓ Module count field present"
        else
            echo "✗ Module count field missing"
            exit 1
        fi
    else
        echo ""
        echo "⊘ Skipping Test 5: No .dmp files found in $MODULE_BUILD_DIR"
    fi
else
    echo ""
    echo "⊘ Skipping Tests 3-5: Module build directory not found"
    echo "   Run 'cmake -DDMOD_MODE=DMOD_MODULE -B build-module' first"
fi

echo ""
echo "=== All dmod_loader --info integration tests passed! ==="
cd ..
rm -rf "$TEST_DIR"
