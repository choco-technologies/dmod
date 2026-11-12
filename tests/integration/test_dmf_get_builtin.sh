#!/bin/bash
# "@builtin" entries: libraries of the system (the "lib" resource of a package) that the modules need. A plain install skips them,
# --builtin-dir installs them, the "builtin" command resolves the whole tree and installs only them.
set -euo pipefail
BUILD_DIR="$(cd "${1:-../../build}" && pwd)"
DMF_GET="$BUILD_DIR/bin/tools/dmf-get"
TEST_DIR="$(mktemp -d)"
trap 'rm -rf "$TEST_DIR"' EXIT
cd "$TEST_DIR"
export DMOD_CACHE_DIR="$TEST_DIR/cache"
unset DMOD_SCREEN DMOD_PACKAGE_DIR DMOD_VIEWS_DIR DMOD_LICENSE_DIR DMOD_BUILTIN_DIR
fail() { echo "FAIL: $*"; cat install.log; exit 1; }

# package <name> <dmd content> [lib]: a module, with a library of the system ("lib" resource and headers) if "lib" is given
package() {
    local name=$1 dmd=$2 lib=${3:-}
    mkdir -p "src/$name"
    printf 'binary of %s' "$name" > "src/$name/$name.dmf"
    printf '%b' "$dmd" > "src/$name/$name.dmd"
    {
        echo "dmf=./$name.dmf => \${destination}/$name.dmf"
        echo "dmd=./$name.dmd => \${destination}/$name.dmd"
        echo "dmr=./$name.dmr => \${destination}/$name/$name.dmr"
    } > "src/$name/$name.dmr"
    if [ -n "$lib" ]; then
        mkdir -p "src/$name/lib" "src/$name/include"
        printf 'archive of %s' "$name" > "src/$name/lib/lib$name.a"
        printf '/* %s */' "$name" > "src/$name/include/$name.h"
        echo "lib=./lib/lib$name.a => \${destination}/$name/lib/lib$name.a" >> "src/$name/$name.dmr"
        echo "inc=./include => \${destination}/$name/include" >> "src/$name/$name.dmr"
    fi
    (cd "src/$name" && zip -qr "$TEST_DIR/$name.zip" .)
    echo "$name@1.0 $TEST_DIR/$name.zip" >> manifest.dmm
}

: > manifest.dmm
# app needs the library of radio and helper; helper needs radio too (installed once); radio is a module whose package carries it
package app 'radio@1.0 @builtin\nhelper\n'
package helper 'radio@1.0 @builtin\n'
package radio 'radio@1.0 @builtin\n' lib
package broken 'nolib @builtin\n'
package nolib '' 
printf 'app\n' > flash.dmd
printf 'broken\n' > broken.dmd

args=(-m "$TEST_DIR/manifest.dmm" --no-fallback --skip-arch-check --skip-dmod-ver-check -y)
run() { "$DMF_GET" "${args[@]}" "$@" > install.log 2>&1 || fail "$*"; }
reject() { if "$DMF_GET" "${args[@]}" "$@" > install.log 2>&1; then fail "expected failure: $*"; fi; }

# A plain install: the modules, not the library of the system
run -d flash.dmd -o plain
test -f plain/app.dmf && test -f plain/helper.dmf || fail "modules not installed"
test ! -e plain/radio.dmf && test ! -e plain/radio || fail "a library of the system was installed as a module"
grep -q "radio@1.0 is a library of the system" install.log || fail "the library of the system was not reported"
test ! -e plain/builtin.dmd || fail "builtin.dmd without --builtin-dir"

# With --builtin-dir: the modules and the library of the system
run -d flash.dmd -o full --builtin-dir "$TEST_DIR/full-builtin"
test -f full/app.dmf || fail "modules not installed with --builtin-dir"
cmp src/radio/lib/libradio.a full-builtin/radio/lib/libradio.a
cmp src/radio/include/radio.h full-builtin/radio/include/radio.h
[ "$(grep -vc '^#' full-builtin/builtin.dmd)" = 1 ] || fail "builtin.dmd: $(cat full-builtin/builtin.dmd)"
grep -qx 'radio@1.0 @builtin' full-builtin/builtin.dmd || fail "builtin.dmd: $(cat full-builtin/builtin.dmd)"

# The builtin command: only the libraries of the system, from the whole tree; nothing of the modules is left behind
staging() { compgen -G '/tmp/dmf-get-builtin-*' | wc -l || true; }
before=$(staging)
run builtin -d flash.dmd -o only
[ "$(cd only && find . -type f | sort | tr '\n' ' ')" = "./builtin.dmd ./radio/include/radio.h ./radio/lib/libradio.a " ] \
    || fail "builtin installed: $(cd only && find . -type f)"
[ "$(staging)" = "$before" ] || fail "temporary directory left behind"
run builtin app -o single
test -f single/radio/lib/libradio.a || fail "builtin <module>"

# DMOD_BUILTIN_DIR is the default directory
DMOD_BUILTIN_DIR="$TEST_DIR/env-builtin" run builtin -d flash.dmd
test -f env-builtin/radio/lib/libradio.a || fail "DMOD_BUILTIN_DIR"

# A package without a library of the system is an error, and so is a builtin command with configuration
reject builtin -d broken.dmd -o broken-out
reject builtin -d flash.dmd -o conf --config-dir "$TEST_DIR/cfg"
echo "dmf-get @builtin: OK"
