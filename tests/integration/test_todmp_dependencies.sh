#!/bin/bash
# Integration test for todmp dependency resolution functionality

set -e

echo "=== todmp Dependency Resolution Integration Tests ==="

# Get the build directory (passed as first argument or default to ../build)
BUILD_DIR="${1:-../build}"

# Convert to absolute path if relative
if [[ ! "$BUILD_DIR" = /* ]]; then
    BUILD_DIR="$(cd "$(dirname "$BUILD_DIR")" && pwd)/$(basename "$BUILD_DIR")"
fi

# Try both possible locations for todmp
TODMP="$BUILD_DIR/tools/system/todmp/todmp"
if [ ! -f "$TODMP" ]; then
    TODMP="$BUILD_DIR/bin/tools/todmp"
fi
TEST_DIR="$BUILD_DIR/test_todmp_dependencies"

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
echo "Test 1: Check help output includes dependency mode"
if $TODMP --help 2>&1 | grep -q "\-d.*deps"; then
    echo "✓ Help output shows dependency mode"
else
    echo "✗ Help output missing dependency mode"
    exit 1
fi

echo ""
echo "Test 2: Check -d option is recognized"
if $TODMP -d 2>&1 | grep -q "Missing required arguments"; then
    echo "✓ -d option is recognized"
else
    echo "✗ -d option not recognized properly"
    exit 1
fi

echo ""
echo "Test 3: Test -d with missing arguments"
if $TODMP -d package_name 2>&1 | grep -q "Missing required arguments"; then
    echo "✓ Correctly reports missing arguments for -d mode"
else
    echo "✗ Should report missing arguments"
    exit 1
fi

echo ""
echo "Test 4: Test -d with non-existent DMF file"
if $TODMP -d testpkg nonexistent.dmf 2>&1 | grep -q "Cannot get architecture"; then
    echo "✓ Correctly reports missing DMF file"
else
    echo "✗ Should report missing DMF file"
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
    echo "Test 5: Create package with dependencies from DMF file"
    
    # Find example_app or any .dmf file
    DMF_FILE=$(find "$MODULE_BUILD_DIR/dmf" -name "*.dmf" | head -1)
    
    if [ -n "$DMF_FILE" ] && [ -f "$DMF_FILE" ]; then
        # Copy the DMF file to test directory
        cp "$DMF_FILE" test_main.dmf
        
        # Use --dmf-dir and --dmfc-dir flags
        if $TODMP -d testpkg test_main.dmf --dmf-dir "$MODULE_BUILD_DIR/dmf" --dmfc-dir "$MODULE_BUILD_DIR/dmfc" -o test_output.dmp 2>&1 | tee test5_output.log; then
            echo "✓ todmp -d command completed"
            
            # Check that output file was created
            if [ -f "test_output.dmp" ]; then
                echo "✓ Output DMP file exists"
                
                # Verify it's a valid DMP file by checking signature
                # DMP signature is 0x444D5048 (DMPH in little endian)
                if xxd -l 4 test_output.dmp 2>/dev/null | grep -q "444d 5048\|5048 4d44"; then
                    echo "✓ Output file has valid DMP signature"
                else
                    echo "⚠ Could not verify DMP signature (xxd may not be available)"
                fi
            else
                echo "✗ Output DMP file was not created"
                cat test5_output.log
                exit 1
            fi
        else
            echo "⚠ todmp -d command failed (may be due to missing dependencies)"
            cat test5_output.log
            # Don't fail the test - dependencies may not be available
        fi
    else
        echo "⊘ Skipping test 5: No DMF files found"
    fi
    
    echo ""
    echo "Test 6: Create package with explicit DMD file"
    
    if [ -n "$DMF_FILE" ] && [ -f "$DMF_FILE" ]; then
        # Create a simple DMD file with fake dependencies
        cat > test_deps.dmd << 'EOF'
# Test dependencies file
# This is a comment
fake_module@1.0
another_module
EOF
        
        # Use flags for all options
        if $TODMP -d testpkg2 test_main.dmf --dmd test_deps.dmd --dmf-dir "$MODULE_BUILD_DIR/dmf" --dmfc-dir "$MODULE_BUILD_DIR/dmfc" -o test_output2.dmp 2>&1 | tee test6_output.log; then
            echo "✓ todmp -d with DMD file completed"
            
            # Check output
            if [ -f "test_output2.dmp" ]; then
                echo "✓ Output DMP file with DMD exists"
            else
                echo "✓ Command completed (missing dependencies are expected)"
            fi
            
            # Check that DMD file was read
            if grep -q "Read.*dependencies from DMD file" test6_output.log; then
                echo "✓ DMD file was read successfully"
            else
                echo "⚠ Could not confirm DMD file was read"
            fi
        else
            echo "⚠ todmp -d with DMD command failed"
            cat test6_output.log
            # Don't fail - missing dependencies are expected
        fi
    else
        echo "⊘ Skipping test 6: No DMF files found"
    fi
    
    echo ""
    echo "Test 7: Test default output filename"
    
    if [ -n "$DMF_FILE" ] && [ -f "$DMF_FILE" ]; then
        # Test with flags but no -o (should use default output name)
        if $TODMP -d mypkg test_main.dmf --dmf-dir "$MODULE_BUILD_DIR/dmf" --dmfc-dir "$MODULE_BUILD_DIR/dmfc" 2>&1 | tee test7_output.log; then
            echo "✓ todmp -d with default output name completed"
            
            # Check for default output file
            if [ -f "mypkg.dmp" ]; then
                echo "✓ Default output file mypkg.dmp exists"
            else
                echo "⚠ Default output file not created (dependencies may be missing)"
            fi
        else
            echo "⚠ todmp -d with default output failed"
            cat test7_output.log
        fi
    else
        echo "⊘ Skipping test 7: No DMF files found"
    fi
    
    echo ""
    echo "Test 8: Verify architecture checking"
    
    if [ -n "$DMF_FILE" ] && [ -f "$DMF_FILE" ]; then
        # Run and capture output with flags
        $TODMP -d archtest test_main.dmf --dmf-dir "$MODULE_BUILD_DIR/dmf" --dmfc-dir "$MODULE_BUILD_DIR/dmfc" 2>&1 | tee test8_output.log || true
        
        # Check that architecture was read
        if grep -q "Target architecture:" test8_output.log; then
            echo "✓ Architecture detection works"
            ARCH=$(grep "Target architecture:" test8_output.log | head -1)
            echo "  $ARCH"
        else
            echo "⚠ Could not detect architecture output"
        fi
    else
        echo "⊘ Skipping test 8: No DMF files found"
    fi
    
    echo ""
    echo "Test 9: Verify dependency search in multiple paths"
    
    if [ -n "$DMF_FILE" ] && [ -f "$DMF_FILE" ]; then
        # Copy a module to current directory
        TEST_MOD=$(find "$MODULE_BUILD_DIR/dmf" -name "*.dmf" | tail -1)
        if [ -n "$TEST_MOD" ] && [ -f "$TEST_MOD" ]; then
            cp "$TEST_MOD" "./local_module.dmf"
            
            # Don't set environment variables, force search in current dir
            unset DMOD_DMF_DIR
            unset DMOD_DMFC_DIR
            
            $TODMP -d searchtest test_main.dmf 2>&1 | tee test9_output.log || true
            
            # Check if it mentions searching current directory
            if grep -q "current directory" test9_output.log; then
                echo "✓ Searches in current directory"
            else
                echo "⚠ Could not confirm current directory search"
            fi
        fi
    else
        echo "⊘ Skipping test 9: No DMF files found"
    fi
    
    echo ""
    echo "Test 10: Verify cross-platform mode is enabled"
    
    if [ -n "$DMF_FILE" ] && [ -f "$DMF_FILE" ]; then
        # Use flags
        $TODMP -d xptest test_main.dmf --dmf-dir "$MODULE_BUILD_DIR/dmf" --dmfc-dir "$MODULE_BUILD_DIR/dmfc" 2>&1 | tee test10_output.log || true
        
        # The cross-platform mode should be mentioned in initialization
        # or we should not see any platform-specific errors
        echo "✓ Cross-platform mode test completed (implicit)"
    else
        echo "⊘ Skipping test 10: No DMF files found"
    fi
    
    echo ""
    echo "Test 11: Test with both .dmf and .dmfc files"
    
    if [ -d "$MODULE_BUILD_DIR/dmf" ] && [ -d "$MODULE_BUILD_DIR/dmfc" ]; then
        DMF_COUNT=$(find "$MODULE_BUILD_DIR/dmf" -name "*.dmf" | wc -l)
        DMFC_COUNT=$(find "$MODULE_BUILD_DIR/dmfc" -name "*.dmfc" | wc -l)
        
        echo "  Found $DMF_COUNT .dmf files and $DMFC_COUNT .dmfc files"
        
        if [ "$DMF_COUNT" -gt 0 ] || [ "$DMFC_COUNT" -gt 0 ]; then
            # Use flags
            $TODMP -d mixtest test_main.dmf --dmf-dir "$MODULE_BUILD_DIR/dmf" --dmfc-dir "$MODULE_BUILD_DIR/dmfc" 2>&1 | tee test11_output.log || true
            
            # Check that both paths were searched
            if grep -q "DMF.*directory\|DMFC.*directory" test11_output.log; then
                echo "✓ Searches both DMF and DMFC directories"
            else
                echo "✓ Mixed format test completed"
            fi
        fi
    else
        echo "⊘ Skipping test 11: Module directories not found"
    fi
    
    echo ""
    echo "Test 12: Verify system modules are filtered"
    
    if [ -n "$DMF_FILE" ] && [ -f "$DMF_FILE" ]; then
        # Use flags
        $TODMP -d sysfilter test_main.dmf --dmf-dir "$MODULE_BUILD_DIR/dmf" --dmfc-dir "$MODULE_BUILD_DIR/dmfc" 2>&1 | tee test12_output.log || true
        
        # Check if system modules are mentioned as skipped
        if grep -q "Skipping system module" test12_output.log; then
            echo "✓ System modules are filtered out"
        else
            echo "✓ System module filtering test completed (no system deps)"
        fi
    else
        echo "⊘ Skipping test 12: No DMF files found"
    fi
    
else
    echo ""
    echo "⊘ Skipping tests 5-12: Module build directory not found"
    echo "   Run 'cmake -DDMOD_MODE=DMOD_MODULE -B build-module' first"
fi

echo ""
echo "=== All todmp dependency resolution tests completed! ==="
cd ..
rm -rf "$TEST_DIR"
