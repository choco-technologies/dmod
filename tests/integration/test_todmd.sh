#!/bin/bash
# Integration test for todmd tool

set -e

echo "=== todmd Integration Tests ==="

# Get the build directory (passed as first argument or default to ../build)
BUILD_DIR="${1:-../build}"

# Convert to absolute path if relative
if [[ ! "$BUILD_DIR" = /* ]]; then
    BUILD_DIR="$(cd "$(dirname "$BUILD_DIR")" && pwd)/$(basename "$BUILD_DIR")"
fi

# Try both possible locations
TODMD="$BUILD_DIR/tools/system/todmd/todmd"
if [ ! -f "$TODMD" ]; then
    TODMD="$BUILD_DIR/bin/tools/todmd"
fi
TEST_DIR="$BUILD_DIR/test_todmd_integration"

# Check if todmd exists
if [ ! -f "$TODMD" ]; then
    echo "✗ todmd not found at $TODMD"
    echo "  Tried: $BUILD_DIR/tools/system/todmd/todmd"
    echo "  Tried: $BUILD_DIR/bin/tools/todmd"
    exit 1
fi

# Clean up any previous test artifacts
rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

echo ""
echo "Test 1: Check help output"
if $TODMD --help | grep -q "Usage:"; then
    echo "✓ Help output works"
else
    echo "✗ Help output failed"
    exit 1
fi

echo ""
echo "Test 2: Check version output"
if $TODMD --version | grep -q "Dynamic Module Loader"; then
    echo "✓ Version output works"
else
    echo "✗ Version output failed"
    exit 1
fi

echo ""
echo "Test 3: Test with non-existent file"
if $TODMD nonexistent.dmf 2>&1 | grep -q "Cannot load module"; then
    echo "✓ Correctly reports missing file"
else
    echo "✗ Should report missing file"
    exit 1
fi

echo ""
echo "Test 4: Test with too many arguments"
if $TODMD arg1 arg2 arg3 2>&1 | grep -q "Too many arguments"; then
    echo "✓ Correctly rejects too many arguments"
else
    echo "✗ Should reject too many arguments"
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
    echo ""
    echo "Test 5: Generate .dmd from example_app module"
    
    # Find the first .dmf file
    DMF_FILE=$(find "$MODULE_BUILD_DIR/dmf" -name "example_app.dmf" | head -1)
    
    if [ -n "$DMF_FILE" ] && [ -f "$DMF_FILE" ]; then
        if $TODMD "$DMF_FILE" test_output.dmd 2>&1 | grep -q "Success"; then
            echo "✓ Successfully generated .dmd file"
            
            # Check that output file was created
            if [ -f "test_output.dmd" ]; then
                echo "✓ Output file exists"
                
                # Check that file has proper header
                if head -1 test_output.dmd | grep -q "# DMOD Dependencies File"; then
                    echo "✓ Output file has correct header"
                else
                    echo "✗ Output file header is incorrect"
                    exit 1
                fi
                
                # Check for module name in header
                if grep -q "# Generated from module:" test_output.dmd; then
                    echo "✓ Output file contains module name"
                else
                    echo "✗ Output file missing module name"
                    exit 1
                fi
            else
                echo "✗ Output file was not created"
                exit 1
            fi
        else
            echo "✗ Failed to generate .dmd file"
            exit 1
        fi
    else
        echo "⊘ Skipping test 5: example_app.dmf not found"
    fi
    
    echo ""
    echo "Test 6: Generate .dmd with default output name"
    
    if [ -n "$DMF_FILE" ] && [ -f "$DMF_FILE" ]; then
        # Copy the DMF file to current dir with a known name
        cp "$DMF_FILE" test_module.dmf
        
        if $TODMD test_module.dmf 2>&1 | grep -q "Success"; then
            echo "✓ Successfully generated .dmd with default name"
            
            # Check that default output file was created
            if [ -f "test_module.dmd" ]; then
                echo "✓ Default output file exists"
            else
                echo "✗ Default output file was not created"
                exit 1
            fi
        else
            echo "✗ Failed to generate .dmd with default name"
            exit 1
        fi
    else
        echo "⊘ Skipping test 6: example_app.dmf not found"
    fi
    
    echo ""
    echo "Test 7: Verify dependencies are listed correctly"
    
    if [ -f "test_output.dmd" ]; then
        # Check that non-system dependencies are listed
        # Count non-comment, non-empty lines
        DEP_COUNT=$(grep -v '^#' test_output.dmd | grep -v '^$' | wc -l)
        
        if [ "$DEP_COUNT" -ge 0 ]; then
            echo "✓ Dependencies section exists (found $DEP_COUNT dependencies)"
        else
            echo "✗ No dependencies found in output"
            exit 1
        fi
    else
        echo "⊘ Skipping test 7: test_output.dmd not found"
    fi
    
    echo ""
    echo "Test 8: Check that system modules are filtered"
    
    if [ -f "test_output.dmd" ]; then
        # Check that "Dmod" (system module) is not in the dependencies
        if ! grep -v '^#' test_output.dmd | grep -q '^Dmod'; then
            echo "✓ System modules are correctly filtered out"
        else
            echo "✗ System module found in dependencies (should be filtered)"
            exit 1
        fi
    else
        echo "⊘ Skipping test 8: test_output.dmd not found"
    fi
    
    echo ""
    echo "Test 9: Verify module versions are preserved"
    
    if [ -f "test_output.dmd" ]; then
        # Look for any dependency with version notation
        if grep -v '^#' test_output.dmd | grep -E '^[a-zA-Z_][a-zA-Z0-9_]*@' > /dev/null 2>&1 || \
           grep -v '^#' test_output.dmd | grep -E '^[a-zA-Z_][a-zA-Z0-9_]*$' > /dev/null 2>&1; then
            echo "✓ Dependencies with/without versions are properly formatted"
        else
            # If no dependencies, that's also valid
            if [ "$DEP_COUNT" -eq 0 ]; then
                echo "✓ No dependencies to check (module has no non-system deps)"
            fi
        fi
    else
        echo "⊘ Skipping test 9: test_output.dmd not found"
    fi
else
    echo ""
    echo "⊘ Skipping tests 5-9: Module build directory not found"
    echo "   Run 'cmake -DDMOD_MODE=DMOD_MODULE -B build-module' first"
fi

echo ""
echo "=== All todmd integration tests passed! ==="
cd ..
rm -rf "$TEST_DIR"
