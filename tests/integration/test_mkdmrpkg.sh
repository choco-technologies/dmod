#!/bin/bash
# Integration test for mkdmrpkg tool

set -e

echo "=== mkdmrpkg Integration Tests ==="

# Get the build directory (passed as first argument or default to ../build)
BUILD_DIR="${1:-../build}"

# Convert to absolute path if relative
if [[ ! "$BUILD_DIR" = /* ]]; then
    BUILD_DIR="$(cd "$(dirname "$BUILD_DIR")" && pwd)/$(basename "$BUILD_DIR")"
fi

# Try both possible locations
MKDMRPKG="$BUILD_DIR/tools/system/mkdmrpkg/mkdmrpkg"
if [ ! -f "$MKDMRPKG" ]; then
    MKDMRPKG="$BUILD_DIR/bin/tools/mkdmrpkg"
fi
TEST_DIR="$BUILD_DIR/test_mkdmrpkg_integration"

# Check if mkdmrpkg exists
if [ ! -f "$MKDMRPKG" ]; then
    echo "✗ mkdmrpkg not found at $MKDMRPKG"
    echo "  Tried: $BUILD_DIR/tools/system/mkdmrpkg/mkdmrpkg"
    echo "  Tried: $BUILD_DIR/bin/tools/mkdmrpkg"
    exit 1
fi

# Clean up any previous test artifacts
rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

echo ""
echo "Test 1: Check help output"
if $MKDMRPKG --help | grep -q "Usage:"; then
    echo "✓ Help output works"
else
    echo "✗ Help output failed"
    exit 1
fi

echo ""
echo "Test 2: Check version output"
if $MKDMRPKG --version | grep -q "Dynamic Module Loader"; then
    echo "✓ Version output works"
else
    echo "✗ Version output failed"
    exit 1
fi

echo ""
echo "Test 3: Test with non-existent .dmr file"
if $MKDMRPKG nonexistent.dmr 2>&1 | grep -q "Failed to parse\|Cannot open"; then
    echo "✓ Correctly reports missing file"
else
    echo "✗ Should report missing file"
    exit 1
fi

echo ""
echo "Test 4: Test with unknown argument after dmr file"
if $MKDMRPKG test_dummy.dmr --unknown-arg 2>&1 | grep -q "Unknown argument\|Usage:"; then
    echo "✓ Correctly rejects unknown argument"
else
    echo "✗ Should reject unknown argument"
    exit 1
fi

echo ""
echo "Test 5: Package a simple .dmr with [origin] entries"

# Create source files
mkdir -p src_files/include
echo "int foo(void);" > src_files/include/foo.h
echo "void bar(void);" > src_files/include/bar.h
echo "binary content" > src_files/module.bin

# Create a .dmr file referencing them
cat > test.dmr <<'EOF'
# Test resource file
bin=./module.bin => ${destination}/module.bin [origin=src_files/module.bin]
inc=./include => ${destination}/include [origin=src_files/include]
EOF

if $MKDMRPKG test.dmr -m testmodule -o pkg_out 2>&1 | grep -q "Success"; then
    echo "✓ Package creation succeeded"
else
    echo "✗ Package creation failed"
    exit 1
fi

echo ""
echo "Test 6: Verify output directory structure"

if [ -f "pkg_out/module.bin" ]; then
    echo "✓ Binary file copied"
else
    echo "✗ Binary file not found in output"
    exit 1
fi

if [ -f "pkg_out/include/foo.h" ]; then
    echo "✓ Header file foo.h copied"
else
    echo "✗ Header file foo.h not found in output"
    exit 1
fi

if [ -f "pkg_out/include/bar.h" ]; then
    echo "✓ Header file bar.h copied"
else
    echo "✗ Header file bar.h not found in output"
    exit 1
fi

echo ""
echo "Test 7: Verify file content preserved"
if diff src_files/module.bin pkg_out/module.bin > /dev/null 2>&1; then
    echo "✓ File content preserved"
else
    echo "✗ File content differs"
    exit 1
fi

echo ""
echo "Test 8: Entries without [origin] are skipped"

cat > no_origin.dmr <<'EOF'
# No origin directives - should be skipped
bin=./module.bin => ${destination}/module.bin
docs=./docs => ${destination}/docs
EOF

rm -rf pkg_no_origin
if $MKDMRPKG no_origin.dmr -m testmodule -o pkg_no_origin 2>&1 | grep -q "skipped"; then
    echo "✓ Entries without [origin] are correctly skipped"
else
    echo "✗ Expected skipped message for entries without [origin]"
    exit 1
fi

# Output directory should be empty (only has the directory itself)
if [ -z "$(ls -A pkg_no_origin 2>/dev/null)" ]; then
    echo "✓ Output directory is empty when no origins are present"
else
    echo "✗ Output directory should be empty when no origins are present"
    exit 1
fi

echo ""
echo "Test 9: Multiple [origin] directives for one entry"

mkdir -p multi_src/a multi_src/b
echo "file_a" > multi_src/a/a.txt
echo "file_b" > multi_src/b/b.txt

cat > multi_origin.dmr <<'EOF'
combined=./combined => ${destination}/combined [origin=multi_src/a] [origin=multi_src/b]
EOF

rm -rf pkg_multi
if $MKDMRPKG multi_origin.dmr -m testmodule -o pkg_multi 2>&1 | grep -q "Success"; then
    echo "✓ Multiple origins processed"
else
    echo "✗ Multiple origins failed"
    exit 1
fi

if [ -f "pkg_multi/combined/a.txt" ] && [ -f "pkg_multi/combined/b.txt" ]; then
    echo "✓ Both origin directories merged correctly"
else
    echo "✗ Not all files from multiple origins found"
    exit 1
fi

echo ""
echo "Test 10: Variable substitution in .dmr"

mkdir -p repo_dir/include
echo "// repo header" > repo_dir/include/mymodule_api.h

cat > varsubst.dmr <<'EOF'
inc=./mymodule/include => ${destination}/mymodule/include [origin=${repo_dir}/include]
EOF

rm -rf pkg_varsubst
if $MKDMRPKG varsubst.dmr -m mymodule -r "$TEST_DIR/repo_dir" -o pkg_varsubst 2>&1 | grep -q "Success"; then
    echo "✓ Variable substitution works"
else
    echo "✗ Variable substitution failed"
    exit 1
fi

if [ -f "pkg_varsubst/mymodule/include/mymodule_api.h" ]; then
    echo "✓ File placed at correct substituted path"
else
    echo "✗ File not found at substituted path"
    exit 1
fi

echo ""
echo "Test 11: --name sets the output directory name"

mkdir -p named_src
echo "named content" > named_src/named.bin

cat > named.dmr <<'EOF'
bin=./named.bin => ${destination}/named.bin [origin=named_src/named.bin]
EOF

rm -rf mymodule-1.0.0
if $MKDMRPKG named.dmr -m testmodule --name mymodule-1.0.0 2>&1 | grep -q "Success"; then
    echo "✓ Package creation with --name succeeded"
else
    echo "✗ Package creation with --name failed"
    exit 1
fi

if [ -f "mymodule-1.0.0/named.bin" ]; then
    echo "✓ Output placed in directory named after --name value"
else
    echo "✗ Output directory not named after --name value"
    exit 1
fi

echo ""
echo "Test 12: -n short form also sets the output directory name"

rm -rf mymodule-short
if $MKDMRPKG named.dmr -m testmodule -n mymodule-short 2>&1 | grep -q "Success"; then
    echo "✓ Package creation with -n succeeded"
else
    echo "✗ Package creation with -n failed"
    exit 1
fi

if [ -f "mymodule-short/named.bin" ]; then
    echo "✓ Output placed in directory named after -n value"
else
    echo "✗ Output directory not named after -n value"
    exit 1
fi

echo ""
echo "Test 13: -o overrides --name for the output directory"

rm -rf pkg_override_name mymodule-override
if $MKDMRPKG named.dmr -m testmodule --name mymodule-override -o pkg_override_name 2>&1 | grep -q "Success"; then
    echo "✓ Package creation with --name and -o succeeded"
else
    echo "✗ Package creation with --name and -o failed"
    exit 1
fi

if [ -f "pkg_override_name/named.bin" ]; then
    echo "✓ -o directory used (overrides --name)"
else
    echo "✗ Expected -o to override --name"
    exit 1
fi

if [ -d "mymodule-override" ]; then
    echo "✗ --name directory should not be created when -o is provided"
    exit 1
else
    echo "✓ --name directory not created (correctly overridden by -o)"
fi

echo ""
echo "Test 14: --add-file copies extra file into the output directory root"

echo "Release notes for v1.0.0" > release-notes.txt

cat > addfile.dmr <<'EOF'
bin=./named.bin => ${destination}/named.bin [origin=named_src/named.bin]
EOF

rm -rf pkg_addfile
if $MKDMRPKG addfile.dmr -m testmodule -o pkg_addfile --add-file release-notes.txt 2>&1 | grep -q "Success"; then
    echo "✓ Package creation with --add-file succeeded"
else
    echo "✗ Package creation with --add-file failed"
    exit 1
fi

if [ -f "pkg_addfile/release-notes.txt" ]; then
    echo "✓ Extra file copied into output directory"
else
    echo "✗ Extra file not found in output directory"
    exit 1
fi

if diff release-notes.txt pkg_addfile/release-notes.txt > /dev/null 2>&1; then
    echo "✓ Extra file content preserved"
else
    echo "✗ Extra file content differs"
    exit 1
fi

echo ""
echo "Test 15: Multiple --add-file options"

echo "Changelog entry" > changelog.txt

rm -rf pkg_multi_addfile
if $MKDMRPKG addfile.dmr -m testmodule -o pkg_multi_addfile \
    --add-file release-notes.txt --add-file changelog.txt 2>&1 | grep -q "Success"; then
    echo "✓ Package creation with multiple --add-file succeeded"
else
    echo "✗ Package creation with multiple --add-file failed"
    exit 1
fi

if [ -f "pkg_multi_addfile/release-notes.txt" ] && [ -f "pkg_multi_addfile/changelog.txt" ]; then
    echo "✓ Both extra files copied into output directory"
else
    echo "✗ Not all extra files found in output directory"
    exit 1
fi

echo ""
echo "=== All mkdmrpkg integration tests passed! ==="
cd ..
rm -rf "$TEST_DIR"
