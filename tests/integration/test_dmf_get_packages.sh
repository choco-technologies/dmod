#!/bin/bash
# Verify separate package roots using local archives and actual installed files.
set -euo pipefail

BUILD_DIR="$(cd "${1:-../build}" && pwd)"
DMF_GET="$BUILD_DIR/bin/tools/dmf-get"
TEST_DIR="$(mktemp -d)"
trap 'rm -rf "$TEST_DIR"' EXIT
cd "$TEST_DIR"
export DMOD_CACHE_DIR="$TEST_DIR/cache"
unset DMOD_PACKAGE_DIR DMOD_LICENSE_DIR DMOD_VIEWS_DIR

fail() {
    echo "FAIL: $*"
    cat install.log 2>/dev/null || true
    exit 1
}

mkdir -p parent/docs parent/settings parent/views parent/tools child/configs
echo 'parent dmf' > parent/parent.dmf
echo 'parent dmfc' > parent/parent.dmfc
echo 'helper dmfc' > parent/tools/helper.dmfc
echo 'parent docs' > parent/docs/README
echo 'parent config' > parent/settings/board.ini
echo 'parent view' > parent/views/main.dmv
echo 'parent license' > parent/LICENSE
echo child > parent/parent.dmd
cat > parent/parent.dmr <<'DMR'
dmf=./parent.dmf => ${destination}/${module}.dmf
dmfc=./parent.dmfc => ${destination}/${module}.dmfc
dmfc=./tools/helper.dmfc => ${destination}/${module}/tools/helper.dmfc
docs=./docs => ${destination}/${module}/docs
examples=./optional-examples => ${destination}/${module}/examples
config=./settings => ${destination}/custom-config/${module}
views=./views => ${views_dir}
license=./LICENSE => ${destination}/${module}/LICENSE
dmd=./parent.dmd => ${destination}/${module}.dmd
DMR
# No .dmr: the complete package must move, and config lookup must still work.
echo 'child dmf' > child/child.dmf
echo 'child config' > child/configs/board.ini
(cd parent && zip -qr ../parent.zip .)
(cd child && zip -qr ../child.zip .)
echo 'raw module' > raw.dmf
echo 'packed modules' > bundle.dmp
cat > manifest.dmm <<MANIFEST
parent $TEST_DIR/parent.zip
child $TEST_DIR/child.zip
raw $TEST_DIR/raw.dmf
bundle $TEST_DIR/bundle.dmp
MANIFEST
args=(-m "$TEST_DIR/manifest.dmm" --no-fallback --skip-arch-check --skip-dmod-ver-check -y)
run_install() {
    "$DMF_GET" "${args[@]}" "$@" > install.log 2>&1 || fail "install $*"
}
check_binary_dir() {
    local unexpected
    unexpected="$(find "$1" -mindepth 1 ! \( -type f \( -name '*.dmf' -o -name '*.dmfc' \) \))"
    test -z "$unexpected" || fail "non-module entries in $1: $unexpected"
}
check_packages() {
    local packages="$1" binaries="$2"
    cmp parent/parent.dmfc "$binaries/parent.dmfc"
    cmp parent/tools/helper.dmfc "$binaries/helper.dmfc"
    cmp child/child.dmf "$binaries/child.dmf"
    cmp parent/docs/README "$packages/parent/docs/README"
    test ! -e "$packages/parent/examples"
    cmp parent/settings/board.ini "$packages/custom-config/parent/board.ini"
    cmp parent/views/main.dmv "$packages/views/parent/main.dmv"
    cmp parent/LICENSE "$packages/parent/LICENSE"
    cmp parent/parent.dmd "$packages/parent.dmd"
    cmp parent/parent.dmr "$packages/parent.dmr"
    cmp child/configs/board.ini "$packages/child/configs/board.ini"
    test ! -e "$packages/parent.zip"
    test ! -e "$packages/child.zip"
    check_binary_dir "$binaries"
}

"$DMF_GET" --help > help.txt
grep -q -- '--package-dir' help.txt
grep -q 'DMOD_PACKAGE_DIR' help.txt

# Backward compatibility: without the setting metadata/resources share -o.
run_install -o legacy parent
test -f legacy/parent.dmfc
test -f legacy/parent.dmd
test -f legacy/parent/docs/README
test -f legacy/child/configs/board.ini
test -f legacy/views/parent/main.dmv

# Nested relative path with spaces/trailing slash, recursive deps, custom config mapping.
run_install -o binaries --package-dir 'nested/unpacked packages/' --config board.ini --config-dir installed-config parent
check_packages 'nested/unpacked packages' binaries
cmp parent/settings/board.ini installed-config/parent/board.ini

# Explicit uncompressed preference keeps the binary root flat as well.
run_install -o dmf-binaries --package-dir dmf-packages --type dmf --no-dependencies parent
cmp parent/parent.dmf dmf-binaries/parent.dmf
test ! -e dmf-binaries/parent.dmfc
test ! -e dmf-binaries/helper.dmfc
check_binary_dir dmf-binaries

# Reinstall into the same roots; directory resources must not gain another level.
run_install -o binaries --package-dir 'nested/unpacked packages' parent
check_packages 'nested/unpacked packages' binaries
test ! -e 'nested/unpacked packages/parent/docs/docs'

# .dmd installs, tagged configs, no-.dmr fallback, and an absolute root whose
# final component equals a module name (must not be stripped by normalization).
printf 'parent driver=board.ini\nchild service=board.ini\n' > dependencies.dmd
run_install -o dmd-binaries --package-dir "$TEST_DIR/separate/parent" -d dependencies.dmd \
    --config-map 'driver:drivers;service:services'
check_packages "$TEST_DIR/separate/parent" dmd-binaries
cmp parent/settings/board.ini drivers/parent/board.ini
cmp child/configs/board.ini services/child/board.ini

export DMOD_PACKAGE_DIR="$TEST_DIR/from env"
run_install -o env-binaries parent
check_packages "$DMOD_PACKAGE_DIR" env-binaries
export DMOD_PACKAGE_DIR="$TEST_DIR/unused env"
run_install -o cli-binaries --package-dir cli-packages parent
check_packages cli-packages cli-binaries
test ! -e "$DMOD_PACKAGE_DIR"
export DMOD_PACKAGE_DIR=''
run_install -o empty-env --no-dependencies child
test -f empty-env/child/configs/board.ini
unset DMOD_PACKAGE_DIR

# Mini mode and explicit view/license destinations keep their existing priorities.
run_install -o mini-binaries --package-dir mini-packages --mini --no-dependencies parent
check_binary_dir mini-binaries
cmp parent/views/main.dmv mini-packages/views/parent/main.dmv
test -f mini-packages/parent.dmd
test -f mini-packages/parent.dmr
test ! -e mini-packages/parent/docs
test ! -e mini-packages/parent/LICENSE
run_install -o override-binaries --package-dir override-packages --mini --no-dependencies \
    --views-dir custom-views --license-dir custom-licenses parent
check_binary_dir override-binaries
cmp parent/views/main.dmv custom-views/parent/main.dmv
cmp parent/LICENSE custom-licenses/parent/LICENSE
test ! -e override-packages/views

# Raw binaries stay in the module output; DMP archives go to the package root.
DMOD_DMF_DIR=raw-binaries run_install --package-dir raw-packages --no-dependencies raw
cmp raw.dmf raw-binaries/raw.dmf
check_binary_dir raw-binaries
run_install -o bundle-binaries --package-dir bundle-packages --no-dependencies bundle
cmp bundle.dmp bundle-packages/bundle.dmp
check_binary_dir bundle-binaries

# Resource-only commands keep the meaning of their own -o.
run_install docs parent -o docs-only --package-dir unused-packages
cmp parent/docs/README docs-only/parent/docs/README
test ! -e unused-packages

if "$DMF_GET" --package-dir > install.log 2>&1; then
    fail 'missing argument should fail'
fi
grep -q -- '--package-dir requires an argument' install.log
for path in '' '--mini' 'unsafe;path' "$(printf '%0512d' 0)"; do
    if "$DMF_GET" "${args[@]}" -o invalid --package-dir "$path" child > install.log 2>&1; then
        fail "invalid package path should fail: $path"
    fi
done

echo 'not a directory' > blocked
if "$DMF_GET" "${args[@]}" -o blocked-binaries --package-dir blocked child > install.log 2>&1; then
    fail 'unwritable package root should fail'
fi
check_binary_dir blocked-binaries

# Existing blocking paths must fail metadata copies and no-.dmr extraction.
mkdir -p metadata-blocked/parent.dmd
if "$DMF_GET" "${args[@]}" -o metadata-binaries --package-dir metadata-blocked --no-dependencies parent > install.log 2>&1; then
    fail 'metadata destination directory should not be treated as a file'
fi
mkdir -p contents-blocked
echo 'not a directory' > contents-blocked/child
if "$DMF_GET" "${args[@]}" -o contents-binaries --package-dir contents-blocked --no-dependencies child > install.log 2>&1; then
    fail 'unwritable unpacked package directory should fail'
fi
check_binary_dir contents-binaries

echo 'All dmf-get package directory tests passed!'
