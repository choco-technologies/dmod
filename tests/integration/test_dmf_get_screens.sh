#!/bin/bash
# Screen variants use one package/binary, selected before any installed files change.
set -euo pipefail
BUILD_DIR="$(cd "${1:-../../build}" && pwd)"
DMF_GET="$BUILD_DIR/bin/tools/dmf-get"
MKDMRPKG="$BUILD_DIR/bin/tools/mkdmrpkg"
TEST_DIR="$(mktemp -d)"
trap 'rm -rf "$TEST_DIR"' EXIT
cd "$TEST_DIR"
export DMOD_CACHE_DIR="$TEST_DIR/cache"
unset DMOD_SCREEN DMOD_PACKAGE_DIR DMOD_VIEWS_DIR DMOD_LICENSE_DIR
fail() { echo "FAIL: $*"; cat install.log; exit 1; }
mkdir -p source/views/{480x272,320x240} source/configs
printf 'shared binary' > source/view.dmf
printf '480 view' > source/views/480x272/main.dmv
printf '480 font' > source/views/480x272/large.dmvf
printf '320 view' > source/views/320x240/main.dmv
printf '320 font' > source/views/320x240/small.dmvf
printf 'config' > source/configs/view.ini
: > source/view.dmd
cat > source/view.dmr <<'DMR'
dmf=./view.dmf => ${destination}/view.dmf [origin=${repo_dir}/view.dmf]
dmd=./view.dmd => ${destination}/view.dmd [origin=${repo_dir}/view.dmd]
dmr=./view.dmr => ${destination}/view/view.dmr [origin=${repo_dir}/view.dmr]
configs=./configs => ${destination}/view/configs [origin=${repo_dir}/configs]
views=./views/480x272 => ${views_dir} [screen=480x272] [default=true] [origin=${views_dir}/480x272]
views=./views/320x240 => ${views_dir} [screen=320x240] [origin=${views_dir}/320x240]
DMR
"$MKDMRPKG" source/view.dmr -m view -r "$TEST_DIR/source" --views-dir "$TEST_DIR/source/views" -o package > install.log 2>&1 || fail package
cmp source/views/320x240/main.dmv package/views/320x240/main.dmv
cmp source/views/480x272/main.dmv package/views/480x272/main.dmv
(cd package && zip -qr ../view.zip .)
echo "view $TEST_DIR/view.zip" > manifest.dmm
args=(-m "$TEST_DIR/manifest.dmm" --no-fallback --skip-arch-check --skip-dmod-ver-check -y)
run() { "$DMF_GET" "${args[@]}" "$@" > install.log 2>&1 || fail "install $*"; }
reject() { if "$DMF_GET" "${args[@]}" "$@" > install.log 2>&1; then fail "expected failure: $*"; fi; }

run view -o out --views-dir "$TEST_DIR/runtime" --mini
cmp source/views/480x272/main.dmv runtime/view/main.dmv
test -f runtime/view/large.dmvf
# Resolution switches replace the managed bundle, removing old fonts.
run view -o out --views-dir "$TEST_DIR/runtime" --mini --screen 320x240
cmp source/views/320x240/main.dmv runtime/view/main.dmv
test -f runtime/view/small.dmvf
test ! -e runtime/view/large.dmvf
cmp source/view.dmf out/view.dmf
# A missing variant cannot alter an installed binary or view.
printf 'existing binary' > out/view.dmf
reject view -o out --views-dir "$TEST_DIR/runtime" --screen 800x480
grep -q '480x272.*320x240' install.log
cmp source/views/320x240/main.dmv runtime/view/main.dmv
test "$(cat out/view.dmf)" = 'existing binary'
# A staged copy failure preserves the previously installed runtime bundle.
mkdir fail-copy
cat > fail-copy/cp <<'COPY'
#!/bin/sh
case "$*" in *.dmf-get-*) exit 1;; esac
exec /usr/bin/cp "$@"
COPY
chmod +x fail-copy/cp
PATH="$TEST_DIR/fail-copy:$PATH" reject view -o out --views-dir "$TEST_DIR/runtime" --mini --screen 480x272
cmp source/views/320x240/main.dmv runtime/view/main.dmv
test -f runtime/view/small.dmvf
test ! -e runtime/view/large.dmvf
# Entry selection overrides the CLI default and combines with tagged configs.
echo 'view @screen 320x240 service=view.ini' > selected.dmd
run -d selected.dmd -o selected --screen 480x272 --config-dir "$TEST_DIR/cfg" --config-map "service:$TEST_DIR/cfg/services"
cmp source/views/320x240/main.dmv selected/views/view/main.dmv
cmp source/configs/view.ini cfg/services/view/view.ini
# Environment default; CLI has priority.
DMOD_SCREEN=320x240 run view -o env --mini
cmp source/views/320x240/main.dmv env/views/view/main.dmv
DMOD_SCREEN=320x240 run view -o cli --mini --screen 480x272
cmp source/views/480x272/main.dmv cli/views/view/main.dmv
# Nested dependencies inherit the parent's selected screen.
mkdir parent
printf parent > parent/parent.dmf
echo view > parent/parent.dmd
(cd parent && zip -qr ../parent.zip .)
echo "parent $TEST_DIR/parent.zip" >> manifest.dmm
run parent -o nested --screen 320x240
cmp source/views/320x240/main.dmv nested/views/view/main.dmv
# One destination cannot be installed with conflicting resolutions in one run.
printf 'view @screen 320x240\nview @screen 480x272\n' > conflict.dmd
reject -d conflict.dmd -o conflict
grep -q conflicting install.log
cmp source/views/320x240/main.dmv conflict/views/view/main.dmv
# Invalid requests fail instead of becoming a configuration path.
for value in 0x240 0320x240 320X240 320x 65536x240; do
    reject view --screen "$value" -o invalid
    echo "view @screen $value" > invalid.dmd
    reject -d invalid.dmd -o invalid
done
# Legacy packages keep their existing unqualified view behavior.
mkdir -p legacy/views
printf legacy > legacy/legacy.dmf
printf old > legacy/views/main.dmv
: > legacy/legacy.dmd
cat > legacy/legacy.dmr <<'DMR'
views=./views => ${views_dir}
DMR
(cd legacy && zip -qr ../legacy.zip .)
echo "legacy $TEST_DIR/legacy.zip" >> manifest.dmm
run legacy -o old --mini --screen 320x240
cmp legacy/views/main.dmv old/views/legacy/main.dmv
# Invalid metadata must fail preflight even with --mini or --ignore-missing.
sed 's/screen=320x240/screen=480x272/' package/view.dmr > package/bad.dmr
mv package/bad.dmr package/view.dmr
(cd package && zip -qr ../bad.zip .)
echo "view $TEST_DIR/bad.zip" > bad.dmm
reject view -m bad.dmm -o bad --mini --screen 320x240
test ! -f bad/view.dmf
printf 'screen variant integration: PASS\n'
