#!/bin/bash
# Integration test for whereisdmf tool

set -e

echo "=== whereisdmf Integration Tests ==="

# Get the build directory (passed as first argument or default to ../build)
BUILD_DIR="${1:-../build}"

# Convert to absolute path if relative
if [[ ! "$BUILD_DIR" = /* ]]; then
    BUILD_DIR="$(cd "$(dirname "$BUILD_DIR")" && pwd)/$(basename "$BUILD_DIR")"
fi

# Try both possible locations
WHEREISDMF="$BUILD_DIR/tools/system/whereisdmf/whereisdmf"
if [ ! -f "$WHEREISDMF" ]; then
    WHEREISDMF="$BUILD_DIR/bin/tools/whereisdmf"
fi
TEST_DIR="$BUILD_DIR/test_whereisdmf_integration"

# Check if whereisdmf exists
if [ ! -f "$WHEREISDMF" ]; then
    echo "✗ whereisdmf not found at $WHEREISDMF"
    echo "  Tried: $BUILD_DIR/tools/system/whereisdmf/whereisdmf"
    echo "  Tried: $BUILD_DIR/bin/tools/whereisdmf"
    exit 1
fi

# Clean up any previous test artifacts
rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

echo ""
echo "Test 1: Check help output"
if $WHEREISDMF --help | grep -q "Usage:"; then
    echo "✓ Help output works"
else
    echo "✗ Help output failed"
    exit 1
fi

echo ""
echo "Test 2: Check version output"
if $WHEREISDMF --version | grep -q "whereisdmf ver."; then
    echo "✓ Version output works"
else
    echo "✗ Version output failed"
    exit 1
fi

echo ""
echo "Test 3: Test with no arguments"
if $WHEREISDMF 2>&1 | grep -q "Usage:"; then
    echo "✓ Correctly shows usage when no arguments provided"
else
    echo "✗ Should show usage when no arguments provided"
    exit 1
fi

echo ""
echo "Test 4: Test with too many arguments"
if $WHEREISDMF arg1 arg2 arg3 2>&1 | grep -q "Too many arguments"; then
    echo "✓ Correctly rejects too many arguments"
else
    echo "✗ Should reject too many arguments"
    exit 1
fi

echo ""
echo "Test 5: Test with non-existent module"
if $WHEREISDMF nonexistent_module_12345 2>&1 | grep -q "not found"; then
    echo "✓ Correctly reports module not found"
    
    # Verify exit code is non-zero
    if ! $WHEREISDMF nonexistent_module_12345 > /dev/null 2>&1; then
        echo "✓ Returns non-zero exit code for missing module"
    else
        echo "✗ Should return non-zero exit code for missing module"
        exit 1
    fi
else
    echo "✗ Should report module not found"
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
    # Set up DMOD_REPO_DIR to point to our test DMF directory
    # Also set DMOD_DMF_DIR and DMOD_DMFC_DIR to ensure proper search paths
    # Use DMOD_REPO_PATHS to override any built-in paths with highest priority
    export DMOD_REPO_PATHS="$MODULE_BUILD_DIR/dmf;$MODULE_BUILD_DIR/dmfc"
    export DMOD_REPO_DIR="$MODULE_BUILD_DIR/dmf"
    export DMOD_DMF_DIR="$MODULE_BUILD_DIR/dmf"
    export DMOD_DMFC_DIR="$MODULE_BUILD_DIR/dmfc"
    
    echo ""
    echo "Test 6: Find an existing module"
    
    # Find the first .dmf file
    DMF_FILE=$(find "$MODULE_BUILD_DIR/dmf" -name "*.dmf" | head -1)
    
    if [ -n "$DMF_FILE" ] && [ -f "$DMF_FILE" ]; then
        # Extract module name from file (remove path and extension)
        MODULE_NAME=$(basename "$DMF_FILE" .dmf)
        
        echo "  Testing with module: $MODULE_NAME"
        echo "  Search paths:"
        echo "    DMOD_REPO_PATHS=$DMOD_REPO_PATHS"
        echo "    DMOD_DMF_DIR=$DMOD_DMF_DIR"
        echo "    DMOD_DMFC_DIR=$DMOD_DMFC_DIR"
        
        # Test finding the module
        # Capture full output to see what whereisdmf returns
        FULL_OUTPUT=$($WHEREISDMF "$MODULE_NAME" 2>&1)
        EXIT_CODE=$?
        # Extract just the path (last line that looks like a path)
        OUTPUT=$(echo "$FULL_OUTPUT" | grep -E '^/' | tail -1)
        
        if [ $EXIT_CODE -eq 0 ]; then
            echo "✓ Successfully found module"
            
            # Verify the output contains a path
            if echo "$OUTPUT" | grep -q "/"; then
                echo "✓ Output contains a file path"
            else
                echo "✗ Output should contain a file path"
                exit 1
            fi
            
            # Verify the output file exists
            if [ -f "$OUTPUT" ]; then
                echo "✓ Returned path points to an existing file"
            else
                echo "✗ Returned path does not exist: $OUTPUT"
                echo "  Expected directory: $MODULE_BUILD_DIR"
                echo "  DMF_DIR env: $DMOD_DMF_DIR"
                echo "  DMFC_DIR env: $DMOD_DMFC_DIR"
                exit 1
            fi
            
            # Verify the returned path ends with .dmf or .dmfc
            if echo "$OUTPUT" | grep -qE '\.(dmf|dmfc)$'; then
                echo "✓ Returned path has correct extension"
            else
                echo "✗ Returned path should have .dmf or .dmfc extension"
                exit 1
            fi
        else
            echo "✗ Failed to find existing module"
            echo "  Output: $OUTPUT"
            exit 1
        fi
    else
        echo "⊘ Skipping test 6: No .dmf files found in $MODULE_BUILD_DIR/dmf"
    fi
    
    echo ""
    echo "Test 7: Test with architecture parameter"
    
    if [ -n "$DMF_FILE" ] && [ -f "$DMF_FILE" ]; then
        # Get the current architecture
        CURRENT_ARCH=$($WHEREISDMF --help | grep -oP 'Architecture.*' | head -1 || echo "x86_64")
        
        # Test with explicit architecture
        OUTPUT=$($WHEREISDMF "$MODULE_NAME" x86_64 2>&1)
        EXIT_CODE=$?
        
        if [ $EXIT_CODE -eq 0 ] || echo "$OUTPUT" | grep -q "not found"; then
            echo "✓ Architecture parameter is accepted"
        else
            echo "✗ Should accept architecture parameter"
            exit 1
        fi
    else
        echo "⊘ Skipping test 7: No .dmf files found"
    fi
    
    echo ""
    echo "Test 8: Verify output can be used in scripts"
    
    if [ -n "$DMF_FILE" ] && [ -f "$DMF_FILE" ]; then
        # Test that output can be captured and used
        MODULE_PATH=$($WHEREISDMF "$MODULE_NAME" 2>/dev/null)
        
        if [ -n "$MODULE_PATH" ] && [ -f "$MODULE_PATH" ]; then
            echo "✓ Output can be captured in variable"
            
            # Try to use the path (e.g., get file size)
            if [ -s "$MODULE_PATH" ]; then
                echo "✓ Path can be used to access the file"
            else
                echo "✗ Path should point to a non-empty file"
                exit 1
            fi
        else
            echo "✗ Should be able to capture output"
            exit 1
        fi
    else
        echo "⊘ Skipping test 8: No .dmf files found"
    fi
else
    echo ""
    echo "⊘ Skipping tests 6-8: Module build directory not found"
    echo "   Run 'cmake -DDMOD_MODE=DMOD_MODULE -B build-module' first to build test modules"
fi

echo ""
echo "=== All whereisdmf integration tests passed! ==="
cd ..
rm -rf "$TEST_DIR"
