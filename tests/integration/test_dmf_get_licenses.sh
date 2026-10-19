#!/bin/bash
# Exercise license installation with local packages, without network access.
set -euo pipefail

BUILD_DIR="$(cd "${1:-../build}" && pwd)"
DMF_GET="$BUILD_DIR/bin/tools/dmf-get"
TEST_DIR="$(mktemp -d)"
trap 'rm -rf "$TEST_DIR"' EXIT
cd "$TEST_DIR"
export DMOD_CACHE_DIR="$TEST_DIR/cache"
unset DMOD_LICENSE_DIR

fail() {
    echo "FAIL: $*"
    cat install.log 2>/dev/null || true
    exit 1
}

for module in parent child; do
    mkdir -p "package_$module/legal/third-party" "package_$module/docs"
    echo "fake module" > "package_$module/$module.dmf"
    echo "$module license" > "package_$module/legal/LICENSE.txt"
    echo "$module notice" > "package_$module/legal/NOTICE"
    echo "$module third-party license" > "package_$module/legal/third-party/COPYING"
    echo "documentation" > "package_$module/docs/README"
    cat > "package_$module/$module.dmr" <<'DMR'
dmf=./${module}.dmf => ${destination}/${module}.dmf
license=./legal/LICENSE.txt => ${destination}/${module}/LICENSE.txt
license=./legal/NOTICE => ${destination}/${module}/NOTICE
license=./legal/third-party/ => ${destination}/${module}/third-party
docs=./docs => ${destination}/${module}/docs
DMR
    if [ "$module" = parent ]; then
        echo "child" > "package_$module/$module.dmd"
    fi
    (cd "package_$module" && zip -qr "../$module.zip" .)
    echo "$module $TEST_DIR/$module.zip" >> manifest.dmm
done

args=(-m "$TEST_DIR/manifest.dmm" --no-fallback --skip-arch-check --skip-dmod-ver-check)
run_install() {
    "$DMF_GET" "${args[@]}" -y "$@" > install.log 2>&1 || fail "install $*"
}
check_licenses() {
    local base="$1" module="$2"
    cmp "package_$module/legal/LICENSE.txt" "$base/$module/LICENSE.txt"
    cmp "package_$module/legal/NOTICE" "$base/$module/NOTICE"
    cmp "package_$module/legal/third-party/COPYING" "$base/$module/third-party/COPYING"
    test ! -e "$base/$module/third-party/third-party"
}

"$DMF_GET" --help > help.txt
grep -q -- '--license-dir' help.txt
grep -q 'DMOD_LICENSE_DIR' help.txt

run_install -o default --no-dependencies child
check_licenses default child
run_install -o mini-default --mini --no-dependencies child
test -f mini-default/child.dmf
test ! -e mini-default/child/LICENSE.txt

# Relative paths, spaces, trailing slashes, multiple resources and recursive dependencies.
run_install -o modules --license-dir 'collected licenses/' parent
for module in parent child; do
    check_licenses 'collected licenses' "$module"
    test -f "modules/$module.dmf"
    test -f "modules/$module/docs/README"
    test ! -e "modules/$module/LICENSE.txt"
done

# Reuse the license directory with fresh module output to exercise copying again.
run_install -o reinstall --license-dir 'collected licenses' parent
check_licenses 'collected licenses' parent
check_licenses 'collected licenses' child

# Absolute paths and .dmd input in mini mode; docs must still be skipped.
printf 'parent\nchild\n' > dependencies.dmd
run_install -d dependencies.dmd -o mini --mini --license-dir "$TEST_DIR/mini licenses"
for module in parent child; do
    check_licenses "$TEST_DIR/mini licenses" "$module"
    test -f "mini/$module.dmf"
    test ! -e "mini/$module/docs"
    test ! -e "mini/$module/LICENSE.txt"
done

export DMOD_LICENSE_DIR="$TEST_DIR/env licenses"
run_install -o env-output --mini --no-dependencies child
check_licenses "$DMOD_LICENSE_DIR" child
run_install -o cli-output --license-dir cli-licenses --no-dependencies parent
check_licenses cli-licenses parent
test ! -e "$DMOD_LICENSE_DIR/parent"
export DMOD_LICENSE_DIR=''
run_install -o empty-env --no-dependencies child
check_licenses empty-env child
unset DMOD_LICENSE_DIR

# Selecting a destination must not implicitly accept the license.
if printf 'n\n' | "$DMF_GET" "${args[@]}" -o rejected --license-dir rejected-licenses child > install.log 2>&1; then
    fail "license rejection should fail installation"
fi
test ! -e rejected-licenses/child
test ! -e rejected/child.dmf

if "$DMF_GET" --license-dir > install.log 2>&1; then
    fail "missing argument should fail"
fi
grep -q -- '--license-dir requires an argument' install.log
for path in '' '--mini' 'unsafe;path'; do
    if "$DMF_GET" "${args[@]}" -o invalid --license-dir "$path" child > install.log 2>&1; then
        fail "invalid license path should fail: $path"
    fi
done

# A valid base path can still produce an overlong per-module destination.
long_path="$(printf '%0495d' 0)"
if "$DMF_GET" "${args[@]}" -y -o too-long --license-dir "$long_path" --no-dependencies child > install.log 2>&1; then
    fail "overlong license destination should fail"
fi
grep -q 'Invalid or too long license destination' install.log

# Report failure if the requested directory cannot be created.
echo "not a directory" > blocked
if "$DMF_GET" "${args[@]}" -y -o blocked-output --license-dir blocked --no-dependencies child > install.log 2>&1; then
    fail "unwritable license destination should fail"
fi

echo "All dmf-get license installation tests passed!"
