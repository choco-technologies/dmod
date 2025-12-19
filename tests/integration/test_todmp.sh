#!/bin/bash
# Integration test for todmp tool

set -e

echo "=== todmp Integration Tests ==="

# Get the build directory (passed as first argument or default to ../build)
BUILD_DIR="${1:-../build}"

# Convert to absolute path if relative
if [[ ! "$BUILD_DIR" = /* ]]; then
    BUILD_DIR="$(cd "$(dirname "$BUILD_DIR")" && pwd)/$(basename "$BUILD_DIR")"
fi

# Try both possible locations
TODMP="$BUILD_DIR/tools/system/todmp/todmp"
if [ ! -f "$TODMP" ]; then
    TODMP="$BUILD_DIR/bin/tools/todmp"
fi
TEST_DIR="$BUILD_DIR/test_todmp_integration"

# Check if todmp exists
if [ ! -f "$TODMP" ]; then
    echo "✗ todmp not found at $TODMP"
    echo "  Tried: $BUILD_DIR/tools/system/todmp/todmp"
    echo "  Tried: $BUILD_DIR/bin/tools/todmp"
    exit 1
fi

# Clean up any previous test artifacts
rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

echo ""
echo "Test 1: Check help output"
if $TODMP --help | grep -q "Usage:"; then
    echo "✓ Help output works"
else
    echo "✗ Help output failed"
    exit 1
fi

echo ""
echo "Test 2: Check version output"
if $TODMP --version | grep -q "Dynamic Module Loader"; then
    echo "✓ Version output works"
else
    echo "✗ Version output failed"
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
    echo "Test 3: Create package from directory"
    
    # Create a package from all modules in directory
    if $TODMP test_pkg1 "$MODULE_BUILD_DIR/dmf" test_pkg1.dmp 2>&1 | grep -q "successfully created"; then
        echo "✓ Successfully created package from directory"
        
        # Check that output file was created
        if [ -f "test_pkg1.dmp" ]; then
            echo "✓ Output file exists"
        else
            echo "✗ Output file was not created"
            exit 1
        fi
    else
        echo "✗ Failed to create package from directory"
        exit 1
    fi
    
    echo ""
    echo "Test 4: List package contents"
    if $TODMP -l test_pkg1.dmp | grep -q "Package Information:"; then
        echo "✓ Successfully listed package contents"
    else
        echo "✗ Failed to list package contents"
        exit 1
    fi
    
    echo ""
    echo "Test 5: Create .dmd file for testing"
    
    # Find some modules to include
    MODULES=$(find "$MODULE_BUILD_DIR/dmf" -name "*.dmf" | head -3 | xargs -n1 basename | sed 's/\.dmf$//')
    
    if [ -z "$MODULES" ]; then
        echo "⊘ Skipping tests 5-8: No modules found"
    else
        # Create a .dmd file
        cat > test_deps.dmd << EOF
# Test dependencies file
# This file lists modules for todmp testing

EOF
        echo "$MODULES" >> test_deps.dmd
        
        echo "✓ Created test .dmd file with modules:"
        echo "$MODULES" | sed 's/^/  - /'
        
        echo ""
        echo "Test 6: Create package from .dmd file"
        
        if $TODMP test_pkg2 test_deps.dmd "$MODULE_BUILD_DIR/dmf" test_pkg2.dmp 2>&1 | grep -q "successfully created"; then
            echo "✓ Successfully created package from .dmd file"
            
            # Check that output file was created
            if [ -f "test_pkg2.dmp" ]; then
                echo "✓ Output file exists"
            else
                echo "✗ Output file was not created"
                exit 1
            fi
        else
            echo "✗ Failed to create package from .dmd file"
            exit 1
        fi
        
        echo ""
        echo "Test 7: Verify .dmd package contains only specified modules"
        
        # Count modules in package
        MODULE_COUNT=$($TODMP -l test_pkg2.dmp | grep -c "^\  \[[0-9]\]" || true)
        EXPECTED_COUNT=$(echo "$MODULES" | wc -l)
        
        if [ "$MODULE_COUNT" -eq "$EXPECTED_COUNT" ]; then
            echo "✓ Package contains exactly $MODULE_COUNT modules as specified in .dmd"
        else
            echo "✗ Package contains $MODULE_COUNT modules, expected $EXPECTED_COUNT"
            exit 1
        fi
        
        echo ""
        echo "Test 8: Verify first module is set as main"
        
        FIRST_MODULE=$(echo "$MODULES" | head -1)
        OUTPUT=$($TODMP -l test_pkg2.dmp)
        if echo "$OUTPUT" | grep -A3 "\[0\] $FIRST_MODULE" | grep -q "MAIN MODULE"; then
            echo "✓ First module '$FIRST_MODULE' is correctly set as main"
        else
            echo "✗ First module is not set as main"
            exit 1
        fi
        
        echo ""
        echo "Test 9: Create package from .dmd with custom main module"
        
        THIRD_MODULE=$(echo "$MODULES" | tail -1)
        if $TODMP test_pkg3 test_deps.dmd "$MODULE_BUILD_DIR/dmf" test_pkg3.dmp "$THIRD_MODULE" 2>&1 | grep -q "successfully created"; then
            echo "✓ Successfully created package with custom main module"
            
            # Verify custom main module
            OUTPUT3=$($TODMP -l test_pkg3.dmp)
            if echo "$OUTPUT3" | grep -A3 "$THIRD_MODULE" | grep -q "MAIN MODULE"; then
                echo "✓ Custom main module '$THIRD_MODULE' is correctly set"
            else
                echo "✗ Custom main module is not set correctly"
                exit 1
            fi
        else
            echo "✗ Failed to create package with custom main module"
            exit 1
        fi
    fi
else
    echo ""
    echo "⊘ Skipping tests 3-9: Module build directory not found"
    echo "   Run 'cmake -DDMOD_MODE=DMOD_MODULE -B build-module' first"
fi

echo ""
echo "=== All todmp integration tests passed! ==="
cd ..
rm -rf "$TEST_DIR"
